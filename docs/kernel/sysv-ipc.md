## sysv ipc
<!-- 7f1f67e7-4b50-469a-ab89-062397de39fd -->
**semaphores**, **message queues**, and **shared memory**

sysv 的 ipc 都是有 posix 的机制替代的:
1. share memory (shmget)
  - memfd
  - shm_open (就是创建一个文件在 /dev/shm/shm1 )
2. msg queue (msgsnd)
  - mq_open (mq_timedreceive)
  - unix domain socket
3. sem (semop semget)
  - sem_open (futex)

1. fs/pipe.c 和 fs/splice.c 和这个不是一个体系的内容

| file         | blank | comment | code | desc       |
|--------------|-------|---------|------|------------|
| sem.c        | 299   | 570     | 1556 |            |
| shm.c        | 231   | 245     | 1277 |            |
| mqueue.c     | 196   | 139     | 1259 |            |
| msg.c        | 186   | 136     | 975  |            |
| util.c       | 99    | 305     | 460  |            |
| ipc_sysctl.c | 22    | 10      | 192  |            |
| util.h       | 41    | 53      | 186  |            |
| syscall.c    | 21    | 12      | 166  | Man ipc(2) |
| namespace.c  | 30    | 30      | 146  |            |
| msgutil.c    | 31    | 15      | 136  |            |
| mq_sysctl.c  | 12    | 10      | 102  |            |
| compat.c     | 6     | 23      | 53   |            |
| Makefile     | 2     | 4       | 6    |            |

1. Semaphores : `sem/sem.c`


```c
struct ipc_namespace {
  ...
  struct ipc_ids *ids[3];
  /* Resource limits */
  ...
}
```
omitted a large number of elements devoted to observing resource consumption and setting
resource limits. The kernel, for instance, restricts the maximum number of shared memory pages, the
maximum size for a shared memory segment, the maximum number of message queues, and so on.

More interesting is the **array ids**.
One array position per IPC mechanism — shared memory, semaphores,
and messages — exists, and each points to an instance of struct ipc_ids
that is the basis to keep track
of the existing IPC objects per category.
But just in case
you were wondering, semaphores live in position 0, followed by message queues and then by shared
memory

> ipc_namespace 哇，又一个namespace

```c
struct ipc_ids {
	int in_use;
	unsigned short seq;
	struct rw_semaphore rwsem;
	struct idr ipcs_idr; // 通过这一个和kern_ipc_perm 相联系起来
	int next_id;
};
```
The first elements hold general information on the status of the IPC objects:
> The identifier visible to userland is given by `s*SEQ_MULTIPLIER+i` 这一段很迷，非常迷

Each
object has a kernel-internal ID, and `ipcs_idr`
is **used to associate an ID with a pointer to the corresponding `kern_ipc_perm` instance**.
Since the number of used IPC objects can grow and shrink dynamically,
a static array would not serve well to manage the information, but the kernel provides a radix-tree-like
(see Appendix C) standard data structure in `lib/idr.c` for this purpose. How the entries are managed
in detail is not relevant for our purposes; **it suffices to know that each internal ID can be associated with
the respective `kern_ipc_perm` instance without problems**

```c
/* used by in-kernel data structures */
struct kern_ipc_perm
{
	spinlock_t	lock;
	bool		deleted;
	int		id;
	key_t		key;
	kuid_t		uid;
	kgid_t		gid;
	kuid_t		cuid;
	kgid_t		cgid;
	umode_t		mode;
	unsigned long	seq;
	void		*security;
};
```
The structure can be used not only for semaphores but also for other IPC mechanisms. You will come
across it frequently in this chapter.
1. `key` holds the magic number used by user programs to identify the semaphore, and id is the
kernel-internal identifier.
2. `uid` and `gid` specify the user and group ID of the owner. `cuid` and `cgid` hold the same data for
the process that generated the semaphore.
3. `seq` is a sequence number that was used when the object was reserved.
4. `mod`e holds the bitmask, which specifies access permissions in accordance with the owner, group,
others scheme.


The above data structures are not sufficient to keep all information required for semaphores. A special
per-task element is required:
```c
struct task_struct {
...
#ifdef CONFIG_SYSVIPC
  /* ipc stuff */
    struct sysv_sem sysvsem;
    struct sysv_shm sysvshm;
#endif
...
}


struct sysv_sem {
	struct sem_undo_list *undo_list;
};
```
The only member, undo_list, is used to permit semaphore manipulations that can be undone


