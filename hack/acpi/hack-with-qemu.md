# hack with qemu
- `-acpitable`
- [ ] https://gist.github.com/mcastelino/47c1fcec1b364d5e82d42e6f341eba78 : 调试的方法

info mtree
```
      0000000000000600-0000000000000603 (prio 0, i/o): acpi-evt
      0000000000000604-0000000000000605 (prio 0, i/o): acpi-cnt
      0000000000000608-000000000000060b (prio 0, i/o): acpi-tmr

    000000000000ae00-000000000000ae17 (prio 0, i/o): acpi-pci-hotplug
    000000000000af00-000000000000af0b (prio 0, i/o): acpi-cpu-hotplug
    000000000000afe0-000000000000afe3 (prio 0, i/o): acpi-gpe0
```


[^1]: https://stackoverflow.com/questions/60137506/qemu-support-for-acpi-2-0
