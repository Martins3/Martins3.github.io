## 内核文档
[内核文档](https://www.kernel.org/doc/html/latest/driver-api/vfio.html)

> While the IOMMU may be able to distinguish between devices within the enclosure, the enclosure may not require transactions between devices to reach the IOMMU.

> Examples of this could be anything from a multi-function PCI device with backdoors between functions to a non-PCI-ACS (Access Control Services) capable bridge allowing redirection without reaching the IOMMU. Topology can also play a factor in terms of hiding devices. A PCIe-to-PCI bridge masks the devices behind it, making transaction appear as if from the bridge itself. Obviously IOMMU design plays a major factor as well.

https://docs.redhat.com/en/documentation/red_hat_enterprise_linux/7/html/virtualization_deployment_and_administration_guide/sect-iommu-deep-dive#sect-iommu-deep-dive
> This is the reason why a typical x86 PC will group all conventional-PCI devices together,
> with all of them aliased to the same requester ID, the PCIe-to-PCI bridge.

- https://unix.stackexchange.com/questions/595353/vt-d-support-enabled-but-iommu-groups-are-missing

## [An Introduction to IOMMU Infrastructure in the Linux Kernel](https://lenovopress.lenovo.com/lp1467.pdf)


## 参考
- https://blog.kernel.love/vfio-insight.html
- 1 https://www.openeuler.org/zh/blog/wxggg/2020-11-29-vfio-passthrough-1.html
- 2 https://www.openeuler.org/zh/blog/wxggg/2020-11-29-vfio-passthrough-2.html
- 3 https://www.openeuler.org/zh/blog/wxggg/2020-11-21-iommu-smmu-intro.html
- https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2019/08/31/vfio-passthrough


[^1]: http://www.linux-kvm.org/images/5/54/01x04-Alex_Williamson-An_Introduction_to_PCI_Device_Assignment_with_VFIO.pdf
[^2]: https://www.kernel.org/doc/html/latest/driver-api/uio-howto.html
[^3]: [populate the empty /sys/kernel/iommu_groups](https://unix.stackexchange.com/questions/595353/vt-d-support-enabled-but-iommu-groups-are-missing)


## ccw
- https://www.kernel.org/doc/html/latest/s390/vfio-ccw.html
- https://www.ibm.com/support/knowledgecenter/en/linuxonibm/com.ibm.linux.z.lkdd/lkdd_c_ccwdd.html

https://www.kernel.org/doc/html/latest/driver-api/vfio.html
https://www.kernel.org/doc/html/latest/driver-api/vfio-mediated-device.html
https://zhuanlan.zhihu.com/p/27026590

## https://kernelnote.com/iommu-posted-interrupt-deep-dive.html

iommu 硬件上是通过 capability register(CAP_REG) 当中的PI这个field来表示其是否支持 posted interrupt，具体的格式如下

(不能确认) 不知道这个图哪里搞的

drivers/iommu/intel/irq_remapping.c
```c
		if (boot_cpu_has(X86_FEATURE_CX16))
			intel_irq_remap_ops.capability |= 1 << IRQ_POSTING_CAP;
```

## 回答这两个问题
- https://stackoverflow.com/questions/66937301/what-is-irq-bypass-and-how-to-use-it-in-linux
- https://stackoverflow.com/questions/29461518/interrupt-handling-for-assigned-device-through-vfio
- https://serverfault.com/questions/1077297/ilo4-and-almalinux-centos8-do-not-work-properly

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