`sem_queue` is another data structure that is used to associate a semaphore with a sleeping process that
wants to perform a semaphore operation but is not allowed to do so at the moment. In other words, each
instance of the data structure is an entry in the list of pending operations


```c
/* One queue for each sleeping process in the system. */
struct sem_queue {
	struct list_head	list;	 /* queue of pending operations */
	struct task_struct	*sleeper; /* this process */
	struct sem_undo		*undo;	 /* undo structure */
	int			pid;	 /* process id of requesting process */
	int			status;	 /* completion status of operation */
	struct sembuf		*sops;	 /* array of pending operations */
	struct sembuf		*blocking; /* the operation that blocked */
	int			nsops;	 /* number of operations */
	int			alter;	 /* does *sops alter the array? */
};
```
1. `sleeper` is a pointer to the task structure of the process waiting for permission to perform a
semaphore operation.(既然有pid, 为什么还需要sleeper指针指向的task_struct, 内核中间无法访问)
2. `pid` specifies the PID of the waiting process.
3. `id` holds the kernel-internal semaphore identifier.(为什么需要in-kernel semaphore identifier)
4. `sops` is a pointer to an array that holds the pending semaphore operations
5. `alter` indicates whether the operations alter the value of the semaphore

> 实际上根本就找不到sem_queue 指向 sem_array 的指针，两者的联系也无从说起。
> 所以后面的图其实也是不准确的

> 最关键的问题是: 到底是一步步实现应用层的几个函数的

#### 5.3.3 Message Queues
Even if several processes are listening in on a channel, each message
can be read by one process only.

The starting point is the appropriate `ipc_ids` instance of the current namespace.
> fuck，namespace again

Again, the internal ID numbers are formally associated with `kern_ipc_perm` instances, but as in the
semaphore case, a different data type (struct `msg_queue`) is obtained as a result of type conversion
> 好吧，并不知道两者到底是如何连接起来的，也不知道两者表达的是什么意思



```
/* one msq_queue structure for each present queue on the system */
struct msg_queue {
	struct kern_ipc_perm q_perm;
	time_t q_stime;			/* last msgsnd time */
	time_t q_rtime;			/* last msgrcv time */
	time_t q_ctime;			/* last change time */
	unsigned long q_cbytes;		/* current number of bytes on queue */
	unsigned long q_qnum;		/* number of messages in queue */
	unsigned long q_qbytes;		/* max number of bytes on queue */
	pid_t q_lspid;			/* pid of last msgsnd */
	pid_t q_lrpid;			/* last receive pid */

	struct list_head q_messages;
	struct list_head q_receivers;
	struct list_head q_senders;
};
```


Each message in `q_messages` is encapsulated in an instance of `msg_msg`.
```
/* one msg_msg structure for each message */
struct msg_msg {
	struct list_head m_list;
	long m_type;
	size_t m_ts;		/* message text size */
	struct msg_msgseg *next;
	void *security;
	/* the actual message follows immediately */
};
```


Sleeping senders are placed on the `q_senders` list of `msg_queue` using the following data structure:
```
/* one msg_sender for each sleeping sender */
struct msg_sender {
	struct list_head	list;
	struct task_struct	*tsk;
};
```

The data structure to hold the receiver process in the `q_receivers` list is a little longer.
```
/* one msg_receiver structure for each sleeping receiver */
struct msg_receiver {
	struct list_head	r_list;
	struct task_struct	*r_tsk;

	int			r_mode;
	long			r_msgtype;
	long			r_maxsize;

	/*
	 * Mark r_msg volatile so that the compiler
	 * does not try to get smart and optimize
	 * it. We rely on this for the lockless
	 * receive algorithm.
	 */
	struct msg_msg		*volatile r_msg;
};
```

![](../img/5-6.png)
> 中间的联系其实没有看懂，好伐
> 如何上层的接口也是不知道的
> 难道介绍几个数据结构我就知道你在说什么吗 ?


#### 5.3.4 Shared Memory
 Its essential aspects do not differ from those of semaphores and message queues.
1. Applications request an IPC object that can be accessed via a `common magic number` and a
`kernel-internal identifier` via the current namespace.
2. Access to memory can be restricted by means of a system of privileges.
3. System calls are used to allocate memory that is associated with the IPC object and that can be
accessed by all processes with the appropriate authorization.


