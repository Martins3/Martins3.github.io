# aarch64 AA64MMFR1
<!-- 47128ea3-090b-4def-b50e-74787a8f88fa -->

```txt
┌───────┬──────────┬──────────────────────────────────────────────────┐
│ 位域  │ 字段     │ 含义                                             │
├───────┼──────────┼──────────────────────────────────────────────────┤
│ 63:60 │ ECBHB    │ Exception-based Cache Bypass Hint Behavior       │
├───────┼──────────┼──────────────────────────────────────────────────┤
│ 59:56 │ CMOW     │ Cache Maintenance OS-visible（此内核未使用）     │
├───────┼──────────┼──────────────────────────────────────────────────┤
│ 55:52 │ TIDCP1   │ Trap IMPLEMENTATION DEFINED functionality at EL0 │
├───────┼──────────┼──────────────────────────────────────────────────┤
│ 51:48 │ nTLBPA   │ TLB permission check at PA（此内核未使用）       │
├───────┼──────────┼──────────────────────────────────────────────────┤
│ 47:44 │ AFP      │ Alternate Floating-point behavior                │
├───────┼──────────┼──────────────────────────────────────────────────┤
│ 43:40 │ HCX      │ Support for HCRX_EL2                             │
├───────┼──────────┼──────────────────────────────────────────────────┤
│ 39:36 │ ETS      │ Enhanced Translation Synchronization             │
├───────┼──────────┼──────────────────────────────────────────────────┤
│ 35:32 │ TWED     │ Delayed Trapping of WFE                          │
├───────┼──────────┼──────────────────────────────────────────────────┤
│ 31:28 │ XNX      │ Execute-never control by Privilege level         │
├───────┼──────────┼──────────────────────────────────────────────────┤
│ 27:24 │ SpecSEI  │ Speculative SEI support                          │
├───────┼──────────┼──────────────────────────────────────────────────┤
│ 23:20 │ PAN      │ Privileged Access Never（PAN/PAN2/PAN3）         │
├───────┼──────────┼──────────────────────────────────────────────────┤
│ 19:16 │ LO       │ LORegion support                                 │
├───────┼──────────┼──────────────────────────────────────────────────┤
│ 15:12 │ HPDS     │ Hierarchical Permission Disables（HPDS/HPDS2）   │
├───────┼──────────┼──────────────────────────────────────────────────┤
│ 11:8  │ VH       │ Virtualization Host Extensions（VHE）            │
├───────┼──────────┼──────────────────────────────────────────────────┤
│ 7:4   │ VMIDBits │ VMID 位数（8 或 16）                             │
├───────┼──────────┼──────────────────────────────────────────────────┤
│ 3:0   │ HAFDBS   │ Hardware Access Flag / Dirty Bit Management      │
└───────┴──────────┴──────────────────────────────────────────────────┘
```

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
