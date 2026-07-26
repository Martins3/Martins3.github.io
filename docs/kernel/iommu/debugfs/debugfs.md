## intel 的 iommu debugfs

/sys/kernel/debug/iommu/intel 下:
```txt
drwxr-xr-x   2 root root 0 Jan 10 12:38 0000:ff:1e.2
drwxr-xr-x   2 root root 0 Jan 10 12:38 0000:ff:1e.3
drwxr-xr-x   2 root root 0 Jan 10 12:38 0000:ff:1e.4
drwxr-xr-x   2 root root 0 Jan 10 12:38 0000:ff:1e.5
drwxr-xr-x   2 root root 0 Jan 10 12:38 0000:ff:1e.6
drwxr-xr-x   2 root root 0 Jan 10 12:38 0000:ff:1e.7
-rw-r--r--   1 root root 0 Jan 10 12:38 dmar_perf_latency
-r--r--r--   1 root root 0 Jan 10 12:38 dmar_translation_struct
-r--r--r--   1 root root 0 Jan 10 12:38 invalidation_queue
-r--r--r--   1 root root 0 Jan 10 12:38 iommu_regset
-r--r--r--   1 root root 0 Jan 10 12:38 ir_translation_struct
```

具体的设备下的结果为，前提是这个设备的确映射了:
```txt
[root@bogon 0000:00:07.0]# cat domain_translation_struct
Device 0000:00:07.0 with pasid 0 @0x4862ca000
IOVA_PFN                PML5E                   PML4E                   PDPE                    PDE                     PTE
0x00000000ffe17 |       0x0000000000000000      0x0000000000000000      0x00000004e3ccf003      0x00000004e3cd0003      0x00000004b7207003
0x00000000ffe40 |       0x0000000000000000      0x0000000000000000      0x00000004e3ccf003      0x00000004e3cd0003      0x0000000527f55003
0x00000000ffe41 |       0x0000000000000000      0x0000000000000000      0x00000004e3ccf003      0x00000004e3cd0003      0x0000000527f56003
0x00000000ffe42 |       0x0000000000000000      0x0000000000000000      0x00000004e3ccf003      0x00000004e3cd0003      0x0000000527f57003
0x00000000ffe43 |       0x0000000000000000      0x0000000000000000      0x00000004e3ccf003      0x00000004e3cd0003      0x0000000527f58003
```

debugfs 文件的结果在:
./debugfs-intel-dmar_perf_latency.txt
./debugfs-intel-dmar_translation_struct.txt
./debugfs-intel-invalidation_queue.txt
./debugfs-intel-iommu_regset.txt
./debugfs-intel-ir_translation_struct.txt : 是的，这就是 interrupt remaping 相关的东西


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