A dummy
file linked with the corresponding instance of `shmid_kernel` via `shm_file` is created for each shared
memory object. The kernel uses the pointer `smh_file->f_mapping` to access the address space object
(struct address_space) used to create `anonymous mappings` as described in Chapter 4. The page tables
of the processes involved are set up so that each process is able to access the areas linked with the region.
> 听听，这是人说的话吗 ?


> 所以为什么要创建这么多的蛇皮IPC机制，直接都采用shared memory 不就完了 ?


# ipc/mqueue.c

> @todo 根据前面的分析，猜测 shm 的实现就是 获取一个 id ，然后得到内核分配的内存，然后拷贝到自己的空间中间 ?
> 太过于轻视这个东西了


1. ipcmq_sysctl.c 提供了关于 proc 的接口

## todo
1. 体会 sysv msg queue 的设计上的差别
2. mq_notify 实现的异步机制
3. proc 的 interface 如何实现 ?
4. 那么如何实现 namespace 的隔离 ?
    1. vfsmount
6. msg.c 和 mqueue.c 都存在 COMPAT 是做什么的 ?
5. 也是采用队列的方法吗 ? 当容量不足的时候会出现 阻塞的吗 ?

## ref && doc

Man mq_overview(7)

> POSIX message queues allow processes to exchange data in the form of messages.  This API is distinct from that provided by System V message queues (msgget(2), msgsnd(2), msgrcv(2), etc.), but provides similar functionality.

一个是 Posix , 一个是 System V message queue

> On Linux, message queues are created in a virtual filesystem.

> On Linux, a message queue descriptor is actually a file descriptor.

利用了文件系统实现

## mq_unlink
1. mq_link 在哪里 ?

```c
SYSCALL_DEFINE1(mq_unlink, const char __user *, u_name)
```
1. lookup_one_len 找到 inode
2. vfs_unlink 实现 unlink

## mq_send && mq_timedsend
1. 根本没有 mq_send 这个 syscall , 应该是 glibc 封装出来的


> Man mq_timedsend(2)
> mq_timedsend() behaves just like mq_send(), except that if the queue is full and the O_NONBLOCK flag is not enabled for the message queue description, then abs_timeout points to a structure which specifies how long the call will block.  This value is an absolute timeout in seconds and nanoseconds since the Epoch, 1970-01-01 00:00:00 +0000 (UTC), specified in the following structure:

```c
SYSCALL_DEFINE5(mq_timedsend, mqd_t, mqdes, const char __user *, u_msg_ptr,
		size_t, msg_len, unsigned int, msg_prio,
		const struct __kernel_timespec __user *, u_abs_timeout)
```

```c
static int do_mq_timedsend(mqd_t mqdes, const char __user *u_msg_ptr,
		size_t msg_len, unsigned int msg_prio,
		struct timespec64 *ts)
```

## mq_open

```c
SYSCALL_DEFINE4(mq_open, const char __user *, u_name, int, oflag, umode_t, mode,
		struct mq_attr __user *, u_attr)
{
	struct mq_attr attr;
	if (u_attr && copy_from_user(&attr, u_attr, sizeof(struct mq_attr)))
		return -EFAULT;

	return do_mq_open(u_name, oflag, mode, u_attr ? &attr : NULL);
  // 并没有什么神奇的地方
  // 利用了 vfs 的各种标准函数而已
}
```

## mq_getsetattr

# ipc/sem.c

## todo
1. 最后如何到达 architecture 的 ?
2. 注意，这种 sem 是实现进程之间进行同步的，显然在一个进程中间没有必要如此 ? (现在对于同一个进程的含义已经非常的迷惑了，当考虑内核是 thread process 的概念，只有task 的时候)
3. 为什么 namespace 的感觉只是简单的重新创建了一个空间，parent 对于 children 根本没有控制力可言 ?
1. 为什么需要创建出来一个 sem_array 来 ?
    1. 因为完成工作往往需要多个 sem 协作，这些 sem 属性相同

## doc && ref

