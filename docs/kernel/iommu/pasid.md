## iotlb 什么时候 flush

从 qemu 看，有 pasid 和 iotlb 的关联很强
```txt
@[
    qi_flush_iotlb+5
    domain_context_mapping_one+730
    pci_for_each_dma_alias+66
    intel_iommu_attach_device+347
    __iommu_device_set_domain+110
    __iommu_group_set_domain_internal+110
    iommu_attach_group+125
    vfio_iommu_type1_attach_group+405
    vfio_fops_unl_ioctl+589
    __se_sys_ioctl+107
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 1
@[
    qi_flush_iotlb+5
    intel_context_flush_present+116
    domain_context_clear_one_cb+28
    pci_for_each_dma_alias+66
    device_block_translation+203
    intel_iommu_attach_device+34
    __iommu_device_set_domain+110
    __iommu_group_set_domain_internal+110
    iommu_attach_group+125
    vfio_iommu_type1_attach_group+405
    vfio_fops_unl_ioctl+589
    __se_sys_ioctl+107
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 1
@[
    qi_flush_iotlb+5
    intel_context_flush_present+116
    domain_context_clear_one_cb+28
    pci_for_each_dma_alias+66
    device_block_translation+203
    blocking_domain_attach_dev+17
    __iommu_device_set_domain+110
    __iommu_group_set_domain_internal+110
    __iommu_take_dma_ownership+420
    iommu_group_claim_dma_owner+64
    vfio_container_attach_group+140
    vfio_group_fops_unl_ioctl+1183
    __se_sys_ioctl+107
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 1
```

## 那么可以直接 enable pasid 吗?
pasid

pci_enable_pasid

- 至少 intel 是通过 qemu 实现过的: hw/i386/intel_iommu.c

### pasid 模式初步
```txt
commit 1b2b12376c8a513a0c7b5e3b8ea702038d3d7db5
Author: Jason Wang <jasowang@redhat.com>
Date:   Fri Oct 28 14:14:36 2022 +0800

    intel-iommu: PASID support

    This patch introduce ECAP_PASID via "x-pasid-mode". Based on the
    existing support for scalable mode, we need to implement the following
    missing parts:

    1) tag VTDAddressSpace with PASID and support IOMMU/DMA translation
       with PASID
    2) tag IOTLB with PASID
    3) PASID cache and its flush
    4) PASID based IOTLB invalidation

    For simplicity PASID cache is not implemented so we can simply
    implement the PASID cache flush as a no and leave it to be implemented
    in the future. For PASID based IOTLB invalidation, since we haven't
    had L1 stage support, the PASID based IOTLB invalidation is not
    implemented yet. For PASID based device IOTLB invalidation, it
    requires the support for vhost so we forbid enabling device IOTLB when
    PASID is enabled now. Those work could be done in the future.

    Note that though PASID based IOMMU translation is ready but no device
    can issue PASID DMA right now. In this case, PCI_NO_PASID is used as
    PASID to identify the address without PASID. vtd_find_add_as() has
    been extended to provision address space with PASID which could be
    utilized by the future extension of PCI core to allow device model to
    use PASID based DMA translation.

    This feature would be useful for:

    1) prototyping PASID support for devices like virtio
    2) future vPASID work
    3) future PRS and vSVA work

    Reviewed-by: Peter Xu <peterx@redhat.com>
    Signed-off-by: Jason Wang <jasowang@redhat.com>
    Message-Id: <20221028061436.30093-5-jasowang@redhat.com>
    Reviewed-by: Michael S. Tsirkin <mst@redhat.com>
    Signed-off-by: Michael S. Tsirkin <mst@redhat.com>
```

```c
static const Property vtd_properties[] = {
    DEFINE_PROP_UINT32("version", IntelIOMMUState, version, 0),
    DEFINE_PROP_ON_OFF_AUTO("eim", IntelIOMMUState, intr_eim,
                            ON_OFF_AUTO_AUTO),
    DEFINE_PROP_BOOL("x-buggy-eim", IntelIOMMUState, buggy_eim, false),
    DEFINE_PROP_UINT8("aw-bits", IntelIOMMUState, aw_bits,
                      VTD_HOST_ADDRESS_WIDTH),
    DEFINE_PROP_BOOL("caching-mode", IntelIOMMUState, caching_mode, FALSE),
    DEFINE_PROP_BOOL("x-scalable-mode", IntelIOMMUState, scalable_mode, FALSE),
    DEFINE_PROP_BOOL("snoop-control", IntelIOMMUState, snoop_control, false),
    DEFINE_PROP_BOOL("x-pasid-mode", IntelIOMMUState, pasid, false),
    DEFINE_PROP_BOOL("dma-drain", IntelIOMMUState, dma_drain, true),
    DEFINE_PROP_BOOL("dma-translation", IntelIOMMUState, dma_translation, true),
    DEFINE_PROP_BOOL("stale-tm", IntelIOMMUState, stale_tm, false),
    DEFINE_PROP_END_OF_LIST(),
};
```
## 如何理解 intel pasid
intel_pasid_setup_second_level

## 看看 amd 的 pasid 是什么？

iommu_sva_set_dev_pasid

amd_iommu_set_gcr3

- amd_iommu_attach_device
  - pdev_enable_caps

## pasid 必须和 siov 一起才可以工作吗?

不用的 siov 的时候可以使用 pasid 吗?

## 测试
2. 使用 intel iommu 来调试一下吧，还是那个问题，iommu

## 看看 iommu_group::pasid_array

- iommufd_fault_domain_attach_dev
  - __fault_domain_attach_dev
    - iommu_attach_group_handle

- iommu_sva_bind_device : 这个是唯一的调用路径
  - iommu_attach_device_pasid
    - intel_iommu_set_dev_pasid

所以，什么时候才需要 pasid 的，当一个设备被切分为多个设备使用的时候，需要使用 pasid 来区分

## drivers/iommu/intel/pasid.c

- [PASID Management in KVM](https://kvm-forum.qemu.org/2020/KVM_forum_2020_PASID_MGMT_Yi_Jacob_final.pdf)

Usages :
– PASID in Shared Virtual Addressing (SVA)
– PASID in Intel® Scalable IOV

原来这是两个功能啊

Default PASID of auxiliary domain :
– Each ADI (Assignable Device Interface[1]) has a default PASID
– Assigned once attached to an auxiliary IOMMU domain[1]
– ADIs attached to the same IOMMU domain share the default PASID of the domain
– Programmed to hardware by parent device driver

https://events19.linuxfoundation.org/wp-content/uploads/2017/12/Hardware-Assisted-Mediated-Pass-Through-with-VFIO-Kevin-Tian-Intel.pdf

## pasid 和 siov 是等价的吗?
- https://lpc.events/event/11/contributions/1021/attachments/744/1700/lpc-2021-kernel-svm-jp.pdf

## 4.18 中 intel_iommu_dev_has_feat 里面所有的函数都确认下

```c
static bool
intel_iommu_dev_has_feat(struct device *dev, enum iommu_dev_features feat)
{
	if (feat == IOMMU_DEV_FEAT_AUX) {
		int ret;

		if (!dev_is_pci(dev) || dmar_disabled ||
		    !scalable_mode_support() || !iommu_pasid_support())
			return false;

		ret = pci_pasid_features(to_pci_dev(dev));
		if (ret < 0)
			return false;

		return !!siov_find_pci_dvsec(to_pci_dev(dev));
	}

	return false;
}
```

等等了，所有的东西都需要调查
- [ ] pci_pasid_features : 这个检查一下
- [ ] scalable_mode_support
- [ ] iommu_pasid_support

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
