# HyperV
[Documentation](https://docs.microsoft.com/en-us/virtualization/hyper-v-on-windows/about/)
[Qemu doc for hyperv](https://github.com/qemu/qemu/blob/master/docs/hyperv.txt)


https://docs.microsoft.com/en-us/virtualization/hyper-v-on-windows/tlfs/nested-virtualization

- [ ] 还有自己的 layout https://docs.microsoft.com/en-us/virtualization/hyper-v-on-windows/tlfs/datatypes/hv_vmx_enlightened_vmcs
    - [ ] /home/maritns3/core/linux/arch/x86/include/asm/hyperv-tlfs.h:struct hv_enlightened_vmcs

- [ ] 这是 intel 的属性啊，为什么需要搭上 hyperv

## Why kvm need hyperv
https://archive.fosdem.org/2019/schedule/event/vai_enlightening_kvm/attachments/slides/2860/export/events/attachments/vai_enlightening_kvm/slides/2860/vkuznets_fosdem2019_enlightening_kvm.pdf

- [ ] 好的，现在为什么需要 hyperv 的道理我大概知道了，但是怎么实现啊 ?

Emulating hardware Interfaces can be slow
- Invent virtualization-friendly
  - (paravirtualized) interfaces!
    - Add support to guest OSes
      - ... but what about proprietary OSes?
        - We can try writing device drivers for such OSes
          - ... but some core features (interrupt handling, timekeeping,...) are not devices
            - Emulate an already supported (proprietary) hypervisor interfaces solving the exact same issues!

## https://kernel.love/hyperv-enlightenment.html

## 启动 windows 的时候会调用到这里: kvm_get_hv_cpuid