Man semop(2)
> If sem_op is less than zero, the process must have alter permission on the semaphore set.  If semval is greater than or equal to the absolute value of sem_op, the operation can proceed immediately: the absolute value of sem_op is subtracted from semval, and, if SEM_UNDO is specified for this operation, the system adds the absolute value of sem_op to the semaphore adjustment (semadj) value for this semaphore.  If the absolute value of sem_op is greater than semval, and IPC_NOWAIT is specified in sem_flg, semop() fails, with errno set to EAGAIN (and none of the operations in sops is performed).  Otherwise, semncnt (the counter of threads waiting for this semaphore's value to increase) is incremented by one and the thread sleeps until one of the following occurs:

## semop
1. 似乎支持多个操作，如何保证这些操作的原子性 ?
2. 支持 SEM_UNDO 需要反向的给 semaphore adjustment 中间操作，然后呢 ?
3. 当 operation 的数量是一个的时候，那么如何使用 ?

```c
SYSCALL_DEFINE3(semop, int, semid, struct sembuf __user *, tsops,
		unsigned, nsops)
{
	return do_semtimedop(semid, tsops, nsops, NULL);
}

long ksys_semtimedop(int semid, struct sembuf __user *tsops,
		     unsigned int nsops, const struct __kernel_timespec __user *timeout)
{
	if (timeout) {
		struct timespec64 ts;
		if (get_timespec64(&ts, timeout))
			return -EFAULT;
		return do_semtimedop(semid, tsops, nsops, &ts);
	}
	return do_semtimedop(semid, tsops, nsops, NULL);
}
```


```c
/* One queue for each sleeping process in the system. */
struct sem_queue {
	struct list_head	list;	 /* queue of pending operations */
	struct task_struct	*sleeper; /* this process */
	struct sem_undo		*undo;	 /* undo structure */
	struct pid		*pid;	 /* process id of requesting process */
	int			status;	 /* completion status of operation */
	struct sembuf		*sops;	 /* array of pending operations */
	struct sembuf		*blocking; /* the operation that blocked */
	int			nsops;	 /* number of operations */
	bool			alter;	 /* does *sops alter the array? */
	bool                    dupsop;	 /* sops on more than one sem_num */
};
```


![](../../img/source/do_semtimedop.png)

1. 利用 sem_lock 对于一个组上锁
2. perform_atomic_semop 将每一个操作和 sem array 中间的内容操作

> 根据 perform_atomic_semop 的操作结果 : error == 0 表示 non-blocking succesfull path
> error > 0 表示 blocking succesfull path 将队列挂到其中
```c
	if (error == 0) { /* non-blocking succesfull path */
		DEFINE_WAKE_Q(wake_q);

		/*
		 * If the operation was successful, then do
		 * the required updates.
		 */
		if (alter)
			do_smart_update(sma, sops, nsops, 1, &wake_q);
		else
			set_semotime(sma, sops);

		sem_unlock(sma, locknum);
		rcu_read_unlock();
		wake_up_q(&wake_q);

		goto out_free;
	}
	if (error < 0) /* non-blocking error path */
		goto out_unlock_free;

	/*
	 * We need to sleep on this operation, so we put the current
	 * task into the pending queue and go to sleep.
	 */
	if (nsops == 1) {
		struct sem *curr;
		int idx = array_index_nospec(sops->sem_num, sma->sem_nsems);
		curr = &sma->sems[idx];

		if (alter) {
			if (sma->complex_count) {
				list_add_tail(&queue.list,
						&sma->pending_alter);
			} else {

				list_add_tail(&queue.list,
						&curr->pending_alter);
			}
		} else {
			list_add_tail(&queue.list, &curr->pending_const);
		}
	} else {
		if (!sma->complex_count)
			merge_queues(sma);

		if (alter)
			list_add_tail(&queue.list, &sma->pending_alter);
		else
			list_add_tail(&queue.list, &sma->pending_const);

		sma->complex_count++;
	}
  // 上面的部分将 queue 初始化了


	do {
  // non-blocking 的等待
	} while (error == -EINTR && !signal_pending(current)); /* spurious */

	unlink_queue(sma, &queue);

  // 各种释放
```



流程是什么: 混乱
1. sem_unlock sem_lock
2. merge_queues  unlink_queue



```c
do_semtimedop
update_queue
wake_const_ops
  perform_atomic_semop
    perform_atomic_semop_slow

update_queue
  unlink_queue
  do_smart_wake_zero
```
> @todo 上面的调用关系理清楚吧 !
> 但是现在我已经对于，sysv sem 没有什么兴趣了
> 至于其中到底如何优雅的实现 update_queue wake smart 之类的，以后存在疑惑的时候再说



## semget

```c
/**
 * newary - Create a new semaphore set
 * @ns: namespace
 * @params: ptr to the structure that contains key, semflg and nsems
 *
 * Called with sem_ids.rwsem held (as a writer)
 */
static int newary(struct ipc_namespace *ns, struct ipc_params *params)
{
```
1. @todo struct sem_array 和其持有的数组成员 struct sem ，为什么都持有 pending_const 和 pending_alter ?

## semctl semtiemop

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

## sysv 的各种东西都是在慢慢的清理掉
https://www.phoronix.com/news/Removing-SystemV-Filesystem

## kimi 的结果，似乎是科学的

### Card 1: SysV IPC 的三种机制及替代方案
**Q: SysV IPC 包含哪三种机制？各自的现代替代方案是什么？**

A:
| SysV 机制 | 替代方案 | 说明 |
|-----------|----------|------|
| 共享内存 (shmget) | memfd / shm_open | shm_open 在 /dev/shm/ 创建文件 |
| 消息队列 (msgsnd) | mq_open / Unix Domain Socket | POSIX 消息队列更灵活 |
| 信号量 (semop) | sem_open / futex | futex 性能更好 |

核心洞察：SysV IPC 正在被逐步淘汰，新项目应优先使用 POSIX 替代方案。

---

### Card 2: SysV IPC 核心数据结构
**Q: SysV IPC 的核心数据结构有哪些？它们之间的关系是什么？**

A:
1. **ipc_namespace**: 每个 namespace 管理三类 IPC (信号量、消息队列、共享内存)，通过 `ids[3]` 数组管理
2. **ipc_ids**: 管理每类 IPC 对象的 ID 分配，使用 IDR radix tree 关联 `kern_ipc_perm`
3. **kern_ipc_perm**: 所有 IPC 对象的权限基类，包含 key、uid/gid、mode 等
4. **具体结构**: sem_array、msg_queue、shmid_kernel 分别嵌入 kern_ipc_perm

关系图：`ipc_namespace → ipc_ids (idr) → kern_ipc_perm → 具体对象`

---

### Card 3: msg_queue 的设计
**Q: SysV 消息队列 msg_queue 如何管理消息和阻塞进程？**

A:
```c
struct msg_queue {
    struct kern_ipc_perm q_perm;  // 权限基类
    struct list_head q_messages;  // 消息链表 (msg_msg)
    struct list_head q_receivers; // 阻塞的接收者 (msg_receiver)
    struct list_head q_senders;   // 阻塞的发送者 (msg_sender)
    unsigned long q_qbytes;       // 队列最大字节数
};
```
关键设计：
- 消息通过 `msg_msg` 结构体存储，支持长消息分段 (msg_msgseg)
- 发送/接收阻塞时，将 task_struct 挂到对应队列
- 队列满时发送者阻塞，空时接收者阻塞

---

### Card 4: semop 的原子性与 SEM_UNDO
**Q: SysV 信号量操作如何保证原子性？SEM_UNDO 的作用是什么？**

A:
**原子性保证**：
- `semop()` 可指定多个操作 (struct sembuf 数组)
- 内核使用 `sem_lock` 锁定整个信号量数组
- `perform_atomic_semop` 尝试执行所有操作，要么全部成功，要么全部失败

**SEM_UNDO 机制**：
- 进程终止时自动恢复信号量值
- 内核维护 `sem_undo` 链表，记录每个进程对信号量的修改
- 进程退出时遍历 undo_list，反向执行调整

代码路径：`do_semtimedop → perform_atomic_semop → (blocking → sem_queue)`

---

### Card 5: SysV IPC 与 Namespace
**Q: SysV IPC 如何实现 Namespace 隔离？**

A:
```c
struct ipc_namespace {
    struct ipc_ids *ids[3];  // 0=信号量, 1=消息队列, 2=共享内存
    // 资源限制...
};
```
- 每个 IPC namespace 有独立的 ids 数组
- 通过 `current->nsproxy->ipc_ns` 获取当前 namespace
- 不同 namespace 中的相同 key 指向不同对象
- clone 时可以选择共享或隔离 IPC namespace (CLONE_NEWIPC)

---

### Card 6: SysV vs POSIX 消息队列
**Q: SysV 消息队列与 POSIX 消息队列 (mqueue) 的核心区别是什么？**

A:
| 特性 | SysV (msg.c) | POSIX (mqueue.c) |
|------|--------------|------------------|
| 标识方式 | 整数 ID (key → id) | 文件路径名 |
| 文件系统 | 虚拟文件系统，不可见 | 挂载在 /dev/mqueue |
| 描述符 | 内核内部 ID | 真正的文件描述符 |
| 功能扩展 | 固定功能 | 可利用 VFS 扩展 |
| 异步通知 | 不支持 | mq_notify 支持 sigevent |

核心洞察：POSIX mqueue 利用 VFS 实现，更贴近 Unix "一切皆文件" 哲学。


### 验证方法
<!-- 7f1f67e7-4b50-469a-ab89-062397de39fd -->

#### 1. 查看当前系统中的 SysV IPC 对象

```bash
# 查看所有 IPC 对象（共享内存、消息队列、信号量）
ipcs -a

# 仅查看共享内存
ipcs -m

# 仅查看消息队列
ipcs -q

# 仅查看信号量
ipcs -s

# 查看详细信息（包括创建者、权限等）
ipcs -m -i <shmid>    # 查看指定共享内存详情
ipcs -q -i <msqid>    # 查看指定消息队列详情
ipcs -s -i <semid>    # 查看指定信号量详情
```

#### 2. 创建测试 IPC 对象

```bash
# 创建共享内存（1024 字节），输出 shmid
ipcmk -M 1024

# 创建消息队列
ipcmk -Q

# 创建信号量（包含 3 个信号量）
ipcmk -S 3

# 创建时指定权限和 key
ipcmk -M 2048 -p 0644 -k 0x1234
```

#### 3. 删除 IPC 对象

```bash
# 删除共享内存
ipcrm -m <shmid>
# 或
ipcrm -M <shmkey>

# 删除消息队列
ipcrm -q <msqid>

# 删除信号量
ipcrm -s <semid>

# 通过 key 删除
ipcrm -S <semkey>
```

#### 4. 通过 /proc 查看 IPC 信息

```bash
# 查看 IPC 资源限制
cat /proc/sys/kernel/msgmax      # 单个消息最大字节数
cat /proc/sys/kernel/msgmnb      # 消息队列最大字节数
cat /proc/sys/kernel/msgmni      # 系统最大消息队列数
cat /proc/sys/kernel/shmmax      # 单个共享内存段最大字节数
cat /proc/sys/kernel/shmall      # 系统共享内存总页数
cat /proc/sys/kernel/sem         # 信号量限制 (SEMMSL, SEMMNS, SEMOPM, SEMMNI)

# 查看当前 IPC 对象详情
cat /proc/sysvipc/shm            # 共享内存详情
cat /proc/sysvipc/msg            # 消息队列详情
cat /proc/sysvipc/sem            # 信号量详情
```

#### 5. Namespace 隔离验证

```bash
# 查看当前 IPC namespace
ls -la /proc/self/ns/ipc

# 创建新的 IPC namespace 并测试隔离
sudo unshare --ipc /bin/bash

# 在新 namespace 中创建 IPC 对象
ipcmk -M 1024

# 在新 namespace 中查看（只能看到自己创建的）
ipcs -m

# 在宿主机查看（看不到新 namespace 中的对象）
# 从另一个终端执行:
ipcs -m

# 退出 namespace
exit
```

#### 6. 共享内存实际操作测试

```bash
# 终端 1: 创建共享内存并写入数据
shmid=$(ipcmk -M 1024 | grep -o '[0-9]\+')
echo "Created shmid: $shmid"

# 使用 ipcs 查看创建的信息
ipcs -m -i $shmid

# 终端 2: 读取共享内存（需要知道 shmid）
# 使用 C 程序或 python 附加到共享内存并读取

# 清理
ipcrm -m $shmid
```

#### 7. 消息队列实际操作测试

```bash
# 创建消息队列
msqid=$(ipcmk -Q | grep -o '[0-9]\+')
echo "Created msqid: $msqid"

# 使用 C 程序或 python 发送/接收消息
# 或使用以下命令行工具（需安装 sysvmsg-tools 或自行编译）

# 查看消息队列状态
ipcs -q -i $msqid

# 清理
ipcrm -q $msqid
```

#### 8. SysV IPC 限制验证

```bash
# 临时修改消息队列大小限制（需 root）
sudo sysctl kernel.msgmnb=65536

# 验证修改生效
sysctl kernel.msgmnb

# 查看当前所有 IPC 限制
ipcs -l

# 恢复默认值
sudo sysctl kernel.msgmnb=16384
```

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
