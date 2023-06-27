
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



https://stackoverflow.com/questions/60219639/kernel-error-irq-remapping-doesnt-support-x2apic-mode-disabled-x2apic
