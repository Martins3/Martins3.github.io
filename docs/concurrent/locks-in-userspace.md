# 用户态的锁

似乎的确是没有什么黑科技的，分析下: musl/src/thread/，

其实还是 :
1. mutex
2. condition variable
3. spin lock
4. readwrite lock

```txt
 __lock.c
 __set_thread_area.c
 __syscall_cp.c
 __timedwait.c
 __tls_get_addr.c
 __unmapself.c
 __wait.c
 call_once.c
 clone.c
 cnd_broadcast.c
 cnd_destroy.c
 cnd_init.c
 cnd_signal.c
 cnd_timedwait.c
 cnd_wait.c
 default_attr.c
 lock_ptc.c
 mtx_destroy.c
 mtx_init.c
 mtx_lock.c
 mtx_timedlock.c
 mtx_trylock.c
 mtx_unlock.c
 pthread_atfork.c
 pthread_attr_destroy.c
 pthread_attr_get.c
 pthread_attr_init.c
 pthread_attr_setdetachstate.c
 pthread_attr_setguardsize.c
 pthread_attr_setinheritsched.c
 pthread_attr_setschedparam.c
 pthread_attr_setschedpolicy.c
 pthread_attr_setscope.c
 pthread_attr_setstack.c
 pthread_attr_setstacksize.c
 pthread_barrier_destroy.c
 pthread_barrier_init.c
 pthread_barrier_wait.c
 pthread_barrierattr_destroy.c
 pthread_barrierattr_init.c
 pthread_barrierattr_setpshared.c
 pthread_cancel.c
 pthread_cleanup_push.c
 pthread_cond_broadcast.c
 pthread_cond_destroy.c
 pthread_cond_init.c
 pthread_cond_signal.c
 pthread_cond_timedwait.c
 pthread_cond_wait.c
 pthread_condattr_destroy.c
 pthread_condattr_init.c
 pthread_condattr_setclock.c
 pthread_condattr_setpshared.c
 pthread_create.c
 pthread_detach.c
 pthread_equal.c
 pthread_getattr_np.c
 pthread_getconcurrency.c
 pthread_getcpuclockid.c
 pthread_getname_np.c
 pthread_getschedparam.c
 pthread_getspecific.c
 pthread_join.c
 pthread_key_create.c
 pthread_kill.c
 pthread_mutex_consistent.c
 pthread_mutex_destroy.c
 pthread_mutex_getprioceiling.c
 pthread_mutex_init.c
 pthread_mutex_lock.c
 pthread_mutex_setprioceiling.c
 pthread_mutex_timedlock.c
 pthread_mutex_trylock.c
 pthread_mutex_unlock.c
 pthread_mutexattr_destroy.c
 pthread_mutexattr_init.c
 pthread_mutexattr_setprotocol.c
 pthread_mutexattr_setpshared.c
 pthread_mutexattr_setrobust.c
 pthread_mutexattr_settype.c
 pthread_once.c
 pthread_rwlock_destroy.c
 pthread_rwlock_init.c
 pthread_rwlock_rdlock.c
 pthread_rwlock_timedrdlock.c
 pthread_rwlock_timedwrlock.c
 pthread_rwlock_tryrdlock.c
 pthread_rwlock_trywrlock.c
 pthread_rwlock_unlock.c
 pthread_rwlock_wrlock.c
 pthread_rwlockattr_destroy.c
 pthread_rwlockattr_init.c
 pthread_rwlockattr_setpshared.c
 pthread_self.c
 pthread_setattr_default_np.c
 pthread_setcancelstate.c
 pthread_setcanceltype.c
 pthread_setconcurrency.c
 pthread_setname_np.c
 pthread_setschedparam.c
 pthread_setschedprio.c
 pthread_setspecific.c
 pthread_sigmask.c
 pthread_spin_destroy.c
 pthread_spin_init.c
 pthread_spin_lock.c
 pthread_spin_trylock.c
 pthread_spin_unlock.c
 pthread_testcancel.c
 sem_destroy.c
 sem_getvalue.c
 sem_init.c
 sem_open.c
 sem_post.c
 sem_timedwait.c
 sem_trywait.c
 sem_unlink.c
 sem_wait.c
 synccall.c
 syscall_cp.c
 thrd_create.c
 thrd_exit.c
 thrd_join.c
 thrd_sleep.c
 thrd_yield.c
 tls.c
 tss_create.c
 tss_delete.c
 tss_set.c
 vmlock.c
```
## pthread 仔细分析
- pthread_barrier 和 memory model 的 barrier 没有关系
  - [When to use pthread condition variables?](https://stackoverflow.com/questions/20772476/when-to-use-pthread-condition-variables)
  - barrier 可以实现多个 thread 需要等待在同一个位置上

## 用户态 spin lock 的实现，超级简单

```c
int pthread_spin_lock(pthread_spinlock_t *s)
{
	while (*(volatile int *)s || a_cas(s, 0, EBUSY)) a_spin();
	return 0;
}

#define a_cas a_cas
static inline int a_cas(volatile int *p, int t, int s)
{
	__asm__ __volatile__ (
		"lock ; cmpxchg %3, %1"
		: "=a"(t), "=m"(*p) : "a"(t), "r"(s) : "memory" );
	return t;
}

#define a_spin a_spin
static inline void a_spin()
{
	__asm__ __volatile__( "pause" : : : "memory" );
}
```
- pause 指令就是专门为 spin lock 设计的
https://stackoverflow.com/questions/4725676/how-does-x86-pause-instruction-work-in-spinlock-and-can-it-be-used-in-other-sc

```txt
$ disass pthread_spin_lock
Dump of assembler code for function pthread_spin_lock:
   0x0000000000055f69 <+0>:     mov    $0x10,%edx
   0x0000000000055f6e <+5>:     mov    (%rdi),%eax
   0x0000000000055f70 <+7>:     test   %eax,%eax
   0x0000000000055f72 <+9>:     je     0x55f78 <pthread_spin_lock+15>
   0x0000000000055f74 <+11>:    pause
   0x0000000000055f76 <+13>:    jmp    0x55f6e <pthread_spin_lock+5>
   0x0000000000055f78 <+15>:    lock cmpxchg %edx,(%rdi)
   0x0000000000055f7c <+19>:    test   %eax,%eax
   0x0000000000055f7e <+21>:    jne    0x55f74 <pthread_spin_lock+11>
   0x0000000000055f80 <+23>:    ret
```
