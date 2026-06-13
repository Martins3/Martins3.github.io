内核的参数:
https://www.kernel.org/doc/html/latest/admin-guide/kernel-parameters.html
```txt
        pci=option[,option...]  [PCI,EARLY] various PCI subsystem options.

                                Some options herein operate on a specific device
                                or a set of devices (<pci_dev>). These are
                                specified in one of the following formats:

                                [<domain>:]<bus>:<dev>.<func>[/<dev>.<func>]*
                                pci:<vendor>:<device>[:<subvendor>:<subdevice>]

                                Note: the first format specifies a PCI
                                bus/device/function address which may change
                                if new hardware is inserted, if motherboard
                                firmware changes, or due to changes caused
                                by other kernel parameters. If the
                                domain is left unspecified, it is
                                taken to be zero. Optionally, a path
                                to a device through multiple device/function
                                addresses can be specified after the base
                                address (this is more robust against
                                renumbering issues).  The second format
                                selects devices using IDs from the
                                configuration space which may match multiple
                                devices in the system.

                earlydump       dump PCI config space before the kernel
                                changes anything
                off             [X86] don't probe for the PCI bus
                bios            [X86-32] force use of PCI BIOS, don't access
                                the hardware directly. Use this if your machine
                                has a non-standard PCI host bridge.
                nobios          [X86-32] disallow use of PCI BIOS, only direct
                                hardware access methods are allowed. Use this
                                if you experience crashes upon bootup and you
                                suspect they are caused by the BIOS.
                conf1           [X86] Force use of PCI Configuration Access
                                Mechanism 1 (config address in IO port 0xCF8,
                                data in IO port 0xCFC, both 32-bit).
                conf2           [X86] Force use of PCI Configuration Access
                                Mechanism 2 (IO port 0xCF8 is an 8-bit port for
                                the function, IO port 0xCFA, also 8-bit, sets
                                bus number. The config space is then accessed
                                through ports 0xC000-0xCFFF).
                                See http://wiki.osdev.org/PCI for more info
                                on the configuration access mechanisms.
                noaer           [PCIE] If the PCIEAER kernel config parameter is
                                enabled, this kernel boot option can be used to
                                disable the use of PCIE advanced error reporting.
                nodomains       [PCI] Disable support for multiple PCI
                                root domains (aka PCI segments, in ACPI-speak).
                nommconf        [X86] Disable use of MMCONFIG for PCI
                                Configuration
                check_enable_amd_mmconf [X86] check for and enable
                                properly configured MMIO access to PCI
                                config space on AMD family 10h CPU
                nomsi           [MSI] If the PCI_MSI kernel config parameter is
                                enabled, this kernel boot option can be used to
                                disable the use of MSI interrupts system-wide.
                noioapicquirk   [APIC] Disable all boot interrupt quirks.
                                Safety option to keep boot IRQs enabled. This
                                should never be necessary.
                ioapicreroute   [APIC] Enable rerouting of boot IRQs to the
                                primary IO-APIC for bridges that cannot disable
                                boot IRQs. This fixes a source of spurious IRQs
                                when the system masks IRQs.
                noioapicreroute [APIC] Disable workaround that uses the
                                boot IRQ equivalent of an IRQ that connects to
                                a chipset where boot IRQs cannot be disabled.
                                The opposite of ioapicreroute.
                biosirq         [X86-32] Use PCI BIOS calls to get the interrupt
                                routing table. These calls are known to be buggy
                                on several machines and they hang the machine
                                when used, but on other computers it's the only
                                way to get the interrupt routing table. Try
                                this option if the kernel is unable to allocate
                                IRQs or discover secondary PCI buses on your
                                motherboard.
                rom             [X86] Assign address space to expansion ROMs.
                                Use with caution as certain devices share
                                address decoders between ROMs and other
                                resources.
                norom           [X86] Do not assign address space to
                                expansion ROMs that do not already have
                                BIOS assigned address ranges.
                nobar           [X86] Do not assign address space to the
                                BARs that weren't assigned by the BIOS.
                irqmask=0xMMMM  [X86] Set a bit mask of IRQs allowed to be
                                assigned automatically to PCI devices. You can
                                make the kernel exclude IRQs of your ISA cards
                                this way.
                pirqaddr=0xAAAAA        [X86] Specify the physical address
                                of the PIRQ table (normally generated
                                by the BIOS) if it is outside the
                                F0000h-100000h range.
                lastbus=N       [X86] Scan all buses thru bus #N. Can be
                                useful if the kernel is unable to find your
                                secondary buses and you want to tell it
                                explicitly which ones they are.
                assign-busses   [X86] Always assign all PCI bus
                                numbers ourselves, overriding
                                whatever the firmware may have done.
                usepirqmask     [X86] Honor the possible IRQ mask stored
                                in the BIOS $PIR table. This is needed on
                                some systems with broken BIOSes, notably
                                some HP Pavilion N5400 and Omnibook XE3
                                notebooks. This will have no effect if ACPI
                                IRQ routing is enabled.
                noacpi          [X86] Do not use ACPI for IRQ routing
                                or for PCI scanning.
                use_crs         [X86] Use PCI host bridge window information
                                from ACPI.  On BIOSes from 2008 or later, this
                                is enabled by default.  If you need to use this,
                                please report a bug.
                nocrs           [X86] Ignore PCI host bridge windows from ACPI.
                                If you need to use this, please report a bug.
                use_e820        [X86] Use E820 reservations to exclude parts of
                                PCI host bridge windows. This is a workaround
                                for BIOS defects in host bridge _CRS methods.
                                If you need to use this, please report a bug to
                                <linux-pci@vger.kernel.org>.
                no_e820         [X86] Ignore E820 reservations for PCI host
                                bridge windows. This is the default on modern
                                hardware. If you need to use this, please report
                                a bug to <linux-pci@vger.kernel.org>.
                routeirq        Do IRQ routing for all PCI devices.
                                This is normally done in pci_enable_device(),
                                so this option is a temporary workaround
                                for broken drivers that don't call it.
                skip_isa_align  [X86] do not align io start addr, so can
                                handle more pci cards
                noearly         [X86] Don't do any early type 1 scanning.
                                This might help on some broken boards which
                                machine check when some devices' config space
                                is read. But various workarounds are disabled
                                and some IOMMU drivers will not work.
                bfsort          Sort PCI devices into breadth-first order.
                                This sorting is done to get a device
                                order compatible with older (<= 2.4) kernels.
                nobfsort        Don't sort PCI devices into breadth-first order.
                pcie_bus_tune_off       Disable PCIe MPS (Max Payload Size)
                                tuning and use the BIOS-configured MPS defaults.
                pcie_bus_safe   Set every device's MPS to the largest value
                                supported by all devices below the root complex.
                pcie_bus_perf   Set device MPS to the largest allowable MPS
                                based on its parent bus. Also set MRRS (Max
                                Read Request Size) to the largest supported
                                value (no larger than the MPS that the device
                                or bus can support) for best performance.
                pcie_bus_peer2peer      Set every device's MPS to 128B, which
                                every device is guaranteed to support. This
                                configuration allows peer-to-peer DMA between
                                any pair of devices, possibly at the cost of
                                reduced performance.  This also guarantees
                                that hot-added devices will work.
                cbiosize=nn[KMG]        The fixed amount of bus space which is
                                reserved for the CardBus bridge's IO window.
                                The default value is 256 bytes.
                cbmemsize=nn[KMG]       The fixed amount of bus space which is
                                reserved for the CardBus bridge's memory
                                window. The default value is 64 megabytes.
                resource_alignment=
                                Format:
                                [<order of align>@]<pci_dev>[; ...]
                                Specifies alignment and device to reassign
                                aligned memory resources. How to
                                specify the device is described above.
                                If <order of align> is not specified,
                                PAGE_SIZE is used as alignment.
                                A PCI-PCI bridge can be specified if resource
                                windows need to be expanded.
                                To specify the alignment for several
                                instances of a device, the PCI vendor,
                                device, subvendor, and subdevice may be
                                specified, e.g., 12@pci:8086:9c22:103c:198f
                                for 4096-byte alignment.
                ecrc=           Enable/disable PCIe ECRC (transaction layer
                                end-to-end CRC checking). Only effective if
                                OS has native AER control (either granted by
                                ACPI _OSC or forced via "pcie_ports=native")
                                bios: Use BIOS/firmware settings. This is the
                                the default.
                                off: Turn ECRC off
                                on: Turn ECRC on.
                hpiosize=nn[KMG]        The fixed amount of bus space which is
                                reserved for hotplug bridge's IO window.
                                Default size is 256 bytes.
                hpmmiosize=nn[KMG]      The fixed amount of bus space which is
                                reserved for hotplug bridge's MMIO window.
                                Default size is 2 megabytes.
                hpmmioprefsize=nn[KMG]  The fixed amount of bus space which is
                                reserved for hotplug bridge's MMIO_PREF window.
                                Default size is 2 megabytes.
                hpmemsize=nn[KMG]       The fixed amount of bus space which is
                                reserved for hotplug bridge's MMIO and
                                MMIO_PREF window.
                                Default size is 2 megabytes.
                hpbussize=nn    The minimum amount of additional bus numbers
                                reserved for buses below a hotplug bridge.
                                Default is 1.
                realloc=        Enable/disable reallocating PCI bridge resources
                                if allocations done by BIOS are too small to
                                accommodate resources required by all child
                                devices.
                                off: Turn realloc off
                                on: Turn realloc on
                realloc         same as realloc=on
                noari           do not use PCIe ARI.
                noats           [PCIE, Intel-IOMMU, AMD-IOMMU]
                                do not use PCIe ATS (and IOMMU device IOTLB).
                pcie_scan_all   Scan all possible PCIe devices.  Otherwise we
                                only look for one device below a PCIe downstream
                                port.
                big_root_window Try to add a big 64bit memory window to the PCIe
                                root complex on AMD CPUs. Some GFX hardware
                                can resize a BAR to allow access to all VRAM.
                                Adding the window is slightly risky (it may
                                conflict with unreported devices), so this
                                taints the kernel.
                disable_acs_redir=<pci_dev>[; ...]
                                Specify one or more PCI devices (in the format
                                specified above) separated by semicolons.
                                Each device specified will have the PCI ACS
                                redirect capabilities forced off which will
                                allow P2P traffic between devices through
                                bridges without forcing it upstream. Note:
                                this removes isolation between devices and
                                may put more devices in an IOMMU group.
                config_acs=
                                Format:
                                <ACS flags>@<pci_dev>[; ...]
                                Specify one or more PCI devices (in the format
                                specified above) optionally prepended with flags
                                and separated by semicolons. The respective
                                capabilities will be enabled, disabled or
                                unchanged based on what is specified in
                                flags.

                                ACS Flags is defined as follows:
                                  bit-0 : ACS Source Validation
                                  bit-1 : ACS Translation Blocking
                                  bit-2 : ACS P2P Request Redirect
                                  bit-3 : ACS P2P Completion Redirect
                                  bit-4 : ACS Upstream Forwarding
                                  bit-5 : ACS P2P Egress Control
                                  bit-6 : ACS Direct Translated P2P
                                Each bit can be marked as:
                                  '0' – force disabled
                                  '1' – force enabled
                                  'x' – unchanged
                                For example,
                                  pci=config_acs=10x@pci:0:0
                                would configure all devices that support
                                ACS to enable P2P Request Redirect, disable
                                Translation Blocking, and leave Source
                                Validation unchanged from whatever power-up
                                or firmware set it to.

                                Note: this may remove isolation between devices
                                and may put more devices in an IOMMU group.
                force_floating  [S390] Force usage of floating interrupts.
                nomio           [S390] Do not use MIO instructions.
                norid           [S390] ignore the RID field and force use of
                                one PCI domain per PCI function
                notph           [PCIE] If the PCIE_TPH kernel config parameter
                                is enabled, this kernel boot option can be used
                                to disable PCIe TLP Processing Hints support
                                system-wide.
```

