## mdev 中的中断如何解决的?

## qemu 中为什么写 piix3 最后会到到，这个路径的确奇怪啊
- clone3
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - kvm_cpu_exec
          - kvm_handle_io
            - address_space_rw
              - address_space_write
                - flatview_write
                  - flatview_write_continue
                    - memory_region_dispatch_write
                      - access_with_adjusted_size
                        - memory_region_write_accessor
                          - piix3_write_config
                            - piix3_write_config
                              - pci_bus_fire_intx_routing_notifier
                                - vfio_intx_routing_notifier
                                  - vfio_intx_update
                                    - vfio_intx_eoi

## 需要做的实验

amd 做 itre 的地方 : modify_irte_ga

想不到这个想法完全错误，两个
```txt
echo $(uuidgen) | sudo tee /sys/devices/virtual/mdpy/mdpy/mdev_supported_types/mdpy-vga/create
./iova_stress -g 16
```


## hygon 环境调试一下，思路完全乱掉了
为什么 hygon 完全无法调用到:
```txt
sudo perf trace -e kvm:kvm_pi_irte_update
```
连 irq_set_vcpu_affinity 也不会调用

avic_pi_update_irte 该能调用吧

vmx_pi_update_irte  也不会，看来我是完全没理解哦，似乎需要加强一下对于 svm 的理解了

这路径咋就在 intel 上有问题啊?
- kvm_irqfd
  - kvm_irqfd_assign
    - kvm_arch_irq_bypass_add_producer
    - kvm_arch_irq_bypass_del_producer
    - kvm_arch_update_irqfd_routing
      - vmx_pi_update_irte : `static struct kvm_x86_ops vmx_x86_ops;` 的注册内容
        - irq_set_vcpu_affinity

(TODO : 这里补充说明下，为什么路径修改了)

## AMD 的 vmx_deliver_posted_interrupt 在什么地方?

## 继续的内容

- noiommu 如何快速搭建一下环境 (2day )
	- 确认是没有 iommu 才可以吗?
	- noiommu 的中断如何解决的？
- 利用 mdev 完整一个更高的视角的分析 (2 day)
- 重新回到 hct ，将之前的内容都清理掉 ( 3 day)

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
