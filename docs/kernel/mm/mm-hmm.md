# HMM
<!-- ca502962-e1b8-4bc5-bb20-73236963d730 -->

mm/migrate_device.c

通过测试看看:
https://lore.kernel.org/all/20200422195028.3684-3-rcampbell@nvidia.com/

mm/hmm.c
lib/test_hmm.c
tools/testing/selftests/mm/hmm-tests.c

## TODO ZONE_DEVICE 和 HMM 是什么关系
```txt
CONFIG_ZONE_DEVICE=y
CONFIG_DEVICE_MIGRATION=y
CONFIG_GET_FREE_REGION=y
CONFIG_DEVICE_PRIVATE=y
CONFIG_TEST_HMM=m
CONFIG_ND_PFN=m
CONFIG_NVDIMM_PFN=y
CONFIG_NVDIMM_DAX=y
```
## 是不是 hmm 也可以处理 nvdimm 的?


## hmm
Provide infrastructure and helpers to integrate non-conventional memory (device memory like GPU on board memory) into regular kernel path, with the cornerstone of this being specialized struct page for such memory.
HMM also provides optional helpers for SVM (Share Virtual Memory) [^19]

https://www.kernel.org/doc/html/latest/mm/hmm.html

mm/hmm.c 和 memory tier 有关系吗?


https://developer.nvidia.com/blog/simplifying-gpu-application-development-with-heterogeneous-memory-management/

这是一个东西吗?

https://tallendev.github.io/assets/papers/sc21.pdf

## hmm 的用户都是谁啊

- drivers/gpu/drm/xe/xe_svm.h
- drivers/infiniband/core/umem_odp.c
- drivers/hv/mshv_regions.c
- drivers/gpu/drm/amd/amdkfd/kfd_svm.c
- drivers/gpu/drm/amd/amdgpu/amdgpu_hmm.c
- drivers/gpu/drm/amd/amdgpu/amdgpu_amdkfd_gpuvm.c
- drivers/accel/amdxdna/aie2_ctx.c
- drivers/gpu/drm/drm_gpusvm.c
- drivers/gpu/drm/nouveau/nouveau_svm.c

## 看看那个环境是支持这个的

 6. 检查 NVMe 是否支持 CMB（Controller Memory Buffer）
  nvme id-ctrl /dev/nvme0n1 | grep -i cmb
  nvme id-ctrl /dev/nvme1n1 | grep -i cmb
  nvme id-ctrl /dev/nvme2n1 | grep -i cmb
  作用：CMB 是 NVMe 参与 P2PDMA 的关键硬件能力之一。
  结果：三块 NVMe 都没有 CMB 信息。

Documentation/translations/zh_CN/mm/hmm.rst

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