# pci

- https://www.cnblogs.com/LoyenWang/p/14165852.html
- https://www.cnblogs.com/LoyenWang/p/14209318.html

- [ ] 内核文档 : [^3]
- [ ] https://www.kernel.org/doc/Documentation/PCI/pci.txt
- [ ] https://sites.google.com/site/pinczakko/pinczakko-s-guide-to-award-bios-reverse-engineering
- https://resources.infosecinstitute.com/topic/system-address-map-initialization-in-x86x64-architecture-part-1-pci-based-systems/
- https://github.com/intel/nemu/wiki/ACPI-PCI-discovery-hotplug

- https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-bus-pci ，例如 remove_id 和 unbind 是啥区别?

## ref
- [【原创】Linux PCI 驱动框架分析（一）](https://www.cnblogs.com/LoyenWang/p/14165852.html)
- [pcie-lat](https://github.com/andre-richter/pcie-lat) : Linux 内核延迟测试工具
- [osdev : pcie](https://wiki.osdev.org/PCI_Express)
- [osdev : pci](https://wiki.osdev.org/PCI)
- [System address map initialization in x86/x64 architecture part 1: PCI-based systems](https://resources.infosecinstitute.com/topic/system-address-map-initialization-in-x86x64-architecture-part-1-pci-based-systems/) :star:
- [System address map initialization in x86/x64 architecture part 2: PCI express-based systems](https://resources.infosecinstitute.com/topic/system-address-map-initialization-x86x64-architecture-part-2-pci-express-based-systems/) :star:
- [PCIe发展史，你就算没见过也一定听过它，今天一起聊聊这个接口](https://www.bilibili.com/video/BV1sx4y147Ro)

https://liujunming.top/2019/11/24/Introduction-to-PCIe-Access-Control-Services/

## 硬件设计
- https://github.com/ljgibbslf/Chinese-Translation-of-PCI-Express-Technology-
  - 这个书的英文原版很不错，值得一读

- https://blog.chinaaet.com/justlxy/p/5100053328

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
