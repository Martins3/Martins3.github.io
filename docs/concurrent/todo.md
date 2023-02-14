## [ ] atomic 指令是自动携带 memory barrier 的吗？

## [ ] x86 为什么又存在 lock prefix，又存在 cas 指令

## [ ] atomic 和 cas 似乎是两个东西啊

## 验证: 原子指令不是自带 memory barrier 的

因为原子指令的实现方法是:

## [ ] memory barrier 总是配对的吧

## qatomic_set 有意义吗？
set 本身就是 atomic 的

```c
/* Weak atomic operations prevent the compiler moving other
 * loads/stores past the atomic operation load/store. However there is
 * no explicit memory barrier for the processor.
 *
 * The C11 memory model says that variables that are accessed from
 * different threads should at least be done with __ATOMIC_RELAXED
 * primitives or the result is undefined. Generally this has little to
 * no effect on the generated code but not using the atomic primitives
 * will get flagged by sanitizers as a violation.
 */
#define qatomic_read__nocheck(ptr) \
    __atomic_load_n(ptr, __ATOMIC_RELAXED)
```

只是为了告诉 sanitizers 而已。

## 为什么 pause 指令可以拯救 memory order 啊？
- https://www.felixcloutier.com/x86/pause

## 看 smp_store_release 的展开最后就是 barrier

```c
# define barrier() __asm__ __volatile__("": : :"memory")
```
但是这条指令展开似乎没有没有指令，我们需要理解下

## 仔细阅读这个
- https://stackoverflow.com/questions/50323347/how-many-memory-barriers-instructions-does-an-x86-cpu-have

intel 指令的 ，lock 是自带 memory barrier 的

## 什么是 split lock

```diff
History:        #0
Commit:         6650cdd9a8ccf00555dbbe743d58541ad8feb6a7
Author:         Peter Zijlstra (Intel) <peterz@infradead.org>
Committer:      Borislav Petkov <bp@suse.de>
Author Date:    Mon 27 Jan 2020 04:05:35 AM CST
Committer Date: Fri 21 Feb 2020 04:17:53 AM CST

x86/split_lock: Enable split lock detection by kernel

A split-lock occurs when an atomic instruction operates on data that spans
two cache lines. In order to maintain atomicity the core takes a global bus
lock.

This is typically >1000 cycles slower than an atomic operation within a
cache line. It also disrupts performance on other cores (which must wait
for the bus lock to be released before their memory operations can
complete). For real-time systems this may mean missing deadlines. For other
systems it may just be very annoying.

Some CPUs have the capability to raise an #AC trap when a split lock is
attempted.

Provide a command line option to give the user choices on how to handle
this:

split_lock_detect=
	off	- not enabled (no traps for split locks)
	warn	- warn once when an application does a
		  split lock, but allow it to continue
		  running.
	fatal	- Send SIGBUS to applications that cause split lock

On systems that support split lock detection the default is "warn". Note
that if the kernel hits a split lock in any mode other than "off" it will
OOPs.

One implementation wrinkle is that the MSR to control the split lock
detection is per-core, not per thread. This might result in some short
lived races on HT systems in "warn" mode if Linux tries to enable on one
thread while disabling on the other. Race analysis by Sean Christopherson:

  - Toggling of split-lock is only done in "warn" mode.  Worst case
    scenario of a race is that a misbehaving task will generate multiple
    #AC exceptions on the same instruction.  And this race will only occur
    if both siblings are running tasks that generate split-lock #ACs, e.g.
    a race where sibling threads are writing different values will only
    occur if CPUx is disabling split-lock after an #AC and CPUy is
    re-enabling split-lock after *its* previous task generated an #AC.
  - Transitioning between off/warn/fatal modes at runtime isn't supported
    and disabling is tracked per task, so hardware will always reach a steady
    state that matches the configured mode.  I.e. split-lock is guaranteed to
    be enabled in hardware once all _TIF_SLD threads have been scheduled out.

Signed-off-by: Peter Zijlstra (Intel) <peterz@infradead.org>
Co-developed-by: Fenghua Yu <fenghua.yu@intel.com>
Signed-off-by: Fenghua Yu <fenghua.yu@intel.com>
Co-developed-by: Tony Luck <tony.luck@intel.com>
Signed-off-by: Tony Luck <tony.luck@intel.com>
Signed-off-by: Thomas Gleixner <tglx@linutronix.de>
Signed-off-by: Borislav Petkov <bp@suse.de>
Link: https://lore.kernel.org/r/20200126200535.GB30377@agluck-desk2.amr.corp.intel.com
```
## split lock
- https://lwn.net/Articles/790464/
- https://lwn.net/Articles/806466/
- https://www.sobyte.net/post/2022-05/split-locks/
