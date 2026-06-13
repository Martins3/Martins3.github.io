## CO:RE
- design : https://nakryiko.com/posts/bpf-portability-and-co-re/
- reference : https://nakryiko.com/posts/bpf-core-reference-guide/

1. bpftrace 对于 CORE 有感知吗?
2. CO:RE 需要 llvm 的支持
```txt
make: Entering directory '/root/bpftool/rpmbuild/BUILD/bpftool-6.8.0/src'
...                        libbfd: [ on  ]
...        disassembler-four-args: [ OFF ]
...                          zlib: [ on  ]
...                        libcap: [ on  ]
...               clang-bpf-co-re: [ OFF ]
```

## 如何理解这里的 kerne.config 做啥的
bcc/libbpf-tools/kernel.config


## 其实这个是一个正规的项目
https://github.com/aquasecurity/btfhub?tab=readme-ov-file

什么 libbpf 是 loader 而不是 bpftool ，两者到地什么关系

> This is achieved by the libbpf loader, a component within the eBPF's loader and verification architecture. The libbpf loader arranges the necessary infrastructure for an eBPF object, including eBPF map creation, code relocation, setting up eBPF probes, managing links, handling their attachments, among others.

## 真的可以 用 btfhub 解决 4.19 的 kernel libbpf 的使用问题!

而且从 https://github.com/aquasecurity/btfhub/blob/main/docs/supported-distros.md
看，连 3.10 kernel 都是支持的。

## 这个库解决了什么问题?
https://github.com/eunomia-bpf/eunomia-bpf

## 那么，是不是如果在一个机器上构建了 libbpf-tools ，在另外一个机器上直接可以用的

## bpf core 到指的是 bpf.o 可以移植吧？

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
