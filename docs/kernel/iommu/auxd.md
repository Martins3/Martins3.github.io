# auxd

### ARM auxd 支持

## 提交历史
- https://patchwork.kernel.org/project/kvm/patch/20190213040301.23021-4-baolu.lu@linux.intel.com/
- https://lore.kernel.org/all/20220216025249.3459465-9-baolu.lu@linux.intel.com/
- https://patchwork.kernel.org/project/linux-arm-kernel/patch/20190610184714.6786-7-jean-philippe.brucker@arm.com/

https://lore.kernel.org/all/20210924155705.4258-11-hch@lst.de/

靠，然后又删除了，这么看来不是的
https://lore.kernel.org/all/20220216025249.3459465-9-baolu.lu@linux.intel.com/

> The aux-domain related interfaces and iommu_ops are not referenced
> anywhere in the tree. We've also reached a consensus to redesign it
> based the new iommufd framework. Remove them to avoid dead code.

https://lore.kernel.org/all/20211014053839.727419-7-baolu.lu@linux.intel.com/#r
https://lore.kernel.org/r/20210929072030.1330225-1-baolu.lu@linux.intel.com
https://lore.kernel.org/r/20211014053839.727419-7-baolu.lu@linux.intel.com

## aux 原始想法的来源
- [Hardware-Assisted Mediated Pass-Through with VFIO](
https://events19.linuxfoundation.org/wp-content/uploads/2017/12/Hardware-Assisted-Mediated-Pass-Through-with-VFIO-Kevin-Tian-Intel.pdf
)

- ADI : Assignable Device Interfaces
- VDCM : Virtual Device Composition Module

- One device can attach to multiple domains
  - A primary domain used for DMA-API
  - Multiple auxillary domains used for mdev instances
- 'aux' is a device attribute instead of domain attribute
  - Same domain may represent as 'primary' to deviceA and 'aux' to deviceB
  - 'primary' vs. 'aux' is decided at domain attach time
  - Device driver enables 'aux' capability on device before attaching domain

## https://patchwork.kernel.org/project/kvm/cover/20190213040301.23021-1-baolu.lu@linux.intel.com/

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
