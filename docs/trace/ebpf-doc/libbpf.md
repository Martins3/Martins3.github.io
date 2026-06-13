## 这个居然意外的好懂
https://libbpf.readthedocs.io/en/latest/libbpf_overview.html

```txt
===================================================================================================
 Language                                Files        Lines         Code     Comments       Blanks
===================================================================================================
 C                                          44        71994        52932         8932        10130
---------------------------------------------------------------------------------------------------
 ./libbpf.c                                           14033        10856         1150         2027
 ./libbpf.c                                           14033        10856         1150         2027
 ./btf.c                                               5496         3606         1139          751
 ./btf.c                                               5496         3606         1139          751
 ./linker.c                                            2945         2159          303          483
 ./linker.c                                            2945         2159          303          483
 ./btf_dump.c                                          2547         1761          491          295
 ./btf_dump.c                                          2547         1761          491          295
 ./relo_core.c                                         1687         1186          296          205
 ./relo_core.c                                         1687         1186          296          205
 ./usdt.c                                              1600         1061          339          200
 ./usdt.c                                              1600         1061          339          200
 ./bpf.c                                               1330         1035           65          230
 ./bpf.c                                               1330         1035           65          230
 ./gen_loader.c                                        1123          871          140          112
 ./gen_loader.c                                        1123          871          140          112
 ./netlink.c                                            922          763            3          156
 ./netlink.c                                            922          763            3          156
 ./ringbuf.c                                            683          496           71          116
 ./ringbuf.c                                            683          496           71          116
 ./features.c                                           613          476           66           71
 ./features.c                                           613          476           66           71
 ./elf.c                                                558          408           70           80
 ./elf.c                                                558          408           70           80
 ./btf_relocate.c                                       519          381           89           49
 ./btf_relocate.c                                       519          381           89           49
 ./libbpf_probes.c                                      465          362           53           50
 ./libbpf_probes.c                                      465          362           53           50
 ./zip.c                                                333          218           55           60
 ./zip.c                                                333          218           55           60
 ./bpf_prog_linfo.c                                     246          179           25           42
 ./bpf_prog_linfo.c                                     246          179           25           42
 ./hashmap.c                                            240          186           10           44
 ./hashmap.c                                            240          186           10           44
 ./nlattr.c                                             195          119           40           36
 ./nlattr.c                                             195          119           40           36
 ./btf_iter.c                                           177          158            6           13
 ./btf_iter.c                                           177          158            6           13
 ./strset.c                                             177          116           31           30
 ./strset.c                                             177          116           31           30
 ./libbpf_errno.c                                        75           50           12           13
 ./libbpf_errno.c                                        75           50           12           13
 ./str_error.c                                           33           19           12            2
 ./str_error.c                                           33           19           12            2
```

### 为什么有这么多的 btf 相关代码

## 实在可以

ubuntu 中可以 libbpf-tools ，从而直接用 : https://github.com/iovisor/bcc/blob/master/libbpf-tools/tcptop.c
```txt
sudo apt install libbpf-tools
```

## libbpf
这里提供了一些有用的链接的:
https://github.com/iovisor/bcc/tree/master/libbpf-tools


- [pingcap : Why We Switched from BCC to libbpf for Linux BPF Performance Analysis](https://pingcap.com/blog/why-we-switched-from-bcc-to-libbpf-for-linux-bpf-performance-analysis)

## https://nakryiko.com/posts/bpf-core-reference-guide/

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
