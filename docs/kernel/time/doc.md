# Linux 内核时间子系统文档汇编

本文汇总 `linux-drm` 内核源码树中与 time 相关的文档，来源与版本如下：

- 源码树：`/home/martins3/data/kernel/linux-drm`
- 内核版本：Linux 7.2.0
- 提交：`eb5a10dc0e0026d8144f904b4b98b742a335a1d9`
- 生成日期：2026-08-25

目录：

1. [`Documentation/timers/index.rst`](#doc-1)
2. [`Documentation/timers/timekeeping.rst`](#doc-2)
3. [`Documentation/timers/highres.rst`](#doc-3)
4. [`Documentation/timers/hrtimers.rst`](#doc-4)
5. [`Documentation/timers/no_hz.rst`](#doc-5)
6. [`Documentation/timers/delay_sleep_functions.rst`](#doc-6)
7. [`Documentation/timers/hpet.rst`](#doc-7)
8. [`Documentation/core-api/timekeeping.rst`](#doc-8)
9. [`Documentation/virt/kvm/x86/timekeeping.rst`](#doc-9)
10. [`Documentation/virt/kvm/arm/ptp_kvm.rst`](#doc-10)
11. [`Documentation/virt/kvm/arm/pvtime.rst`](#doc-11)
12. [`Documentation/virt/kvm/devices/vcpu.rst （仅 KVM_VCPU_TSC_CTRL 组）`](#doc-12)
13. [`Documentation/ABI/stable/sysfs-kernel-time-aux-clocks`](#doc-13)

---

<a id="doc-1"></a>

## 1. `Documentation/timers/index.rst`

### Timers

本节包含以下文档：

- `highres`
- `hpet`
- `hrtimers`
- `no_hz`
- `timekeeping`
- `delay_sleep_functions`

---

<a id="doc-2"></a>

## 2. `Documentation/timers/timekeeping.rst`

### Clock sources, Clock events, sched_clock() and delay timers

This document tries to briefly explain some basic kernel timekeeping
abstractions. It partly pertains to the drivers usually found in
drivers/clocksource in the kernel tree, but the code may be spread out
across the kernel.

If you grep through the kernel source you will find a number of architecture-
specific implementations of clock sources, clockevents and several likewise
architecture-specific overrides of the sched_clock() function and some
delay timers.

To provide timekeeping for your platform, the clock source provides
the basic timeline, whereas clock events shoot interrupts on certain points
on this timeline, providing facilities such as high-resolution timers.
sched_clock() is used for scheduling and timestamping, and delay timers
provide an accurate delay source using hardware counters.

#### Clock sources

The purpose of the clock source is to provide a timeline for the system that
tells you where you are in time. For example issuing the command 'date' on
a Linux system will eventually read the clock source to determine exactly
what time it is.

Typically the clock source is a monotonic, atomic counter which will provide
n bits which count from 0 to (2^n)-1 and then wraps around to 0 and start over.
It will ideally NEVER stop ticking as long as the system is running. It
may stop during system suspend.

The clock source shall have as high resolution as possible, and the frequency
shall be as stable and correct as possible as compared to a real-world wall
clock. It should not move unpredictably back and forth in time or miss a few
cycles here and there.

It must be immune to the kind of effects that occur in hardware where e.g.
the counter register is read in two phases on the bus lowest 16 bits first
and the higher 16 bits in a second bus cycle with the counter bits
potentially being updated in between leading to the risk of very strange
values from the counter.

When the wall-clock accuracy of the clock source isn't satisfactory, there
are various quirks and layers in the timekeeping code for e.g. synchronizing
the user-visible time to RTC clocks in the system or against networked time
servers using NTP, but all they do basically is update an offset against
the clock source, which provides the fundamental timeline for the system.
These measures does not affect the clock source per se, they only adapt the
system to the shortcomings of it.

The clock source struct shall provide means to translate the provided counter
into a nanosecond value as an unsigned long long (unsigned 64 bit) number.
Since this operation may be invoked very often, doing this in a strict
mathematical sense is not desirable: instead the number is taken as close as
possible to a nanosecond value using only the arithmetic operations
multiply and shift, so in clocksource_cyc2ns() you find:

  ns ~= (clocksource * mult) >> shift

You will find a number of helper functions in the clock source code intended
to aid in providing these mult and shift values, such as
clocksource_khz2mult(), clocksource_hz2mult() that help determine the
mult factor from a fixed shift, and clocksource_register_hz() and
clocksource_register_khz() which will help out assigning both shift and mult
factors using the frequency of the clock source as the only input.

For real simple clock sources accessed from a single I/O memory location
there is nowadays even clocksource_mmio_init() which will take a memory
location, bit width, a parameter telling whether the counter in the
register counts up or down, and the timer clock rate, and then conjure all
necessary parameters.

Since a 32-bit counter at say 100 MHz will wrap around to zero after some 43
seconds, the code handling the clock source will have to compensate for this.
That is the reason why the clock source struct also contains a 'mask'
member telling how many bits of the source are valid. This way the timekeeping
code knows when the counter will wrap around and can insert the necessary
compensation code on both sides of the wrap point so that the system timeline
remains monotonic.

#### Clock events

Clock events are the conceptual reverse of clock sources: they take a
desired time specification value and calculate the values to poke into
hardware timer registers.

Clock events are orthogonal to clock sources. The same hardware
and register range may be used for the clock event, but it is essentially
a different thing. The hardware driving clock events has to be able to
fire interrupts, so as to trigger events on the system timeline. On an SMP
system, it is ideal (and customary) to have one such event driving timer per
CPU core, so that each core can trigger events independently of any other
core.

You will notice that the clock event device code is based on the same basic
idea about translating counters to nanoseconds using mult and shift
arithmetic, and you find the same family of helper functions again for
assigning these values. The clock event driver does not need a 'mask'
attribute however: the system will not try to plan events beyond the time
horizon of the clock event.

#### sched_clock()

In addition to the clock sources and clock events there is a special weak
function in the kernel called sched_clock(). This function shall return the
number of nanoseconds since the system was started. An architecture may or
may not provide an implementation of sched_clock() on its own. If a local
implementation is not provided, the system jiffy counter will be used as
sched_clock().

As the name suggests, sched_clock() is used for scheduling the system,
determining the absolute timeslice for a certain process in the CFS scheduler
for example. It is also used for printk timestamps when you have selected to
include time information in printk for things like bootcharts.

Compared to clock sources, sched_clock() has to be very fast: it is called
much more often, especially by the scheduler. If you have to do trade-offs
between accuracy compared to the clock source, you may sacrifice accuracy
for speed in sched_clock(). It however requires some of the same basic
characteristics as the clock source, i.e. it should be monotonic.

The sched_clock() function may wrap only on unsigned long long boundaries,
i.e. after 64 bits. Since this is a nanosecond value this will mean it wraps
after circa 585 years. (For most practical systems this means "never".)

If an architecture does not provide its own implementation of this function,
it will fall back to using jiffies, making its maximum resolution 1/HZ of the
jiffy frequency for the architecture. This will affect scheduling accuracy
and will likely show up in system benchmarks.

The clock driving sched_clock() may stop or reset to zero during system
suspend/sleep. This does not matter to the function it serves of scheduling
events on the system. However it may result in interesting timestamps in
printk().

The sched_clock() function should be callable in any context, IRQ- and
NMI-safe and return a sane value in any context.

Some architectures may have a limited set of time sources and lack a nice
counter to derive a 64-bit nanosecond value, so for example on the ARM
architecture, special helper functions have been created to provide a
sched_clock() nanosecond base from a 16- or 32-bit counter. Sometimes the
same counter that is also used as clock source is used for this purpose.

On SMP systems, it is crucial for performance that sched_clock() can be called
independently on each CPU without any synchronization performance hits.
Some hardware (such as the x86 TSC) will cause the sched_clock() function to
drift between the CPUs on the system. The kernel can work around this by
enabling the CONFIG_HAVE_UNSTABLE_SCHED_CLOCK option. This is another aspect
that makes sched_clock() different from the ordinary clock source.

#### Delay timers (some architectures only)

On systems with variable CPU frequency, the various kernel delay() functions
will sometimes behave strangely. Basically these delays usually use a hard
loop to delay a certain number of jiffy fractions using a "lpj" (loops per
jiffy) value, calibrated on boot.

Let's hope that your system is running on maximum frequency when this value
is calibrated: as an effect when the frequency is geared down to half the
full frequency, any delay() will be twice as long. Usually this does not
hurt, as you're commonly requesting that amount of delay *or more*. But
basically the semantics are quite unpredictable on such systems.

Enter timer-based delays. Using these, a timer read may be used instead of
a hard-coded loop for providing the desired delay.

This is done by declaring a struct delay_timer and assigning the appropriate
function pointers and rate settings for this delay timer.

This is available on some architectures like OpenRISC or ARM.

---

<a id="doc-3"></a>

## 3. `Documentation/timers/highres.rst`

### High resolution timers and dynamic ticks design notes

Further information can be found in the paper of the OLS 2006 talk "hrtimers
and beyond". The paper is part of the OLS 2006 Proceedings Volume 1, which can
be found on the OLS website:
https://www.kernel.org/doc/ols/2006/ols2006v1-pages-333-346.pdf

The slides to this talk are available from:
http://www.cs.columbia.edu/~nahum/w6998/papers/ols2006-hrtimers-slides.pdf

The slides contain five figures (pages 2, 15, 18, 20, 22), which illustrate the
changes in the time(r) related Linux subsystems. Figure #1 (p. 2) shows the
design of the Linux time(r) system before hrtimers and other building blocks
got merged into mainline.

Note: the paper and the slides are talking about "clock event source", while we
switched to the name "clock event devices" in meantime.

The design contains the following basic building blocks:

- hrtimer base infrastructure
- timeofday and clock source management
- clock event management
- high resolution timer functionality
- dynamic ticks

#### hrtimer base infrastructure

The hrtimer base infrastructure was merged into the 2.6.16 kernel. Details of
the base implementation are covered in Documentation/timers/hrtimers.rst. See
also figure #2 (OLS slides p. 15)

The main differences to the timer wheel, which holds the armed timer_list type
timers are:

       - time ordered enqueueing into a rb-tree
       - independent of ticks (the processing is based on nanoseconds)

#### timeofday and clock source management

John Stultz's Generic Time Of Day (GTOD) framework moves a large portion of
code out of the architecture-specific areas into a generic management
framework, as illustrated in figure #3 (OLS slides p. 18). The architecture
specific portion is reduced to the low level hardware details of the clock
sources, which are registered in the framework and selected on a quality based
decision. The low level code provides hardware setup and readout routines and
initializes data structures, which are used by the generic time keeping code to
convert the clock ticks to nanosecond based time values. All other time keeping
related functionality is moved into the generic code. The GTOD base patch got
merged into the 2.6.18 kernel.

Further information about the Generic Time Of Day framework is available in the
OLS 2005 Proceedings Volume 1:

	http://www.linuxsymposium.org/2005/linuxsymposium_procv1.pdf

The paper "We Are Not Getting Any Younger: A New Approach to Time and
Timers" was written by J. Stultz, D.V. Hart, & N. Aravamudan.

Figure #3 (OLS slides p.18) illustrates the transformation.

#### clock event management

While clock sources provide read access to the monotonically increasing time
value, clock event devices are used to schedule the next event
interrupt(s). The next event is currently defined to be periodic, with its
period defined at compile time. The setup and selection of the event device
for various event driven functionalities is hardwired into the architecture
dependent code. This results in duplicated code across all architectures and
makes it extremely difficult to change the configuration of the system to use
event interrupt devices other than those already built into the
architecture. Another implication of the current design is that it is necessary
to touch all the architecture-specific implementations in order to provide new
functionality like high resolution timers or dynamic ticks.

The clock events subsystem tries to address this problem by providing a generic
solution to manage clock event devices and their usage for the various clock
event driven kernel functionalities. The goal of the clock event subsystem is
to minimize the clock event related architecture dependent code to the pure
hardware related handling and to allow easy addition and utilization of new
clock event devices. It also minimizes the duplicated code across the
architectures as it provides generic functionality down to the interrupt
service handler, which is almost inherently hardware dependent.

Clock event devices are registered either by the architecture dependent boot
code or at module insertion time. Each clock event device fills a data
structure with clock-specific property parameters and callback functions. The
clock event management decides, by using the specified property parameters, the
set of system functions a clock event device will be used to support. This
includes the distinction of per-CPU and per-system global event devices.

System-level global event devices are used for the Linux periodic tick. Per-CPU
event devices are used to provide local CPU functionality such as process
accounting, profiling, and high resolution timers.

The management layer assigns one or more of the following functions to a clock
event device:

      - system global periodic tick (jiffies update)
      - cpu local update_process_times
      - cpu local profiling
      - cpu local next event interrupt (non periodic mode)

The clock event device delegates the selection of those timer interrupt related
functions completely to the management layer. The clock management layer stores
a function pointer in the device description structure, which has to be called
from the hardware level handler. This removes a lot of duplicated code from the
architecture specific timer interrupt handlers and hands the control over the
clock event devices and the assignment of timer interrupt related functionality
to the core code.

The clock event layer API is rather small. Aside from the clock event device
registration interface it provides functions to schedule the next event
interrupt, clock event device notification service and support for suspend and
resume.

The framework adds about 700 lines of code which results in a 2KB increase of
the kernel binary size. The conversion of i386 removes about 100 lines of
code. The binary size decrease is in the range of 400 byte. We believe that the
increase of flexibility and the avoidance of duplicated code across
architectures justifies the slight increase of the binary size.

The conversion of an architecture has no functional impact, but allows to
utilize the high resolution and dynamic tick functionalities without any change
to the clock event device and timer interrupt code. After the conversion the
enabling of high resolution timers and dynamic ticks is simply provided by
adding the kernel/time/Kconfig file to the architecture specific Kconfig and
adding the dynamic tick specific calls to the idle routine (a total of 3 lines
added to the idle function and the Kconfig file)

Figure #4 (OLS slides p.20) illustrates the transformation.

#### high resolution timer functionality

During system boot it is not possible to use the high resolution timer
functionality, while making it possible would be difficult and would serve no
useful function. The initialization of the clock event device framework, the
clock source framework (GTOD) and hrtimers itself has to be done and
appropriate clock sources and clock event devices have to be registered before
the high resolution functionality can work. Up to the point where hrtimers are
initialized, the system works in the usual low resolution periodic mode. The
clock source and the clock event device layers provide notification functions
which inform hrtimers about availability of new hardware. hrtimers validates
the usability of the registered clock sources and clock event devices before
switching to high resolution mode. This ensures also that a kernel which is
configured for high resolution timers can run on a system which lacks the
necessary hardware support.

The high resolution timer code does not support SMP machines which have only
global clock event devices. The support of such hardware would involve IPI
calls when an interrupt happens. The overhead would be much larger than the
benefit. This is the reason why we currently disable high resolution and
dynamic ticks on i386 SMP systems which stop the local APIC in C3 power
state. A workaround is available as an idea, but the problem has not been
tackled yet.

The time ordered insertion of timers provides all the infrastructure to decide
whether the event device has to be reprogrammed when a timer is added. The
decision is made per timer base and synchronized across per-cpu timer bases in
a support function. The design allows the system to utilize separate per-CPU
clock event devices for the per-CPU timer bases, but currently only one
reprogrammable clock event device per-CPU is utilized.

When the timer interrupt happens, the next event interrupt handler is called
from the clock event distribution code and moves expired timers from the
red-black tree to a separate double linked list and invokes the softirq
handler. An additional mode field in the hrtimer structure allows the system to
execute callback functions directly from the next event interrupt handler. This
is restricted to code which can safely be executed in the hard interrupt
context. This applies, for example, to the common case of a wakeup function as
used by nanosleep. The advantage of executing the handler in the interrupt
context is the avoidance of up to two context switches - from the interrupted
context to the softirq and to the task which is woken up by the expired
timer.

Once a system has switched to high resolution mode, the periodic tick is
switched off. This disables the per system global periodic clock event device -
e.g. the PIT on i386 SMP systems.

The periodic tick functionality is provided by an per-cpu hrtimer. The callback
function is executed in the next event interrupt context and updates jiffies
and calls update_process_times and profiling. The implementation of the hrtimer
based periodic tick is designed to be extended with dynamic tick functionality.
This allows to use a single clock event device to schedule high resolution
timer and periodic events (jiffies tick, profiling, process accounting) on UP
systems. This has been proved to work with the PIT on i386 and the Incrementer
on PPC.

The softirq for running the hrtimer queues and executing the callbacks has been
separated from the tick bound timer softirq to allow accurate delivery of high
resolution timer signals which are used by itimer and POSIX interval
timers. The execution of this softirq can still be delayed by other softirqs,
but the overall latencies have been significantly improved by this separation.

Figure #5 (OLS slides p.22) illustrates the transformation.

#### dynamic ticks

Dynamic ticks are the logical consequence of the hrtimer based periodic tick
replacement (sched_tick). The functionality of the sched_tick hrtimer is
extended by three functions:

- hrtimer_stop_sched_tick
- hrtimer_restart_sched_tick
- hrtimer_update_jiffies

hrtimer_stop_sched_tick() is called when a CPU goes into idle state. The code
evaluates the next scheduled timer event (from both hrtimers and the timer
wheel) and in case that the next event is further away than the next tick it
reprograms the sched_tick to this future event, to allow longer idle sleeps
without worthless interruption by the periodic tick. The function is also
called when an interrupt happens during the idle period, which does not cause a
reschedule. The call is necessary as the interrupt handler might have armed a
new timer whose expiry time is before the time which was identified as the
nearest event in the previous call to hrtimer_stop_sched_tick.

hrtimer_restart_sched_tick() is called when the CPU leaves the idle state before
it calls schedule(). hrtimer_restart_sched_tick() resumes the periodic tick,
which is kept active until the next call to hrtimer_stop_sched_tick().

hrtimer_update_jiffies() is called from irq_enter() when an interrupt happens
in the idle period to make sure that jiffies are up to date and the interrupt
handler has not to deal with an eventually stale jiffy value.

The dynamic tick feature provides statistical values which are exported to
userspace via /proc/stat and can be made available for enhanced power
management control.

The implementation leaves room for further development like full tickless
systems, where the time slice is controlled by the scheduler, variable
frequency profiling, and a complete removal of jiffies in the future.

Aside the current initial submission of i386 support, the patchset has been
extended to x86_64 and ARM already. Initial (work in progress) support is also
available for MIPS and PowerPC.

	  Thomas, Ingo

---

<a id="doc-4"></a>

## 4. `Documentation/timers/hrtimers.rst`

### hrtimers - subsystem for high-resolution kernel timers

This patch introduces a new subsystem for high-resolution kernel timers.

One might ask the question: we already have a timer subsystem
(kernel/timers.c), why do we need two timer subsystems? After a lot of
back and forth trying to integrate high-resolution and high-precision
features into the existing timer framework, and after testing various
such high-resolution timer implementations in practice, we came to the
conclusion that the timer wheel code is fundamentally not suitable for
such an approach. We initially didn't believe this ('there must be a way
to solve this'), and spent a considerable effort trying to integrate
things into the timer wheel, but we failed. In hindsight, there are
several reasons why such integration is hard/impossible:

- the forced handling of low-resolution and high-resolution timers in
  the same way leads to a lot of compromises, macro magic and #ifdef
  mess. The timers.c code is very "tightly coded" around jiffies and
  32-bitness assumptions, and has been honed and micro-optimized for a
  relatively narrow use case (jiffies in a relatively narrow HZ range)
  for many years - and thus even small extensions to it easily break
  the wheel concept, leading to even worse compromises. The timer wheel
  code is very good and tight code, there's zero problems with it in its
  current usage - but it is simply not suitable to be extended for
  high-res timers.

- the unpredictable [O(N)] overhead of cascading leads to delays which
  necessitate a more complex handling of high resolution timers, which
  in turn decreases robustness. Such a design still leads to rather large
  timing inaccuracies. Cascading is a fundamental property of the timer
  wheel concept, it cannot be 'designed out' without inevitably
  degrading other portions of the timers.c code in an unacceptable way.

- the implementation of the current posix-timer subsystem on top of
  the timer wheel has already introduced a quite complex handling of
  the required readjusting of absolute CLOCK_REALTIME timers at
  settimeofday or NTP time - further underlying our experience by
  example: that the timer wheel data structure is too rigid for high-res
  timers.

- the timer wheel code is most optimal for use cases which can be
  identified as "timeouts". Such timeouts are usually set up to cover
  error conditions in various I/O paths, such as networking and block
  I/O. The vast majority of those timers never expire and are rarely
  recascaded because the expected correct event arrives in time so they
  can be removed from the timer wheel before any further processing of
  them becomes necessary. Thus the users of these timeouts can accept
  the granularity and precision tradeoffs of the timer wheel, and
  largely expect the timer subsystem to have near-zero overhead.
  Accurate timing for them is not a core purpose - in fact most of the
  timeout values used are ad-hoc. For them it is at most a necessary
  evil to guarantee the processing of actual timeout completions
  (because most of the timeouts are deleted before completion), which
  should thus be as cheap and unintrusive as possible.

The primary users of precision timers are user-space applications that
utilize nanosleep, posix-timers and itimer interfaces. Also, in-kernel
users like drivers and subsystems which require precise timed events
(e.g. multimedia) can benefit from the availability of a separate
high-resolution timer subsystem as well.

While this subsystem does not offer high-resolution clock sources just
yet, the hrtimer subsystem can be easily extended with high-resolution
clock capabilities, and patches for that exist and are maturing quickly.
The increasing demand for realtime and multimedia applications along
with other potential users for precise timers gives another reason to
separate the "timeout" and "precise timer" subsystems.

Another potential benefit is that such a separation allows even more
special-purpose optimization of the existing timer wheel for the low
resolution and low precision use cases - once the precision-sensitive
APIs are separated from the timer wheel and are migrated over to
hrtimers. E.g. we could decrease the frequency of the timeout subsystem
from 250 Hz to 100 HZ (or even smaller).

#### hrtimer subsystem implementation details

the basic design considerations were:

- simplicity

- data structure not bound to jiffies or any other granularity. All the
  kernel logic works at 64-bit nanoseconds resolution - no compromises.

- simplification of existing, timing related kernel code

another basic requirement was the immediate enqueueing and ordering of
timers at activation time. After looking at several possible solutions
such as radix trees and hashes, we chose the red black tree as the basic
data structure. Rbtrees are available as a library in the kernel and are
used in various performance-critical areas of e.g. memory management and
file systems. The rbtree is solely used for time sorted ordering, while
a separate list is used to give the expiry code fast access to the
queued timers, without having to walk the rbtree.

(This separate list is also useful for later when we'll introduce
high-resolution clocks, where we need separate pending and expired
queues while keeping the time-order intact.)

Time-ordered enqueueing is not purely for the purposes of
high-resolution clocks though, it also simplifies the handling of
absolute timers based on a low-resolution CLOCK_REALTIME. The existing
implementation needed to keep an extra list of all armed absolute
CLOCK_REALTIME timers along with complex locking. In case of
settimeofday and NTP, all the timers (!) had to be dequeued, the
time-changing code had to fix them up one by one, and all of them had to
be enqueued again. The time-ordered enqueueing and the storage of the
expiry time in absolute time units removes all this complex and poorly
scaling code from the posix-timer implementation - the clock can simply
be set without having to touch the rbtree. This also makes the handling
of posix-timers simpler in general.

The locking and per-CPU behavior of hrtimers was mostly taken from the
existing timer wheel code, as it is mature and well suited. Sharing code
was not really a win, due to the different data structures. Also, the
hrtimer functions now have clearer behavior and clearer names - such as
hrtimer_try_to_cancel() and hrtimer_cancel() [which are roughly
equivalent to timer_delete() and timer_delete_sync()] - so there's no direct
1:1 mapping between them on the algorithmic level, and thus no real
potential for code sharing either.

Basic data types: every time value, absolute or relative, is in a
special nanosecond-resolution 64bit type: ktime_t.
(Originally, the kernel-internal representation of ktime_t values and
operations was implemented via macros and inline functions, and could be
switched between a "hybrid union" type and a plain "scalar" 64bit
nanoseconds representation (at compile time). This was abandoned in the
context of the Y2038 work.)

#### hrtimers - rounding of timer values

the hrtimer code will round timer events to lower-resolution clocks
because it has to. Otherwise it will do no artificial rounding at all.

one question is, what resolution value should be returned to the user by
the clock_getres() interface. This will return whatever real resolution
a given clock has - be it low-res, high-res, or artificially-low-res.

#### hrtimers - testing and verification

We used the high-resolution clock subsystem on top of hrtimers to verify
the hrtimer implementation details in praxis, and we also ran the posix
timer tests in order to ensure specification compliance. We also ran
tests on low-resolution clocks.

The hrtimer patch converts the following kernel functionality to use
hrtimers:

 - nanosleep
 - itimers
 - posix-timers

The conversion of nanosleep and posix-timers enabled the unification of
nanosleep and clock_nanosleep.

The code was successfully compiled for the following platforms:

 i386, x86_64, ARM, PPC, PPC64, IA64

The code was run-tested on the following platforms:

 i386(UP/SMP), x86_64(UP/SMP), ARM, PPC

hrtimers were also integrated into the -rt tree, along with a
hrtimers-based high-resolution clock implementation, so the hrtimers
code got a healthy amount of testing and use in practice.

	Thomas Gleixner, Ingo Molnar

---

<a id="doc-5"></a>

## 5. `Documentation/timers/no_hz.rst`

### NO_HZ: Reducing Scheduling-Clock Ticks

This document describes Kconfig options and boot parameters that can
reduce the number of scheduling-clock interrupts, thereby improving energy
efficiency and reducing OS jitter.  Reducing OS jitter is important for
some types of computationally intensive high-performance computing (HPC)
applications and for real-time applications.

There are three main ways of managing scheduling-clock interrupts
(also known as "scheduling-clock ticks" or simply "ticks"):

1.	Never omit scheduling-clock ticks (CONFIG_HZ_PERIODIC=y or
	CONFIG_NO_HZ=n for older kernels).  You normally will -not-
	want to choose this option.

2.	Omit scheduling-clock ticks on idle CPUs (CONFIG_NO_HZ_IDLE=y or
	CONFIG_NO_HZ=y for older kernels).  This is the most common
	approach, and should be the default.

3.	Omit scheduling-clock ticks on CPUs that are either idle or that
	have only one runnable task (CONFIG_NO_HZ_FULL=y).  Unless you
	are running realtime applications or certain types of HPC
	workloads, you will normally -not- want this option.

These three cases are described in the following three sections, followed
by a third section on RCU-specific considerations, a fourth section
discussing testing, and a fifth and final section listing known issues.

#### Never Omit Scheduling-Clock Ticks

Very old versions of Linux from the 1990s and the very early 2000s
are incapable of omitting scheduling-clock ticks.  It turns out that
there are some situations where this old-school approach is still the
right approach, for example, in heavy workloads with lots of tasks
that use short bursts of CPU, where there are very frequent idle
periods, but where these idle periods are also quite short (tens or
hundreds of microseconds).  For these types of workloads, scheduling
clock interrupts will normally be delivered any way because there
will frequently be multiple runnable tasks per CPU.  In these cases,
attempting to turn off the scheduling clock interrupt will have no effect
other than increasing the overhead of switching to and from idle and
transitioning between user and kernel execution.

This mode of operation can be selected using CONFIG_HZ_PERIODIC=y (or
CONFIG_NO_HZ=n for older kernels).

However, if you are instead running a light workload with long idle
periods, failing to omit scheduling-clock interrupts will result in
excessive power consumption.  This is especially bad on battery-powered
devices, where it results in extremely short battery lifetimes.  If you
are running light workloads, you should therefore read the following
section.

In addition, if you are running either a real-time workload or an HPC
workload with short iterations, the scheduling-clock interrupts can
degrade your applications performance.  If this describes your workload,
you should read the following two sections.

#### Omit Scheduling-Clock Ticks For Idle CPUs

If a CPU is idle, there is little point in sending it a scheduling-clock
interrupt.  After all, the primary purpose of a scheduling-clock interrupt
is to force a busy CPU to shift its attention among multiple duties,
and an idle CPU has no duties to shift its attention among.

An idle CPU that is not receiving scheduling-clock interrupts is said to
be "dyntick-idle", "in dyntick-idle mode", "in nohz mode", or "running
tickless".  The remainder of this document will use "dyntick-idle mode".

The CONFIG_NO_HZ_IDLE=y Kconfig option causes the kernel to avoid sending
scheduling-clock interrupts to idle CPUs, which is critically important
both to battery-powered devices and to highly virtualized mainframes.
A battery-powered device running a CONFIG_HZ_PERIODIC=y kernel would
drain its battery very quickly, easily 2-3 times as fast as would the
same device running a CONFIG_NO_HZ_IDLE=y kernel.  A mainframe running
1,500 OS instances might find that half of its CPU time was consumed by
unnecessary scheduling-clock interrupts.  In these situations, there
is strong motivation to avoid sending scheduling-clock interrupts to
idle CPUs.  That said, dyntick-idle mode is not free:

1.	It increases the number of instructions executed on the path
	to and from the idle loop.

2.	On many architectures, dyntick-idle mode also increases the
	number of expensive clock-reprogramming operations.

Therefore, systems with aggressive real-time response constraints often
run CONFIG_HZ_PERIODIC=y kernels (or CONFIG_NO_HZ=n for older kernels)
in order to avoid degrading from-idle transition latencies.

There is also a boot parameter "nohz=" that can be used to disable
dyntick-idle mode in CONFIG_NO_HZ_IDLE=y kernels by specifying "nohz=off".
By default, CONFIG_NO_HZ_IDLE=y kernels boot with "nohz=on", enabling
dyntick-idle mode.

#### Omit Scheduling-Clock Ticks For CPUs With Only One Runnable Task

If a CPU has only one runnable task, there is little point in sending it
a scheduling-clock interrupt because there is no other task to switch to.
Note that omitting scheduling-clock ticks for CPUs with only one runnable
task implies also omitting them for idle CPUs.

The CONFIG_NO_HZ_FULL=y Kconfig option causes the kernel to avoid
sending scheduling-clock interrupts to CPUs with a single runnable task,
and such CPUs are said to be "adaptive-ticks CPUs".  This is important
for applications with aggressive real-time response constraints because
it allows them to improve their worst-case response times by the maximum
duration of a scheduling-clock interrupt.  It is also important for
computationally intensive short-iteration workloads:  If any CPU is
delayed during a given iteration, all the other CPUs will be forced to
wait idle while the delayed CPU finishes.  Thus, the delay is multiplied
by one less than the number of CPUs.  In these situations, there is
again strong motivation to avoid sending scheduling-clock interrupts.

By default, no CPU will be an adaptive-ticks CPU.  The "nohz_full="
boot parameter specifies the adaptive-ticks CPUs.  For example,
"nohz_full=1,6-8" says that CPUs 1, 6, 7, and 8 are to be adaptive-ticks
CPUs.  Note that you are prohibited from marking all of the CPUs as
adaptive-tick CPUs:  At least one non-adaptive-tick CPU must remain
online to handle timekeeping tasks in order to ensure that system
calls like gettimeofday() returns accurate values on adaptive-tick CPUs.
(This is not an issue for CONFIG_NO_HZ_IDLE=y because there are no running
user processes to observe slight drifts in clock rate.) Note that this
means that your system must have at least two CPUs in order for
CONFIG_NO_HZ_FULL=y to do anything for you.

Finally, adaptive-ticks CPUs must have their RCU callbacks offloaded.
This is covered in the "RCU IMPLICATIONS" section below.

Normally, a CPU remains in adaptive-ticks mode as long as possible.
In particular, transitioning to kernel mode does not automatically change
the mode.  Instead, the CPU will exit adaptive-ticks mode only if needed,
for example, if that CPU enqueues an RCU callback.

Just as with dyntick-idle mode, the benefits of adaptive-tick mode do
not come for free:

1.	CONFIG_NO_HZ_FULL selects CONFIG_NO_HZ_COMMON, so you cannot run
	adaptive ticks without also running dyntick idle.  This dependency
	extends down into the implementation, so that all of the costs
	of CONFIG_NO_HZ_IDLE are also incurred by CONFIG_NO_HZ_FULL.

2.	The user/kernel transitions are slightly more expensive due
	to the need to inform kernel subsystems (such as RCU) about
	the change in mode.

3.	POSIX CPU timers prevent CPUs from entering adaptive-tick mode.
	Real-time applications needing to take actions based on CPU time
	consumption need to use other means of doing so.

4.	If there are more perf events pending than the hardware can
	accommodate, they are normally round-robined so as to collect
	all of them over time.  Adaptive-tick mode may prevent this
	round-robining from happening.  This will likely be fixed by
	preventing CPUs with large numbers of perf events pending from
	entering adaptive-tick mode.

5.	Scheduler statistics for adaptive-tick CPUs may be computed
	slightly differently than those for non-adaptive-tick CPUs.
	This might in turn perturb load-balancing of real-time tasks.

Although improvements are expected over time, adaptive ticks is quite
useful for many types of real-time and compute-intensive applications.
However, the drawbacks listed above mean that adaptive ticks should not
(yet) be enabled by default.

#### RCU Implications

There are situations in which idle CPUs cannot be permitted to
enter either dyntick-idle mode or adaptive-tick mode, the most
common being when that CPU has RCU callbacks pending.

Avoid this by offloading RCU callback processing to "rcuo" kthreads
using the CONFIG_RCU_NOCB_CPU=y Kconfig option.  The specific CPUs to
offload may be selected using The "rcu_nocbs=" kernel boot parameter,
which takes a comma-separated list of CPUs and CPU ranges, for example,
"1,3-5" selects CPUs 1, 3, 4, and 5.  Note that CPUs specified by
the "nohz_full" kernel boot parameter are also offloaded.

The offloaded CPUs will never queue RCU callbacks, and therefore RCU
never prevents offloaded CPUs from entering either dyntick-idle mode
or adaptive-tick mode.  That said, note that it is up to userspace to
pin the "rcuo" kthreads to specific CPUs if desired.  Otherwise, the
scheduler will decide where to run them, which might or might not be
where you want them to run.

#### Testing

So you enable all the OS-jitter features described in this document,
but do not see any change in your workload's behavior.  Is this because
your workload isn't affected that much by OS jitter, or is it because
something else is in the way?  This section helps answer this question
by providing a simple OS-jitter test suite, which is available on branch
master of the following git archive:

git://git.kernel.org/pub/scm/linux/kernel/git/frederic/dynticks-testing.git

Clone this archive and follow the instructions in the README file.
This test procedure will produce a trace that will allow you to evaluate
whether or not you have succeeded in removing OS jitter from your system.
If this trace shows that you have removed OS jitter as much as is
possible, then you can conclude that your workload is not all that
sensitive to OS jitter.

Note: this test requires that your system have at least two CPUs.
We do not currently have a good way to remove OS jitter from single-CPU
systems.

#### Known Issues

*	Dyntick-idle slows transitions to and from idle slightly.
	In practice, this has not been a problem except for the most
	aggressive real-time workloads, which have the option of disabling
	dyntick-idle mode, an option that most of them take.  However,
	some workloads will no doubt want to use adaptive ticks to
	eliminate scheduling-clock interrupt latencies.  Here are some
	options for these workloads:

	a.	Use PMQOS from userspace to inform the kernel of your
		latency requirements (preferred).

	b.	On x86 systems, use the "idle=mwait" boot parameter.

	c.	On x86 systems, use the "intel_idle.max_cstate=" to limit
	`	the maximum C-state depth.

	d.	On x86 systems, use the "idle=poll" boot parameter.
		However, please note that use of this parameter can cause
		your CPU to overheat, which may cause thermal throttling
		to degrade your latencies -- and that this degradation can
		be even worse than that of dyntick-idle.  Furthermore,
		this parameter effectively disables Turbo Mode on Intel
		CPUs, which can significantly reduce maximum performance.

*	Adaptive-ticks slows user/kernel transitions slightly.
	This is not expected to be a problem for computationally intensive
	workloads, which have few such transitions.  Careful benchmarking
	will be required to determine whether or not other workloads
	are significantly affected by this effect.

*	Adaptive-ticks does not do anything unless there is only one
	runnable task for a given CPU, even though there are a number
	of other situations where the scheduling-clock tick is not
	needed.  To give but one example, consider a CPU that has one
	runnable high-priority SCHED_FIFO task and an arbitrary number
	of low-priority SCHED_OTHER tasks.  In this case, the CPU is
	required to run the SCHED_FIFO task until it either blocks or
	some other higher-priority task awakens on (or is assigned to)
	this CPU, so there is no point in sending a scheduling-clock
	interrupt to this CPU.	However, the current implementation
	nevertheless sends scheduling-clock interrupts to CPUs having a
	single runnable SCHED_FIFO task and multiple runnable SCHED_OTHER
	tasks, even though these interrupts are unnecessary.

	And even when there are multiple runnable tasks on a given CPU,
	there is little point in interrupting that CPU until the current
	running task's timeslice expires, which is almost always way
	longer than the time of the next scheduling-clock interrupt.

	Better handling of these sorts of situations is future work.

*	A reboot is required to reconfigure both adaptive idle and RCU
	callback offloading.  Runtime reconfiguration could be provided
	if needed, however, due to the complexity of reconfiguring RCU at
	runtime, there would need to be an earthshakingly good reason.
	Especially given that you have the straightforward option of
	simply offloading RCU callbacks from all CPUs and pinning them
	where you want them whenever you want them pinned.

*	Additional configuration is required to deal with other sources
	of OS jitter, including interrupts and system-utility tasks
	and processes.  This configuration normally involves binding
	interrupts and tasks to particular CPUs.

*	Some sources of OS jitter can currently be eliminated only by
	constraining the workload.  For example, the only way to eliminate
	OS jitter due to global TLB shootdowns is to avoid the unmapping
	operations (such as kernel module unload operations) that
	result in these shootdowns.  For another example, page faults
	and TLB misses can be reduced (and in some cases eliminated) by
	using huge pages and by constraining the amount of memory used
	by the application.  Pre-faulting the working set can also be
	helpful, especially when combined with the mlock() and mlockall()
	system calls.

*	Unless all CPUs are idle, at least one CPU must keep the
	scheduling-clock interrupt going in order to support accurate
	timekeeping.

*	If there might potentially be some adaptive-ticks CPUs, there
	will be at least one CPU keeping the scheduling-clock interrupt
	going, even if all CPUs are otherwise idle.

	Better handling of this situation is ongoing work.

*	Some process-handling operations still require the occasional
	scheduling-clock tick.	These operations include calculating CPU
	load, maintaining sched average, computing CFS entity vruntime,
	computing avenrun, and carrying out load balancing.  They are
	currently accommodated by scheduling-clock tick every second
	or so.	On-going work will eliminate the need even for these
	infrequent scheduling-clock ticks.

---

<a id="doc-6"></a>

## 6. `Documentation/timers/delay_sleep_functions.rst`

### Delay and sleep mechanisms

This document seeks to answer the common question: "What is the
RightWay (TM) to insert a delay?"

This question is most often faced by driver writers who have to
deal with hardware delays and who may not be the most intimately
familiar with the inner workings of the Linux Kernel.

The following table gives a rough overview about the existing function
'families' and their limitations. This overview table does not replace the
reading of the function description before usage!

| Property | `*delay()` | `usleep_range*()` | `*sleep()` | `fsleep()` |
|---|---|---|---|---|
|  | busy-wait loop | hrtimers based | timer list timers based | combines the others |
| Usage in atomic Context | yes | no | no | no |
| precise on "short intervals" | yes | yes | depends | yes |
| precise on "long intervals" | Do not use! | yes | max 12.5% slack | yes |
| interruptible variant | no | yes | yes | no |

A generic advice for non atomic contexts could be:

1. Use `fsleep()` whenever unsure (as it combines all the advantages of the
   others)
1. Use `*sleep()` whenever possible
1. Use `usleep_range*()` whenever accuracy of `*sleep()` is not sufficient
1. Use `*delay()` for very, very short delays

Find some more detailed information about the function 'families' in the next
sections.

#### `*delay()` family of functions

These functions use the jiffy estimation of clock speed and will busy wait for
enough loop cycles to achieve the desired delay. udelay() is the basic
implementation and ndelay() as well as mdelay() are variants.

These functions are mainly used to add a delay in atomic context. Please make
sure to ask yourself before adding a delay in atomic context: Is this really
required?

**`udelay`**

udelay - Inserting a delay based on microseconds with busy waiting
- `usec`: requested delay in microseconds

When delaying in an atomic context ndelay(), udelay() and mdelay() are the
only valid variants of delaying/sleeping to go with.

When inserting delays in non atomic context which are shorter than the time
which is required to queue e.g. an hrtimer and to enter then the scheduler,
it is also valuable to use udelay(). But it is not simple to specify a
generic threshold for this which will fit for all systems. An approximation
is a threshold for all delays up to 10 microseconds.

When having a delay which is larger than the architecture specific
%MAX_UDELAY_MS value, please make sure mdelay() is used. Otherwise a overflow
risk is given.

Please note that ndelay(), udelay() and mdelay() may return early for several
reasons (https://lists.openwall.net/linux-kernel/2011/01/09/56):

1. computed loops_per_jiffy too low (due to the time taken to execute the
   timer interrupt.)
1. cache behaviour affecting the time it takes to execute the loop function.
1. CPU clock rate changes.

**`ndelay`**

ndelay - Inserting a delay based on nanoseconds with busy waiting
- `nsec`: requested delay in nanoseconds

See udelay() for basic information about ndelay() and it's variants.

**`mdelay`**

mdelay - Inserting a delay based on milliseconds with busy waiting
- `n`: requested delay in milliseconds

See udelay() for basic information about mdelay() and it's variants.

Please double check, whether mdelay() is the right way to go or whether a
refactoring of the code is the better variant to be able to use msleep()
instead.

#### `usleep_range*()` and `*sleep()` family of functions

These functions use hrtimers or timer list timers to provide the requested
sleeping duration. In order to decide which function is the right one to use,
take some basic information into account:

1. hrtimers are more expensive as they are using an rb-tree (instead of hashing)
1. hrtimers are more expensive when the requested sleeping duration is the first
   timer which means real hardware has to be programmed
1. timer list timers always provide some sort of slack as they are jiffy based

The generic advice is repeated here:

1. Use `fsleep()` whenever unsure (as it combines all the advantages of the
   others)
1. Use `*sleep()` whenever possible
1. Use `usleep_range*()` whenever accuracy of `*sleep()` is not sufficient

First check fsleep() function description and to learn more about accuracy,
please check msleep() function description.

##### `usleep_range*()`

**`usleep_range`**

usleep_range - Sleep for an approximate time
- `min`: Minimum time in microseconds to sleep
- `max`: Maximum time in microseconds to sleep

For basic information please refer to usleep_range_state().

The task will be in the state TASK_UNINTERRUPTIBLE during the sleep.

**`usleep_range_idle`**

usleep_range_idle - Sleep for an approximate time with idle time accounting
- `min`: Minimum time in microseconds to sleep
- `max`: Maximum time in microseconds to sleep

For basic information please refer to usleep_range_state().

The sleeping task has the state TASK_IDLE during the sleep to prevent
contribution to the load average.

**`usleep_range_state`**

usleep_range_state - Sleep for an approximate time in a given state
- `min`: Minimum time in usecs to sleep
- `max`: Maximum time in usecs to sleep
- `state`: State of the current task that will be while sleeping

usleep_range_state() sleeps at least for the minimum specified time but not
longer than the maximum specified amount of time. The range might reduce
power usage by allowing hrtimers to coalesce an already scheduled interrupt
with this hrtimer. In the worst case, an interrupt is scheduled for the upper
bound.

The sleeping task is set to the specified state before starting the sleep.

In non-atomic context where the exact wakeup time is flexible, use
usleep_range() or its variants instead of udelay(). The sleep improves
responsiveness by avoiding the CPU-hogging busy-wait of udelay().

##### `*sleep()`

**`msleep`**

msleep - sleep safely even with waitqueue interruptions
- `msecs`: Requested sleep duration in milliseconds

msleep() uses jiffy based timeouts for the sleep duration. Because of the
design of the timer wheel, the maximum additional percentage delay (slack) is
12.5%. This is only valid for timers which will end up in level 1 or a higher
level of the timer wheel. For explanation of those 12.5% please check the
detailed description about the basics of the timer wheel.

The slack of timers which will end up in level 0 depends on sleep duration
(msecs) and HZ configuration and can be calculated in the following way (with
the timer wheel design restriction that the slack is not less than 12.5%):

  `slack = MSECS_PER_TICK / msecs`

When the allowed slack of the callsite is known, the calculation could be
turned around to find the minimal allowed sleep duration to meet the
constraints. For example:

* `HZ=1000` with `slack=25%`: `MSECS_PER_TICK / slack = 1 / (1/4) = 4`:
  all sleep durations greater or equal 4ms will meet the constraints.
* `HZ=1000` with `slack=12.5%`: `MSECS_PER_TICK / slack = 1 / (1/8) = 8`:
  all sleep durations greater or equal 8ms will meet the constraints.
* `HZ=250` with `slack=25%`: `MSECS_PER_TICK / slack = 4 / (1/4) = 16`:
  all sleep durations greater or equal 16ms will meet the constraints.
* `HZ=250` with `slack=12.5%`: `MSECS_PER_TICK / slack = 4 / (1/8) = 32`:
  all sleep durations greater or equal 32ms will meet the constraints.

See also the signal aware variant msleep_interruptible().

**`msleep_interruptible`**

msleep_interruptible - sleep waiting for signals
- `msecs`: Requested sleep duration in milliseconds

See msleep() for some basic information.

The difference between msleep() and msleep_interruptible() is that the sleep
could be interrupted by a signal delivery and then returns early.

Returns: The remaining time of the sleep duration transformed to msecs (see
schedule_timeout() for details).

**`ssleep`**

ssleep - wrapper for seconds around msleep
- `seconds`: Requested sleep duration in seconds

Please refer to msleep() for detailed information.

**`fsleep`**

fsleep - flexible sleep which autoselects the best mechanism
- `usecs`: requested sleep duration in microseconds

fsleep() selects the best mechanism that will provide maximum 25% slack
to the requested sleep duration. Therefore it uses:

* udelay() loop for sleep durations <= 10 microseconds to avoid hrtimer
  overhead for really short sleep durations.
* usleep_range() for sleep durations which would lead with the usage of
  msleep() to a slack larger than 25%. This depends on the granularity of
  jiffies.
* msleep() for all other sleep durations.

Note: When %CONFIG_HIGH_RES_TIMERS is not set, all sleeps are processed with
the granularity of jiffies and the slack might exceed 25% especially for
short sleep durations.

---

<a id="doc-7"></a>

## 7. `Documentation/timers/hpet.rst`

### High Precision Event Timer Driver for Linux

The High Precision Event Timer (HPET) hardware follows a specification
by Intel and Microsoft, revision 1.

Each HPET has one fixed-rate counter (at 10+ MHz, hence "High Precision")
and up to 32 comparators.  Normally three or more comparators are provided,
each of which can generate oneshot interrupts and at least one of which has
additional hardware to support periodic interrupts.  The comparators are
also called "timers", which can be misleading since usually timers are
independent of each other ... these share a counter, complicating resets.

HPET devices can support two interrupt routing modes.  In one mode, the
comparators are additional interrupt sources with no particular system
role.  Many x86 BIOS writers don't route HPET interrupts at all, which
prevents use of that mode.  They support the other "legacy replacement"
mode where the first two comparators block interrupts from 8254 timers
and from the RTC.

The driver supports detection of HPET driver allocation and initialization
of the HPET before the driver module_init routine is called.  This enables
platform code which uses timer 0 or 1 as the main timer to intercept HPET
initialization.  An example of this initialization can be found in
arch/x86/kernel/hpet.c.

The driver provides a userspace API which resembles the API found in the
RTC driver framework.  An example user space program is provided in
file:samples/timers/hpet_example.c

---

<a id="doc-8"></a>

## 8. `Documentation/core-api/timekeeping.rst`

### ktime accessors

Device drivers can read the current time using ktime_get() and the many
related functions declared in linux/timekeeping.h. As a rule of thumb,
using an accessor with a shorter name is preferred over one with a longer
name if both are equally fit for a particular use case.

#### Basic ktime_t based interfaces

The recommended simplest form returns an opaque ktime_t, with variants
that return time for different clock references:

```c
ktime_t ktime_get( void )
```

CLOCK_MONOTONIC

Useful for reliable timestamps and measuring short time intervals
accurately. Starts at system boot time but stops during suspend.

```c
ktime_t ktime_get_boottime( void )
```

CLOCK_BOOTTIME

Like ktime_get(), but does not stop when suspended. This can be
used e.g. for key expiration times that need to be synchronized
with other machines across a suspend operation.

```c
ktime_t ktime_get_real( void )
```

CLOCK_REALTIME

Returns the time in relative to the UNIX epoch starting in 1970
using the Coordinated Universal Time (UTC), same as gettimeofday()
user space. This is used for all timestamps that need to
persist across a reboot, like inode times, but should be avoided
for internal uses, since it can jump backwards due to a leap
second update, NTP adjustment settimeofday() operation from user
space.

```c
ktime_t ktime_get_clocktai( void )
```

 CLOCK_TAI

Like ktime_get_real(), but uses the International Atomic Time (TAI)
reference instead of UTC to avoid jumping on leap second updates.
This is rarely useful in the kernel.

```c
ktime_t ktime_get_raw( void )
```

CLOCK_MONOTONIC_RAW

Like ktime_get(), but runs at the same rate as the hardware
clocksource without (NTP) adjustments for clock drift. This is
also rarely needed in the kernel.

#### nanosecond, timespec64, and second output

For all of the above, there are variants that return the time in a
different format depending on what is required by the user:

```c
u64 ktime_get_ns( void )
u64 ktime_get_boottime_ns( void )
u64 ktime_get_real_ns( void )
u64 ktime_get_clocktai_ns( void )
u64 ktime_get_raw_ns( void )
```

Same as the plain ktime_get functions, but returning a u64 number
of nanoseconds in the respective time reference, which may be
more convenient for some callers.

```c
void ktime_get_ts64( struct timespec64 * )
void ktime_get_boottime_ts64( struct timespec64 * )
void ktime_get_real_ts64( struct timespec64 * )
void ktime_get_clocktai_ts64( struct timespec64 * )
void ktime_get_raw_ts64( struct timespec64 * )
```

Same above, but returns the time in a 'struct timespec64', split
into seconds and nanoseconds. This can avoid an extra division
when printing the time, or when passing it into an external
interface that expects a 'timespec' or 'timeval' structure.

```c
time64_t ktime_get_seconds( void )
time64_t ktime_get_boottime_seconds( void )
time64_t ktime_get_real_seconds( void )
time64_t ktime_get_clocktai_seconds( void )
time64_t ktime_get_raw_seconds( void )
```

Return a coarse-grained version of the time as a scalar
time64_t. This avoids accessing the clock hardware and rounds
down the seconds to the full seconds of the last timer tick
using the respective reference.

#### Coarse and fast_ns access

Some additional variants exist for more specialized cases:

```c
ktime_t ktime_get_coarse( void )
ktime_t ktime_get_coarse_boottime( void )
ktime_t ktime_get_coarse_real( void )
ktime_t ktime_get_coarse_clocktai( void )
```

```c
u64 ktime_get_coarse_ns( void )
u64 ktime_get_coarse_boottime_ns( void )
u64 ktime_get_coarse_real_ns( void )
u64 ktime_get_coarse_clocktai_ns( void )
```

```c
void ktime_get_coarse_ts64( struct timespec64 * )
void ktime_get_coarse_boottime_ts64( struct timespec64 * )
void ktime_get_coarse_real_ts64( struct timespec64 * )
void ktime_get_coarse_clocktai_ts64( struct timespec64 * )
```

These are quicker than the non-coarse versions, but less accurate,
corresponding to CLOCK_MONOTONIC_COARSE and CLOCK_REALTIME_COARSE
in user space, along with the equivalent boottime/tai/raw
timebase not available in user space.

The time returned here corresponds to the last timer tick, which
may be as much as 10ms in the past (for CONFIG_HZ=100), same as
reading the 'jiffies' variable.  These are only useful when called
in a fast path and one still expects better than second accuracy,
but can't easily use 'jiffies', e.g. for inode timestamps.
Skipping the hardware clock access saves around 100 CPU cycles
on most modern machines with a reliable cycle counter, but
up to several microseconds on older hardware with an external
clocksource.

```c
u64 ktime_get_mono_fast_ns( void )
u64 ktime_get_raw_fast_ns( void )
u64 ktime_get_boot_fast_ns( void )
u64 ktime_get_tai_fast_ns( void )
u64 ktime_get_real_fast_ns( void )
```

These variants are safe to call from any context, including from
a non-maskable interrupt (NMI) during a timekeeper update, and
while we are entering suspend with the clocksource powered down.
This is useful in some tracing or debugging code as well as
machine check reporting, but most drivers should never call them,
since the time is allowed to jump under certain conditions.

#### Deprecated time interfaces

Older kernels used some other interfaces that are now being phased out
but may appear in third-party drivers being ported here. In particular,
all interfaces returning a 'struct timeval' or 'struct timespec' have
been replaced because the tv_sec member overflows in year 2038 on 32-bit
architectures. These are the recommended replacements:

```c
void ktime_get_ts( struct timespec * )
```

Use ktime_get() or ktime_get_ts64() instead.

```c
void do_gettimeofday( struct timeval * )
void getnstimeofday( struct timespec * )
void getnstimeofday64( struct timespec64 * )
void ktime_get_real_ts( struct timespec * )
```

ktime_get_real_ts64() is a direct replacement, but consider using
monotonic time (ktime_get_ts64()) and/or a ktime_t based interface
(ktime_get()/ktime_get_real()).

```c
struct timespec current_kernel_time( void )
struct timespec64 current_kernel_time64( void )
struct timespec get_monotonic_coarse( void )
struct timespec64 get_monotonic_coarse64( void )
```

These are replaced by ktime_get_coarse_real_ts64() and
ktime_get_coarse_ts64(). However, A lot of code that wants
coarse-grained times can use the simple 'jiffies' instead, while
some drivers may actually want the higher resolution accessors
these days.

```c
struct timespec getrawmonotonic( void )
struct timespec64 getrawmonotonic64( void )
struct timespec timekeeping_clocktai( void )
struct timespec64 timekeeping_clocktai64( void )
struct timespec get_monotonic_boottime( void )
struct timespec64 get_monotonic_boottime64( void )
```

These are replaced by ktime_get_raw()/ktime_get_raw_ts64(),
ktime_get_clocktai()/ktime_get_clocktai_ts64() as well
as ktime_get_boottime()/ktime_get_boottime_ts64().
However, if the particular choice of clock source is not
important for the user, consider converting to
ktime_get()/ktime_get_ts64() instead for consistency.

---

<a id="doc-9"></a>

## 9. `Documentation/virt/kvm/x86/timekeeping.rst`

### Timekeeping Virtualization for X86-Based Architectures

**Author:** Zachary Amsden <zamsden@redhat.com>
**Copyright:** (c) 2010, Red Hat.  All rights reserved.

1) Overview
2) Timing Devices
3) TSC Hardware
4) Virtualization Problems

#### 1. Overview

One of the most complicated parts of the X86 platform, and specifically,
the virtualization of this platform is the plethora of timing devices available
and the complexity of emulating those devices.  In addition, virtualization of
time introduces a new set of challenges because it introduces a multiplexed
division of time beyond the control of the guest CPU.

First, we will describe the various timekeeping hardware available, then
present some of the problems which arise and solutions available, giving
specific recommendations for certain classes of KVM guests.

The purpose of this document is to collect data and information relevant to
timekeeping which may be difficult to find elsewhere, specifically,
information relevant to KVM and hardware-based virtualization.

#### 2. Timing Devices

First we discuss the basic hardware devices available.  TSC and the related
KVM clock are special enough to warrant a full exposition and are described in
the following section.

##### 2.1. i8254 - PIT

One of the first timer devices available is the programmable interrupt timer,
or PIT.  The PIT has a fixed frequency 1.193182 MHz base clock and three
channels which can be programmed to deliver periodic or one-shot interrupts.
These three channels can be configured in different modes and have individual
counters.  Channel 1 and 2 were not available for general use in the original
IBM PC, and historically were connected to control RAM refresh and the PC
speaker.  Now the PIT is typically integrated as part of an emulated chipset
and a separate physical PIT is not used.

The PIT uses I/O ports 0x40 - 0x43.  Access to the 16-bit counters is done
using single or multiple byte access to the I/O ports.  There are 6 modes
available, but not all modes are available to all timers, as only timer 2
has a connected gate input, required for modes 1 and 5.  The gate line is
controlled by port 61h, bit 0, as illustrated in the following diagram:

```

--------------             ----------------
|            |           |                |
|  1.1932 MHz|---------->| CLOCK      OUT | ---------> IRQ 0
|    Clock   |   |       |                |
--------------   |    +->| GATE  TIMER 0  |
                 |        ----------------
                 |
                 |        ----------------
                 |       |                |
                 |------>| CLOCK      OUT | ---------> 66.3 KHZ DRAM
                 |       |                |            (aka /dev/null)
                 |    +->| GATE  TIMER 1  |
                 |        ----------------
                 |
                 |        ----------------
                 |       |                |
                 |------>| CLOCK      OUT | ---------> Port 61h, bit 5
                         |                |      |
Port 61h, bit 0 -------->| GATE  TIMER 2  |       \_.----   ____
                          ----------------         _|    )--|LPF|---Speaker
                                                  / *----   \___/
Port 61h, bit 1 ---------------------------------/
```

The timer modes are now described.

Mode 0: Single Timeout.
 This is a one-shot software timeout that counts down
 when the gate is high (always true for timers 0 and 1).  When the count
 reaches zero, the output goes high.

Mode 1: Triggered One-shot.
 The output is initially set high.  When the gate
 line is set high, a countdown is initiated (which does not stop if the gate is
 lowered), during which the output is set low.  When the count reaches zero,
 the output goes high.

Mode 2: Rate Generator.
 The output is initially set high.  When the countdown
 reaches 1, the output goes low for one count and then returns high.  The value
 is reloaded and the countdown automatically resumes.  If the gate line goes
 low, the count is halted.  If the output is low when the gate is lowered, the
 output automatically goes high (this only affects timer 2).

Mode 3: Square Wave.
 This generates a high / low square wave.  The count
 determines the length of the pulse, which alternates between high and low
 when zero is reached.  The count only proceeds when gate is high and is
 automatically reloaded on reaching zero.  The count is decremented twice at
 each clock to generate a full high / low cycle at the full periodic rate.
 If the count is even, the clock remains high for N/2 counts and low for N/2
 counts; if the clock is odd, the clock is high for (N+1)/2 counts and low
 for (N-1)/2 counts.  Only even values are latched by the counter, so odd
 values are not observed when reading.  This is the intended mode for timer 2,
 which generates sine-like tones by low-pass filtering the square wave output.

Mode 4: Software Strobe.
 After programming this mode and loading the counter,
 the output remains high until the counter reaches zero.  Then the output
 goes low for 1 clock cycle and returns high.  The counter is not reloaded.
 Counting only occurs when gate is high.

Mode 5: Hardware Strobe.
 After programming and loading the counter, the
 output remains high.  When the gate is raised, a countdown is initiated
 (which does not stop if the gate is lowered).  When the counter reaches zero,
 the output goes low for 1 clock cycle and then returns high.  The counter is
 not reloaded.

In addition to normal binary counting, the PIT supports BCD counting.  The
command port, 0x43 is used to set the counter and mode for each of the three
timers.

PIT commands, issued to port 0x43, using the following bit encoding:

```

Bit 7-4: Command (See table below)
Bit 3-1: Mode (000 = Mode 0, 101 = Mode 5, 11X = undefined)
Bit 0  : Binary (0) / BCD (1)
```

Command table:

```

 0000 - Latch Timer 0 count for port 0x40
sample and hold the count to be read in port 0x40;
additional commands ignored until counter is read;
mode bits ignored.

 0001 - Set Timer 0 LSB mode for port 0x40
set timer to read LSB only and force MSB to zero;
mode bits set timer mode

 0010 - Set Timer 0 MSB mode for port 0x40
set timer to read MSB only and force LSB to zero;
mode bits set timer mode

 0011 - Set Timer 0 16-bit mode for port 0x40
set timer to read / write LSB first, then MSB;
mode bits set timer mode

 0100 - Latch Timer 1 count for port 0x41 - as described above
 0101 - Set Timer 1 LSB mode for port 0x41 - as described above
 0110 - Set Timer 1 MSB mode for port 0x41 - as described above
 0111 - Set Timer 1 16-bit mode for port 0x41 - as described above

 1000 - Latch Timer 2 count for port 0x42 - as described above
 1001 - Set Timer 2 LSB mode for port 0x42 - as described above
 1010 - Set Timer 2 MSB mode for port 0x42 - as described above
 1011 - Set Timer 2 16-bit mode for port 0x42 as described above

 1101 - General counter latch
Latch combination of counters into corresponding ports
Bit 3 = Counter 2
Bit 2 = Counter 1
Bit 1 = Counter 0
Bit 0 = Unused

 1110 - Latch timer status
Latch combination of counter mode into corresponding ports
Bit 3 = Counter 2
Bit 2 = Counter 1
Bit 1 = Counter 0

The output of ports 0x40-0x42 following this command will be:

Bit 7 = Output pin
Bit 6 = Count loaded (0 if timer has expired)
Bit 5-4 = Read / Write mode
    01 = MSB only
    10 = LSB only
    11 = LSB / MSB (16-bit)
Bit 3-1 = Mode
Bit 0 = Binary (0) / BCD mode (1)
```

##### 2.2. RTC

The second device which was available in the original PC was the MC146818 real
time clock.  The original device is now obsolete, and usually emulated by the
system chipset, sometimes by an HPET and some frankenstein IRQ routing.

The RTC is accessed through CMOS variables, which uses an index register to
control which bytes are read.  Since there is only one index register, read
of the CMOS and read of the RTC require lock protection (in addition, it is
dangerous to allow userspace utilities such as hwclock to have direct RTC
access, as they could corrupt kernel reads and writes of CMOS memory).

The RTC generates an interrupt which is usually routed to IRQ 8.  The interrupt
can function as a periodic timer, an additional once a day alarm, and can issue
interrupts after an update of the CMOS registers by the MC146818 is complete.
The type of interrupt is signalled in the RTC status registers.

The RTC will update the current time fields by battery power even while the
system is off.  The current time fields should not be read while an update is
in progress, as indicated in the status register.

The clock uses a 32.768kHz crystal, so bits 6-4 of register A should be
programmed to a 32kHz divider if the RTC is to count seconds.

This is the RAM map originally used for the RTC/CMOS:

```

Location    Size    Description
------------------------------------------
00h         byte    Current second (BCD)
01h         byte    Seconds alarm (BCD)
02h         byte    Current minute (BCD)
03h         byte    Minutes alarm (BCD)
04h         byte    Current hour (BCD)
05h         byte    Hours alarm (BCD)
06h         byte    Current day of week (BCD)
07h         byte    Current day of month (BCD)
08h         byte    Current month (BCD)
09h         byte    Current year (BCD)
0Ah         byte    Register A
                     bit 7   = Update in progress
                     bit 6-4 = Divider for clock
                                000 = 4.194 MHz
                                001 = 1.049 MHz
                                010 = 32 kHz
                                10X = test modes
                                110 = reset / disable
                                111 = reset / disable
                     bit 3-0 = Rate selection for periodic interrupt
                                000 = periodic timer disabled
                                001 = 3.90625 uS
                                010 = 7.8125 uS
                                011 = .122070 mS
                                100 = .244141 mS
                                   ...
                               1101 = 125 mS
                               1110 = 250 mS
                               1111 = 500 mS
0Bh         byte    Register B
                     bit 7   = Run (0) / Halt (1)
                     bit 6   = Periodic interrupt enable
                     bit 5   = Alarm interrupt enable
                     bit 4   = Update-ended interrupt enable
                     bit 3   = Square wave interrupt enable
                     bit 2   = BCD calendar (0) / Binary (1)
                     bit 1   = 12-hour mode (0) / 24-hour mode (1)
                     bit 0   = 0 (DST off) / 1 (DST enabled)
OCh         byte    Register C (read only)
                     bit 7   = interrupt request flag (IRQF)
                     bit 6   = periodic interrupt flag (PF)
                     bit 5   = alarm interrupt flag (AF)
                     bit 4   = update interrupt flag (UF)
                     bit 3-0 = reserved
ODh         byte    Register D (read only)
                     bit 7   = RTC has power
                     bit 6-0 = reserved
32h         byte    Current century BCD (*)
(*) location vendor specific and now determined from ACPI global tables
```

##### 2.3. APIC

On Pentium and later processors, an on-board timer is available to each CPU
as part of the Advanced Programmable Interrupt Controller.  The APIC is
accessed through memory-mapped registers and provides interrupt service to each
CPU, used for IPIs and local timer interrupts.

Although in theory the APIC is a safe and stable source for local interrupts,
in practice, many bugs and glitches have occurred due to the special nature of
the APIC CPU-local memory-mapped hardware.  Beware that CPU errata may affect
the use of the APIC and that workarounds may be required.  In addition, some of
these workarounds pose unique constraints for virtualization - requiring either
extra overhead incurred from extra reads of memory-mapped I/O or additional
functionality that may be more computationally expensive to implement.

Since the APIC is documented quite well in the Intel and AMD manuals, we will
avoid repetition of the detail here.  It should be pointed out that the APIC
timer is programmed through the LVT (local vector timer) register, is capable
of one-shot or periodic operation, and is based on the bus clock divided down
by the programmable divider register.

##### 2.4. HPET

HPET is quite complex, and was originally intended to replace the PIT / RTC
support of the X86 PC.  It remains to be seen whether that will be the case, as
the de facto standard of PC hardware is to emulate these older devices.  Some
systems designated as legacy free may support only the HPET as a hardware timer
device.

The HPET spec is rather loose and vague, requiring at least 3 hardware timers,
but allowing implementation freedom to support many more.  It also imposes no
fixed rate on the timer frequency, but does impose some extremal values on
frequency, error and slew.

In general, the HPET is recommended as a high precision (compared to PIT /RTC)
time source which is independent of local variation (as there is only one HPET
in any given system).  The HPET is also memory-mapped, and its presence is
indicated through ACPI tables by the BIOS.

Detailed specification of the HPET is beyond the current scope of this
document, as it is also very well documented elsewhere.

##### 2.5. Offboard Timers

Several cards, both proprietary (watchdog boards) and commonplace (e1000) have
timing chips built into the cards which may have registers which are accessible
to kernel or user drivers.  To the author's knowledge, using these to generate
a clocksource for a Linux or other kernel has not yet been attempted and is in
general frowned upon as not playing by the agreed rules of the game.  Such a
timer device would require additional support to be virtualized properly and is
not considered important at this time as no known operating system does this.

#### 3. TSC Hardware

The TSC or time stamp counter is relatively simple in theory; it counts
instruction cycles issued by the processor, which can be used as a measure of
time.  In practice, due to a number of problems, it is the most complicated
timekeeping device to use.

The TSC is represented internally as a 64-bit MSR which can be read with the
RDMSR, RDTSC, or RDTSCP (when available) instructions.  In the past, hardware
limitations made it possible to write the TSC, but generally on old hardware it
was only possible to write the low 32-bits of the 64-bit counter, and the upper
32-bits of the counter were cleared.  Now, however, on Intel processors family
0Fh, for models 3, 4 and 6, and family 06h, models e and f, this restriction
has been lifted and all 64-bits are writable.  On AMD systems, the ability to
write the TSC MSR is not an architectural guarantee.

The TSC is accessible from CPL-0 and conditionally, for CPL > 0 software by
means of the CR4.TSD bit, which when enabled, disables CPL > 0 TSC access.

Some vendors have implemented an additional instruction, RDTSCP, which returns
atomically not just the TSC, but an indicator which corresponds to the
processor number.  This can be used to index into an array of TSC variables to
determine offset information in SMP systems where TSCs are not synchronized.
The presence of this instruction must be determined by consulting CPUID feature
bits.

Both VMX and SVM provide extension fields in the virtualization hardware which
allows the guest visible TSC to be offset by a constant.  Newer implementations
promise to allow the TSC to additionally be scaled, but this hardware is not
yet widely available.

##### 3.1. TSC synchronization

The TSC is a CPU-local clock in most implementations.  This means, on SMP
platforms, the TSCs of different CPUs may start at different times depending
on when the CPUs are powered on.  Generally, CPUs on the same die will share
the same clock, however, this is not always the case.

The BIOS may attempt to resynchronize the TSCs during the poweron process and
the operating system or other system software may attempt to do this as well.
Several hardware limitations make the problem worse - if it is not possible to
write the full 64-bits of the TSC, it may be impossible to match the TSC in
newly arriving CPUs to that of the rest of the system, resulting in
unsynchronized TSCs.  This may be done by BIOS or system software, but in
practice, getting a perfectly synchronized TSC will not be possible unless all
values are read from the same clock, which generally only is possible on single
socket systems or those with special hardware support.

##### 3.2. TSC and CPU hotplug

As touched on already, CPUs which arrive later than the boot time of the system
may not have a TSC value that is synchronized with the rest of the system.
Either system software, BIOS, or SMM code may actually try to establish the TSC
to a value matching the rest of the system, but a perfect match is usually not
a guarantee.  This can have the effect of bringing a system from a state where
TSC is synchronized back to a state where TSC synchronization flaws, however
small, may be exposed to the OS and any virtualization environment.

##### 3.3. TSC and multi-socket / NUMA

Multi-socket systems, especially large multi-socket systems are likely to have
individual clocksources rather than a single, universally distributed clock.
Since these clocks are driven by different crystals, they will not have
perfectly matched frequency, and temperature and electrical variations will
cause the CPU clocks, and thus the TSCs to drift over time.  Depending on the
exact clock and bus design, the drift may or may not be fixed in absolute
error, and may accumulate over time.

In addition, very large systems may deliberately slew the clocks of individual
cores.  This technique, known as spread-spectrum clocking, reduces EMI at the
clock frequency and harmonics of it, which may be required to pass FCC
standards for telecommunications and computer equipment.

It is recommended not to trust the TSCs to remain synchronized on NUMA or
multiple socket systems for these reasons.

##### 3.4. TSC and C-states

C-states, or idling states of the processor, especially C1E and deeper sleep
states may be problematic for TSC as well.  The TSC may stop advancing in such
a state, resulting in a TSC which is behind that of other CPUs when execution
is resumed.  Such CPUs must be detected and flagged by the operating system
based on CPU and chipset identifications.

The TSC in such a case may be corrected by catching it up to a known external
clocksource.

##### 3.5. TSC frequency change / P-states

To make things slightly more interesting, some CPUs may change frequency.  They
may or may not run the TSC at the same rate, and because the frequency change
may be staggered or slewed, at some points in time, the TSC rate may not be
known other than falling within a range of values.  In this case, the TSC will
not be a stable time source, and must be calibrated against a known, stable,
external clock to be a usable source of time.

Whether the TSC runs at a constant rate or scales with the P-state is model
dependent and must be determined by inspecting CPUID, chipset or vendor
specific MSR fields.

In addition, some vendors have known bugs where the P-state is actually
compensated for properly during normal operation, but when the processor is
inactive, the P-state may be raised temporarily to service cache misses from
other processors.  In such cases, the TSC on halted CPUs could advance faster
than that of non-halted processors.  AMD Turion processors are known to have
this problem.

##### 3.6. TSC and STPCLK / T-states

External signals given to the processor may also have the effect of stopping
the TSC.  This is typically done for thermal emergency power control to prevent
an overheating condition, and typically, there is no way to detect that this
condition has happened.

##### 3.7. TSC virtualization - VMX

VMX provides conditional trapping of RDTSC, RDMSR, WRMSR and RDTSCP
instructions, which is enough for full virtualization of TSC in any manner.  In
addition, VMX allows passing through the host TSC plus an additional TSC_OFFSET
field specified in the VMCS.  Special instructions must be used to read and
write the VMCS field.

##### 3.8. TSC virtualization - SVM

SVM provides conditional trapping of RDTSC, RDMSR, WRMSR and RDTSCP
instructions, which is enough for full virtualization of TSC in any manner.  In
addition, SVM allows passing through the host TSC plus an additional offset
field specified in the SVM control block.

##### 3.9. TSC feature bits in Linux

In summary, there is no way to guarantee the TSC remains in perfect
synchronization unless it is explicitly guaranteed by the architecture.  Even
if so, the TSCs in multi-sockets or NUMA systems may still run independently
despite being locally consistent.

The following feature bits are used by Linux to signal various TSC attributes,
but they can only be taken to be meaningful for UP or single node systems.

```
=========================	=======================================
X86_FEATURE_TSC			The TSC is available in hardware
X86_FEATURE_RDTSCP		The RDTSCP instruction is available
X86_FEATURE_CONSTANT_TSC	The TSC rate is unchanged with P-states
X86_FEATURE_NONSTOP_TSC		The TSC does not stop in C-states
X86_FEATURE_TSC_RELIABLE	TSC sync checks are skipped (VMware)
=========================	=======================================
```

#### 4. Virtualization Problems

Timekeeping is especially problematic for virtualization because a number of
challenges arise.  The most obvious problem is that time is now shared between
the host and, potentially, a number of virtual machines.  Thus the virtual
operating system does not run with 100% usage of the CPU, despite the fact that
it may very well make that assumption.  It may expect it to remain true to very
exacting bounds when interrupt sources are disabled, but in reality only its
virtual interrupt sources are disabled, and the machine may still be preempted
at any time.  This causes problems as the passage of real time, the injection
of machine interrupts and the associated clock sources are no longer completely
synchronized with real time.

This same problem can occur on native hardware to a degree, as SMM mode may
steal cycles from the naturally on X86 systems when SMM mode is used by the
BIOS, but not in such an extreme fashion.  However, the fact that SMM mode may
cause similar problems to virtualization makes it a good justification for
solving many of these problems on bare metal.

##### 4.1. Interrupt clocking

One of the most immediate problems that occurs with legacy operating systems
is that the system timekeeping routines are often designed to keep track of
time by counting periodic interrupts.  These interrupts may come from the PIT
or the RTC, but the problem is the same: the host virtualization engine may not
be able to deliver the proper number of interrupts per second, and so guest
time may fall behind.  This is especially problematic if a high interrupt rate
is selected, such as 1000 HZ, which is unfortunately the default for many Linux
guests.

There are three approaches to solving this problem; first, it may be possible
to simply ignore it.  Guests which have a separate time source for tracking
'wall clock' or 'real time' may not need any adjustment of their interrupts to
maintain proper time.  If this is not sufficient, it may be necessary to inject
additional interrupts into the guest in order to increase the effective
interrupt rate.  This approach leads to complications in extreme conditions,
where host load or guest lag is too much to compensate for, and thus another
solution to the problem has risen: the guest may need to become aware of lost
ticks and compensate for them internally.  Although promising in theory, the
implementation of this policy in Linux has been extremely error prone, and a
number of buggy variants of lost tick compensation are distributed across
commonly used Linux systems.

Windows uses periodic RTC clocking as a means of keeping time internally, and
thus requires interrupt slewing to keep proper time.  It does use a low enough
rate (ed: is it 18.2 Hz?) however that it has not yet been a problem in
practice.

##### 4.2. TSC sampling and serialization

As the highest precision time source available, the cycle counter of the CPU
has aroused much interest from developers.  As explained above, this timer has
many problems unique to its nature as a local, potentially unstable and
potentially unsynchronized source.  One issue which is not unique to the TSC,
but is highlighted because of its very precise nature is sampling delay.  By
definition, the counter, once read is already old.  However, it is also
possible for the counter to be read ahead of the actual use of the result.
This is a consequence of the superscalar execution of the instruction stream,
which may execute instructions out of order.  Such execution is called
non-serialized.  Forcing serialized execution is necessary for precise
measurement with the TSC, and requires a serializing instruction, such as CPUID
or an MSR read.

Since CPUID may actually be virtualized by a trap and emulate mechanism, this
serialization can pose a performance issue for hardware virtualization.  An
accurate time stamp counter reading may therefore not always be available, and
it may be necessary for an implementation to guard against "backwards" reads of
the TSC as seen from other CPUs, even in an otherwise perfectly synchronized
system.

##### 4.3. Timespec aliasing

Additionally, this lack of serialization from the TSC poses another challenge
when using results of the TSC when measured against another time source.  As
the TSC is much higher precision, many possible values of the TSC may be read
while another clock is still expressing the same value.

That is, you may read (T,T+10) while external clock C maintains the same value.
Due to non-serialized reads, you may actually end up with a range which
fluctuates - from (T-1.. T+10).  Thus, any time calculated from a TSC, but
calibrated against an external value may have a range of valid values.
Re-calibrating this computation may actually cause time, as computed after the
calibration, to go backwards, compared with time computed before the
calibration.

This problem is particularly pronounced with an internal time source in Linux,
the kernel time, which is expressed in the theoretically high resolution
timespec - but which advances in much larger granularity intervals, sometimes
at the rate of jiffies, and possibly in catchup modes, at a much larger step.

This aliasing requires care in the computation and recalibration of kvmclock
and any other values derived from TSC computation (such as TSC virtualization
itself).

##### 4.4. Migration

Migration of a virtual machine raises problems for timekeeping in two ways.
First, the migration itself may take time, during which interrupts cannot be
delivered, and after which, the guest time may need to be caught up.  NTP may
be able to help to some degree here, as the clock correction required is
typically small enough to fall in the NTP-correctable window.

An additional concern is that timers based off the TSC (or HPET, if the raw bus
clock is exposed) may now be running at different rates, requiring compensation
in some way in the hypervisor by virtualizing these timers.  In addition,
migrating to a faster machine may preclude the use of a passthrough TSC, as a
faster clock cannot be made visible to a guest without the potential of time
advancing faster than usual.  A slower clock is less of a problem, as it can
always be caught up to the original rate.  KVM clock avoids these problems by
simply storing multipliers and offsets against the TSC for the guest to convert
back into nanosecond resolution values.

##### 4.5. Scheduling

Since scheduling may be based on precise timing and firing of interrupts, the
scheduling algorithms of an operating system may be adversely affected by
virtualization.  In theory, the effect is random and should be universally
distributed, but in contrived as well as real scenarios (guest device access,
causes of virtualization exits, possible context switch), this may not always
be the case.  The effect of this has not been well studied.

In an attempt to work around this, several implementations have provided a
paravirtualized scheduler clock, which reveals the true amount of CPU time for
which a virtual machine has been running.

##### 4.6. Watchdogs

Watchdog timers, such as the lock detector in Linux may fire accidentally when
running under hardware virtualization due to timer interrupts being delayed or
misinterpretation of the passage of real time.  Usually, these warnings are
spurious and can be ignored, but in some circumstances it may be necessary to
disable such detection.

##### 4.7. Delays and precision timing

Precise timing and delays may not be possible in a virtualized system.  This
can happen if the system is controlling physical hardware, or issues delays to
compensate for slower I/O to and from devices.  The first issue is not solvable
in general for a virtualized system; hardware control software can't be
adequately virtualized without a full real-time operating system, which would
require an RT aware virtualization platform.

The second issue may cause performance problems, but this is unlikely to be a
significant issue.  In many cases these delays may be eliminated through
configuration or paravirtualization.

##### 4.8. Covert channels and leaks

In addition to the above problems, time information will inevitably leak to the
guest about the host in anything but a perfect implementation of virtualized
time.  This may allow the guest to infer the presence of a hypervisor (as in a
red-pill type detection), and it may allow information to leak between guests
by using CPU utilization itself as a signalling channel.  Preventing such
problems would require completely isolated virtual time which may not track
real time any longer.  This may be useful in certain security or QA contexts,
but in general isn't recommended for real-world deployment scenarios.

---

<a id="doc-10"></a>

## 10. `Documentation/virt/kvm/arm/ptp_kvm.rst`

### PTP_KVM support for arm/arm64

PTP_KVM is used for high precision time sync between host and guests.
It relies on transferring the wall clock and counter value from the
host to the guest using a KVM-specific hypercall.

#### `ARM_SMCCC_VENDOR_HYP_KVM_PTP_FUNC_ID`

Retrieve current time information for the specific counter. There are no
endianness restrictions.

```
+---------------------+-------------------------------------------------------+
| Presence:           | Optional                                              |
+---------------------+-------------------------------------------------------+
| Calling convention: | HVC32                                                 |
+---------------------+----------+--------------------------------------------+
| Function ID:        | (uint32) | 0x86000001                                 |
+---------------------+----------+----+---------------------------------------+
| Arguments:          | (uint32) | R1 | `KVM_PTP_VIRT_COUNTER (0)`          |
|                     |          |    +---------------------------------------+
|                     |          |    | `KVM_PTP_PHYS_COUNTER (1)`          |
+---------------------+----------+----+---------------------------------------+
| Return Values:      | (int32)  | R0 | `NOT_SUPPORTED (-1)` on error, else |
|                     |          |    | upper 32 bits of wall clock time      |
|                     +----------+----+---------------------------------------+
|                     | (uint32) | R1 | Lower 32 bits of wall clock time      |
|                     +----------+----+---------------------------------------+
|                     | (uint32) | R2 | Upper 32 bits of counter              |
|                     +----------+----+---------------------------------------+
|                     | (uint32) | R3 | Lower 32 bits of counter              |
+---------------------+----------+----+---------------------------------------+
```

---

<a id="doc-11"></a>

## 11. `Documentation/virt/kvm/arm/pvtime.rst`

### Paravirtualized time support for arm64

Arm specification DEN0057/A defines a standard for paravirtualised time
support for AArch64 guests:

https://developer.arm.com/docs/den0057/a

KVM/arm64 implements the stolen time part of this specification by providing
some hypervisor service calls to support a paravirtualized guest obtaining a
view of the amount of time stolen from its execution.

Two new SMCCC compatible hypercalls are defined:

* PV_TIME_FEATURES: 0xC5000020
* PV_TIME_ST:       0xC5000021

These are only available in the SMC64/HVC64 calling convention as
paravirtualized time is not available to 32 bit Arm guests. The existence of
the PV_TIME_FEATURES hypercall should be probed using the SMCCC 1.1
ARCH_FEATURES mechanism before calling it.

PV_TIME_FEATURES

```
    ============= ========    =================================================
    Function ID:  (uint32)    0xC5000020
    PV_call_id:   (uint32)    The function to query for support.
                              Currently only PV_TIME_ST is supported.
    Return value: (int64)     NOT_SUPPORTED (-1) or SUCCESS (0) if the relevant
                              PV-time feature is supported by the hypervisor.
    ============= ========    =================================================
```

PV_TIME_ST

```
    ============= ========    ==============================================
    Function ID:  (uint32)    0xC5000021
    Return value: (int64)     IPA of the stolen time data structure for this
                              VCPU. On failure:
                              NOT_SUPPORTED (-1)
    ============= ========    ==============================================
```

The IPA returned by PV_TIME_ST should be mapped by the guest as normal memory
with inner and outer write back caching attributes, in the inner shareable
domain. A total of 16 bytes from the IPA returned are guaranteed to be
meaningfully filled by the hypervisor (see structure below).

PV_TIME_ST returns the structure for the calling VCPU.

#### Stolen Time

The structure pointed to by the PV_TIME_ST hypercall is as follows:

```
+-------------+-------------+-------------+----------------------------+
| Field       | Byte Length | Byte Offset | Description                |
+=============+=============+=============+============================+
| Revision    |      4      |      0      | Must be 0 for version 1.0  |
+-------------+-------------+-------------+----------------------------+
| Attributes  |      4      |      4      | Must be 0                  |
+-------------+-------------+-------------+----------------------------+
| Stolen time |      8      |      8      | Stolen time in unsigned    |
|             |             |             | nanoseconds indicating how |
|             |             |             | much time this VCPU thread |
|             |             |             | was involuntarily not      |
|             |             |             | running on a physical CPU. |
+-------------+-------------+-------------+----------------------------+
```

All values in the structure are stored little-endian.

The structure will be updated by the hypervisor prior to scheduling a VCPU. It
will be present within a reserved region of the normal memory given to the
guest. The guest should not attempt to write into this memory. There is a
structure per VCPU of the guest.

It is advisable that one or more 64k pages are set aside for the purpose of
these structures and not used for other purposes, this enables the guest to map
the region using 64k pages and avoids conflicting attributes with other memory.

For the user space interface see
kvm_arm_vcpu_pvtime_ctrl.

---

<a id="doc-12"></a>

## 12. `Documentation/virt/kvm/devices/vcpu.rst （仅 KVM_VCPU_TSC_CTRL 组）`

### 4. GROUP: KVM_VCPU_TSC_CTRL

**Architectures:** x86

4.1 ATTRIBUTE: KVM_VCPU_TSC_OFFSET

**Parameters:** 64-bit unsigned TSC offset

Returns:

```
	 ======= ======================================
	 -EFAULT Error reading/writing the provided
		 parameter address.
	 -ENXIO  Attribute not supported
	 ======= ======================================
```

Specifies the guest's TSC offset relative to the host's TSC. The guest's
TSC is then derived by the following equation:

  guest_tsc = host_tsc + KVM_VCPU_TSC_OFFSET

This attribute is useful to adjust the guest's TSC on live migration,
so that the TSC counts the time during which the VM was paused. The
following describes a possible algorithm to use for this purpose.

From the source VMM process:

1. Invoke the KVM_GET_CLOCK ioctl to record the host TSC (tsc_src),
   kvmclock nanoseconds (guest_src), and host CLOCK_REALTIME nanoseconds
   (host_src).

2. Read the KVM_VCPU_TSC_OFFSET attribute for every vCPU to record the
   guest TSC offset (ofs_src[i]).

3. Invoke the KVM_GET_TSC_KHZ ioctl to record the frequency of the
   guest's TSC (freq).

From the destination VMM process:

4. Invoke the KVM_SET_CLOCK ioctl, providing the source nanoseconds from
   kvmclock (guest_src) and CLOCK_REALTIME (host_src) in their respective
   fields.  Ensure that the KVM_CLOCK_REALTIME flag is set in the provided
   structure.

   KVM will advance the VM's kvmclock to account for elapsed time since
   recording the clock values.  Note that this will cause problems in
   the guest (e.g., timeouts) unless CLOCK_REALTIME is synchronized
   between the source and destination, and a reasonably short time passes
   between the source pausing the VMs and the destination executing
   steps 4-7.

5. Invoke the KVM_GET_CLOCK ioctl to record the host TSC (tsc_dest) and
   kvmclock nanoseconds (guest_dest).

6. Adjust the guest TSC offsets for every vCPU to account for (1) time
   elapsed since recording state and (2) difference in TSCs between the
   source and destination machine:

   ofs_dst[i] = ofs_src[i] -
     (guest_src - guest_dest) * freq +
     (tsc_src - tsc_dest)

   ("ofs[i] + tsc - guest * freq" is the guest TSC value corresponding to
   a time of 0 in kvmclock.  The above formula ensures that it is the
   same on the destination as it was on the source).

7. Write the KVM_VCPU_TSC_OFFSET attribute for every vCPU with the
   respective value derived in the previous step.

---

<a id="doc-13"></a>

## 13. `Documentation/ABI/stable/sysfs-kernel-time-aux-clocks`

```
What:		/sys/kernel/time/aux_clocks/<ID>/enable
Date:		May 2025
Contact:	Thomas Gleixner <tglx@kernel.org>
Description:
		Controls the enablement of auxiliary clock timekeepers.
```

---

## 附：相关但未收录的文档

以下 `linux-drm` 文档同样涉及时间，但并非专门讨论时间子系统，故未逐字收录：

- `Documentation/virt/kvm/devices/vcpu.rst` — `KVM_VCPU_TSC_CTRL` 组（已摘录，见第 12 节）
- `Documentation/virt/kvm/x86/msr.rst` — 包含 `MSR_IA32_TSC`、`MSR_IA32_TSC_ADJUST` 等
- `Documentation/virt/kvm/x86/cpuid.rst` — 包含 TSC 相关 CPUID 特性位
- `Documentation/driver-api/ptp.rst`、`Documentation/networking/timestamping.rst` — PTP/硬件时间戳
- `Documentation/ABI/stable/sysfs-kernel-time-aux-clocks` — auxiliary clock timekeepers 的 sysfs 接口（已收录，见第 13 节）

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
