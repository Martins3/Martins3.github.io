# ipc/msg.md


## 问题
1. msg queue 和 signal 机制的差别是什么 ?
    1. msg queue 的信息是内核保存的，A send，B rcv 但是 A B 的生命周期没有联系。
       2. 似乎 signal 没有中间寄存的机制吧!
    2. msg queue 使用 msg queue id ，而 signal 是需要指定发送的对象的 pid 的

## core struct

```c
/* one msq_queue structure for each present queue on the system */
struct msg_queue {
	struct kern_ipc_perm q_perm;
	time64_t q_stime;		/* last msgsnd time */
	time64_t q_rtime;		/* last msgrcv time */
	time64_t q_ctime;		/* last change time */
	unsigned long q_cbytes;		/* current number of bytes on queue */
	unsigned long q_qnum;		/* number of messages in queue */
	unsigned long q_qbytes;		/* max number of bytes on queue */
	struct pid *q_lspid;		/* pid of last msgsnd */
	struct pid *q_lrpid;		/* last receive pid */

	struct list_head q_messages;
	struct list_head q_receivers;
	struct list_head q_senders;
} __randomize_layout;

/* one msg_receiver structure for each sleeping receiver */
struct msg_receiver {
	struct list_head	r_list;
	struct task_struct	*r_tsk;

	int			r_mode;
	long			r_msgtype;
	long			r_maxsize;

	struct msg_msg		*r_msg;
};

/* one msg_sender for each sleeping sender */
struct msg_sender {
	struct list_head	list;
	struct task_struct	*tsk;
	size_t                  msgsz;
};
```
1. 阻塞的 receiver 队列，阻塞的 sender 队列 以及


## msgget : 获取 msg queue identifier
- msgget
- msgsnd
- msgctl
- msgrcv

```c
long ksys_msgget(key_t key, int msgflg)
{
	struct ipc_namespace *ns;
	static const struct ipc_ops msg_ops = {
		.getnew = newque,
		.associate = security_msg_queue_associate,
	};
	struct ipc_params msg_params;

	ns = current->nsproxy->ipc_ns;

	msg_params.key = key;
	msg_params.flg = msgflg;

	return ipcget(ns, &msg_ids(ns), &msg_ops, &msg_params); // ns
}

/**
 * ipcget - Common sys_*get() code
 * @ns: namespace
 * @ids: ipc identifier set
 * @ops: operations to be called on ipc object creation, permission checks
 *       and further checks
 * @params: the parameters needed by the previous operations.
 *
 * Common routine called by sys_msgget(), sys_semget() and sys_shmget().
 */
int ipcget(struct ipc_namespace *ns, struct ipc_ids *ids,
			const struct ipc_ops *ops, struct ipc_params *params)
{
	if (params->key == IPC_PRIVATE)
		return ipcget_new(ns, ids, ops, params);
	else
		return ipcget_public(ns, ids, ops, params);
}

/**
 * ipcget_new -	create a new ipc object
 * @ns: ipc namespace
 * @ids: ipc identifier set
 * @ops: the actual creation routine to call
 * @params: its parameters
 *
 * This routine is called by sys_msgget, sys_semget() and sys_shmget()
 * when the key is IPC_PRIVATE.
 */
static int ipcget_new(struct ipc_namespace *ns, struct ipc_ids *ids,
		const struct ipc_ops *ops, struct ipc_params *params)
{
	int err;

	down_write(&ids->rwsem);
	err = ops->getnew(ns, params); // 调用 ksys_msgget 注册的 newque 
	up_write(&ids->rwsem);
	return err;
}
```

## msgctl

