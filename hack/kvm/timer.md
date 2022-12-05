# timer

## https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/7/html/virtualization_deployment_and_administration_guide/chap-kvm_guest_timing_management

Guest virtual machines without accurate time keeping may experience issues with network applications and processes, as session validity, migration, and other network activities rely on timestamps to remain correct.

KVM avoids these issues by providing guest virtual machines with a paravirtualized clock (kvm-clock).
However, it is still important to test timing before attempting activities that may be affected by time keeping inaccuracies, such as guest *migration*.

By default, the guest synchronizes its time with the hypervisor as follows:
- When the guest system boots, the guest reads the time from the emulated Real Time Clock (RTC).
- When the NTP protocol is initiated, it automatically synchronizes the guest clock. Afterwards, during normal guest operation, NTP performs clock adjustments in the guest.
- When a guest is resumed after a pause or a restoration process, a command to synchronize the guest clock to a specified value should be issued by the management software (such as virt-manager). This synchronization works only if the QEMU guest agent is installed in the guest and supports the feature. The value to which the guest clock synchronizes is usually the host clock value.

- [ ] trace code :
  - [ ] read emulated RTC
  - [ ] NTP synchronize guest clock
  - [ ] resumed / pause : qemu synchronize guest clock

Modern Intel and AMD CPUs provide a constant Time Stamp Counter (TSC). The count frequency of the constant TSC does **not vary** when the CPU core itself changes frequency, for example to comply with a power-saving policy.
A CPU with a **constant TSC frequency** is necessary in order to use the TSC as a clock source for KVM guests.

- [ ] What we get in the kvmtool, kind of dispointed.
```
root@qemux86:~# cat /sys/devices/system/clocksource/clocksource0/available_clocksource
refined-jiffies jiffies
```

## https://github.com/GiantVM/KVM-Annotation/wiki/Kvmclock

- https://rwmj.wordpress.com/2010/10/15/kvm-pvclock/
- https://lkml.org/lkml/2010/4/15/355

## https://github.com/GiantVM/KVM-Annotation/wiki/Steal-Time


## x86/kvm/i8254.c
hrtimer --expired--> pit_timer_fn ----queue kthread---> pit_do_work ---call--> kvm_set_irq

## 难道使用 vdso 的时候，那些内容是重复的？

感觉 vdso 使用的 rdtsc 的，而 non vdso 使用的 kvmclock 的。

## https://github.com/WCharacter/RDTSC-KVM-Handler

See “Changes to Instruction Behavior in VMX Non-Root Operation” in Chapter 25 of the Intel® 64 and IA-32 Architectures Software Developer’s Manual, Volume 3C, for more information about the behavior of this instruction in VMX non-root operation.

## https://www.redhat.com/en/blog/avoiding-clock-drift-vms

## https://www.kernel.org/doc/Documentation/virtual/kvm/timekeeping.txt

## https://www.google.com.hk/search?q=kvm+clock+vs+tsc&newwindow=1&sxsrf=ALiCzsZSUeVrlv1hASUowmU4_9Mw0b6Ztw%3A1659617897923&ei=acLrYrKKOP7RkPIPxaaz2A8&oq=kvmclock+vs&gs_lcp=Cgdnd3Mtd2l6EAMYADIECAAQDTIFCAAQhgMyBQgAEIYDOgcIABBHELADOgYIABAeEA06CAgAEB4QDRAKOggIABAeEAgQDToKCAAQHhAPEBYQCkoECEEYAEoECEYYAFDhBFiaB2CGFGgBcAF4AIABsQKIAfwGkgEFMi0xLjKYAQCgAQHIAQjAAQE&sclient=gws-wiz

- https://news.ycombinator.com/item?id=32548085
- http://liujunming.top/2022/08/20/Notes-about-KVM-steal-time/ : steal time
