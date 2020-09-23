# ipc/namespace.c

## todo
1. kernel/utsname.c 同样的只有简单的 200 多行的样子，感觉完全就是对称的
    1. @todo  hostname domain , 为什么不和网络的放在一起呀 ?

2. 

## 瞎几把复制

```c
const struct proc_ns_operations ipcns_operations = {
	.name		= "ipc",
	.type		= CLONE_NEWIPC,
	.get		= ipcns_get,
	.put		= ipcns_put,
	.install	= ipcns_install,
	.owner		= ipcns_owner,
};
```

> 这是要注册到 proc 中间吗 ?




```c
struct ipc_namespace {
	refcount_t	count;
	struct ipc_ids	ids[3];

	int		sem_ctls[4];
	int		used_sems;

	unsigned int	msg_ctlmax;
	unsigned int	msg_ctlmnb;
	unsigned int	msg_ctlmni;
	atomic_t	msg_bytes;
	atomic_t	msg_hdrs;

	size_t		shm_ctlmax;
	size_t		shm_ctlall;
	unsigned long	shm_tot;
	int		shm_ctlmni;
	/*
	 * Defines whether IPC_RMID is forced for _all_ shm segments regardless
	 * of shmctl()
	 */
	int		shm_rmid_forced;

	struct notifier_block ipcns_nb;

	/* The kern_mount of the mqueuefs sb.  We take a ref on it */
	struct vfsmount	*mq_mnt;

	/* # queues in this ns, protected by mq_lock */
	unsigned int    mq_queues_count;

	/* next fields are set through sysctl */
	unsigned int    mq_queues_max;   /* initialized to DFLT_QUEUESMAX */
	unsigned int    mq_msg_max;      /* initialized to DFLT_MSGMAX */
	unsigned int    mq_msgsize_max;  /* initialized to DFLT_MSGSIZEMAX */
	unsigned int    mq_msg_default;
	unsigned int    mq_msgsize_default;

	/* user_ns which owns the ipc ns */
	struct user_namespace *user_ns;
	struct ucounts *ucounts;

	struct ns_common ns;
} __randomize_layout;
```


## CLONE_NEWIPC 的跟踪

```c
struct ipc_namespace *copy_ipcs(unsigned long flags,
	struct user_namespace *user_ns, struct ipc_namespace *ns)
{
	if (!(flags & CLONE_NEWIPC))
		return get_ipc_ns(ns);
	return create_ipc_ns(user_ns, ns); // todo 应该不是拷贝，而是创建出来层级关系吧 ?
}
```
> nsproxy.c:create_new_namespaces 唯一调用 !

```c

/*
 * Create new nsproxy and all of its the associated namespaces.
 * Return the newly created nsproxy.  Do not attach this to the task,
 * leave it to the caller to do proper locking and attach it to task.
 */
static struct nsproxy *create_new_namespaces(unsigned long flags,
	struct task_struct *tsk, struct user_namespace *user_ns,
	struct fs_struct *new_fs)
  ...
	new_nsp->ipc_ns = copy_ipcs(flags, user_ns, tsk->nsproxy->ipc_ns); // 将 tsk->nsproxy 的 ipc_ns 拿过来拷贝，根据 flags 自适应的处理
	if (IS_ERR(new_nsp->ipc_ns)) {
		err = PTR_ERR(new_nsp->ipc_ns);
		goto out_ipc;
	}
  ...
```
1. ref : copy_namespaces setns unshare (如 Man 中间所说)
2. `_do_fork` -> copy_process -> copy_namespaces