```c
long ksys_msgctl(int msqid, int cmd, struct msqid_ds __user *buf)
{
	int version;
	struct ipc_namespace *ns;
	struct msqid64_ds msqid64;
	int err;

	if (msqid < 0 || cmd < 0)
		return -EINVAL;

	version = ipc_parse_version(&cmd);
	ns = current->nsproxy->ipc_ns;

	switch (cmd) {
	case IPC_INFO:
	case MSG_INFO: {
		struct msginfo msginfo;
		err = msgctl_info(ns, msqid, cmd, &msginfo);
		if (err < 0)
			return err;
		if (copy_to_user(buf, &msginfo, sizeof(struct msginfo)))
			err = -EFAULT;
		return err;
	}
	case MSG_STAT:	/* msqid is an index rather than a msg queue id */
	case MSG_STAT_ANY:
	case IPC_STAT:
		err = msgctl_stat(ns, msqid, cmd, &msqid64);
		if (err < 0)
			return err;
		if (copy_msqid_to_user(buf, &msqid64, version))
			err = -EFAULT;
		return err;
	case IPC_SET:
		if (copy_msqid_from_user(&msqid64, buf, version))
			return -EFAULT;
		/* fallthru */
	case IPC_RMID:
		return msgctl_down(ns, msqid, cmd, &msqid64);
	default:
		return  -EINVAL;
	}
}
```
1. msgctl_info : 静态信息
2. msgctl_stat : 动态统计
3. msgctl_down : 删除 msg queue

       IPC_RMID
              Immediately remove the message queue, awakening all waiting reader and writer processes (with an error return and errno set to EIDRM).  The calling process must have appropriate privileges or its effective user ID must be either that of the creator or owner of the message queue.  The third argument to msgctl() is ignored in this case.


```c
/*
 * freeque() wakes up waiters on the sender and receiver waiting queue,
 * removes the message queue from message queue ID IDR, and cleans up all the
 * messages associated with this queue.
 *
 * msg_ids.rwsem (writer) and the spinlock for this message queue are held
 * before freeque() is called. msg_ids.rwsem remains locked on exit.
 */
static void freeque(struct ipc_namespace *ns, struct kern_ipc_perm *ipcp)
{
	struct msg_msg *msg, *t;
	struct msg_queue *msq = container_of(ipcp, struct msg_queue, q_perm);
	DEFINE_WAKE_Q(wake_q);

	expunge_all(msq, -EIDRM, &wake_q);
	ss_wakeup(msq, &wake_q, true);
	msg_rmid(ns, msq);
	ipc_unlock_object(&msq->q_perm);
	wake_up_q(&wake_q);
	rcu_read_unlock();

	list_for_each_entry_safe(msg, t, &msq->q_messages, m_list) {
		atomic_dec(&ns->msg_hdrs);
		free_msg(msg);
	}
	atomic_sub(msq->q_cbytes, &ns->msg_bytes);
	ipc_update_pid(&msq->q_lspid, NULL);
	ipc_update_pid(&msq->q_lrpid, NULL);
	ipc_rcu_putref(&msq->q_perm, msg_rcu_free);
}
```


## msgsnd

```c
long ksys_msgsnd(int msqid, struct msgbuf __user *msgp, size_t msgsz,
		 int msgflg)
{
	long mtype;

	if (get_user(mtype, &msgp->mtype))
		return -EFAULT;
	return do_msgsnd(msqid, mtype, msgp->mtext, msgsz, msgflg);
}
```

1. get_user 的实现 @todo
2. do_msgsnd : 如 Man msgsnd(2) 中间的描述，当 queue 超过限制，那么就会阻塞 @todo 阻塞的技术值得分析

## msgrcv
1. 和 msgsnd 的内容对称

```c
long ksys_msgrcv(int msqid, struct msgbuf __user *msgp, size_t msgsz,
		 long msgtyp, int msgflg)
{
	return do_msgrcv(msqid, msgp, msgsz, msgtyp, msgflg, do_msg_fill);
}
```


## ipc_ids
1. 哎，只是用来管理 id 的而已，为了正确分配的一下数值
2. struct ipc_namespace ，此时才发现，ipc_namespace 中间就是管理各种 id 而已。

```c
#define msg_ids(ns)	((ns)->ids[IPC_MSG_IDS])

struct ipc_ids {
	int in_use;
	unsigned short seq;
	struct rw_semaphore rwsem;
	struct idr ipcs_idr;
	int max_idx;
#ifdef CONFIG_CHECKPOINT_RESTORE
	int next_id;
#endif
	struct rhashtable key_ht;
};
```
