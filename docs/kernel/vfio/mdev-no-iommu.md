T4 又调用了 vfio_iommu_type1_ioctl ，
然后又没有调用 amd_iommu_map ，如何理解?


使用  perf ftrace -G vfio_iommu_type1_ioctl
调查，这是在 T4 下的结果:
```txt
  76)               |  vfio_iommu_type1_ioctl [vfio_iommu_type1]() {
  76)               |    vfio_dma_do_map [vfio_iommu_type1]() {
  76)   0.190 us    |      mutex_lock();
  76)               |      kmem_cache_alloc_trace() {
  76)   0.190 us    |        should_failslab();
  76)   1.070 us    |      }
  76)               |      capable() {
  76)               |        security_capable() {
  76)   0.490 us    |          cap_capable();
  76)   1.620 us    |        }
  76)   2.220 us    |      }
  76)   0.180 us    |      mutex_unlock();
  76)   4.930 us    |    }
  76)   5.720 us    |  }
```

而在 L20 GPU 中，结果为:
```txt
  62)               |  vfio_iommu_type1_ioctl [vfio_iommu_type1]() {
  62)               |    vfio_dma_do_map [vfio_iommu_type1]() {
  62)   0.190 us    |      mutex_lock();
  62)               |      kmem_cache_alloc_trace() {
  62)   0.190 us    |        should_failslab();
  62)   0.590 us    |      }
  62)               |      capable() {
  62)               |        security_capable() {
  62)   0.200 us    |          cap_capable();
  62)   1.090 us    |        }
  62)   1.680 us    |      }
  62)               |      vfio_pin_map_dma [vfio_iommu_type1]() {
  62)               |        __get_free_pages() {
  62)   2.510 us    |          alloc_pages();
  62)   2.860 us    |        }
  62)               |        vfio_pin_pages_remote [vfio_iommu_type1]() {
  62)   9.170 us    |          vaddr_get_pfns.constprop.0 [vfio_iommu_type1]();
  62)   0.590 us    |          is_invalid_reserved_pfn [vfio_iommu_type1]();
  62)   1.860 us    |          vaddr_get_pfns.constprop.0 [vfio_iommu_type1]();
  62)   0.270 us    |          is_invalid_reserved_pfn [vfio_iommu_type1]();
  62)   1.800 us    |          vaddr_get_pfns.constprop.0 [vfio_iommu_type1]();
  62)   0.270 us    |          is_invalid_reserved_pfn [vfio_iommu_type1]();
  62)   1.800 us    |          vaddr_get_pfns.constprop.0 [vfio_iommu_type1]();
  62)   0.270 us    |          is_invalid_reserved_pfn [vfio_iommu_type1]();
  62)   1.790 us    |          vaddr_get_pfns.constprop.0 [vfio_iommu_type1]();
```

在主线代码中，极有可能，其中的代码是这里的 vfio_dma_do_map 中这里的:
```txt
	/* Don't pin and map if container doesn't contain IOMMU capable domain*/
	if (list_empty(&iommu->domain_list))
		dma->size = size;
	else
		ret = vfio_pin_map_dma(iommu, dma, size);
```

但是我们现在没有资源（机器 和 时间） 来调试，到底是如何初始化的，最后让有的 container 是没有 iommu 的。

T4 GPU 是可以不用 iommu 的， 而 L20 是使用 iommu 的

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
