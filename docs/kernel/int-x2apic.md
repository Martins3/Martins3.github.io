## amd 中如何打开 x2apic
amd
```txt
🧀  dmesg | grep x2apic
[    0.129273] x2apic: IRQ remapping doesn't support X2APIC mode
```

intel
```txt
🧀  dmesg | grep x2apic
[    0.181268] DMAR-IR: Queued invalidation will be enabled to support x2apic and Intr-remapping.
[    0.182790] DMAR-IR: Enabled IRQ remapping in x2apic mode
[    0.182791] x2apic enabled
[    0.182814] Switched APIC routing to cluster x2apic.
```

## x2apic 在 AMD 中如何打开

https://stackoverflow.com/questions/60219639/kernel-error-irq-remapping-doesnt-support-x2apic-mode-disabled-x2apic

## 基本介绍

https://serverfault.com/questions/873664/when-to-use-x2apic-mode

> When enabled, processor x2APIC support helps operating systems run more efficiently on high core count configurations and optimizes interrupt distribution in virtualized environments. Enabled mode only provides the support necessary to the operating system. So if you have a multiprocessor system, use virtualization and also know that the operating system that you use supports APIC, you can enable the option.

## patch
https://patchwork.ozlabs.org/project/qemu-devel/patch/1456413312-24063-1-git-send-email-tianyu.lan@intel.com/
