## btf
- https://www.ebpf.top/post/kernel_btf/
- https://nakryiko.com/posts/btf-dedup/

## 问题
- btf 在那里，有具体的文件吗?
  - 难道是 : /sys/kernel/btf
- 如何生成的?

## pahole
https://mp.weixin.qq.com/s/uTGRjky09PuQej566P2HVA : 似乎 btf 是从 dwarf 中生成的

https://stackoverflow.com/questions/70093863/linux-btf-bpftool-failed-to-get-ehdr-from-sys-kernel-btf-vmlinux

## vmlinux.h 如何生成的，作用如何?
参考 : https://www.grant.pizza/blog/vmlinux-header/

![](https://www.grant.pizza/libbpf/vmlinux.png)

> Since the vmlinux.h file is generated from your installed kernel, your bpf program could break if you try to run it on another machine without recompiling if it’s running a different kernel version.
> This is because, from version to version, definitions of internal structs change within the linux source code.
>
> However, by using libbpf, you can enable something called “CO:RE” or “Compile once, run everywhere”.
> There are macros defined in libbpf (such as BPF_CORE_READ) that will analyze what fields you’re trying to access in the types that are defined in your vmlinux.h.
> If the field you want to access has been moved within the struct definition that the running kernel uses,
> the macro/helpers will find it for you. It doesn’t matter if you compile your bpf program with the vmlinux.h file you generated from your own kernel and then ran on a different one.

简单来说，构建 bpf 程序的时候，需要

- [ ] 如何解决 vmlinux.h 如何在不同的 kernel 版本的，就是 CO:RE ，但是 libbpf 如何解决的，是个迷?
  - 猜测，libbpf 运行的时候，还是需要知道运行的 kernel 的 header 是什么?

## /sys/kernel/btf
https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-kernel-btf

> Contains BTF type information and related data for kernel and kernel modules.

包含了当前 kernel 的 btf 的信息 : /sys/kernel/btf

- https://github.com/aquasecurity/btfhub/blob/main/docs/how-to-use-pahole.md
  - 不明觉厉啊

```sh
bpftool btf dump file /sys/kernel/btf/vmlinux format raw
```

```sh
pahole /sys/kernel/btf/vmlinux
```
- [ ] 这个生成的结果 vmlinux.h 啥关系

### /sys/kernel/btf 难道和 vmlinux.h 中的内容不是重复的吗?

是的，在 guest 中
```sh
bpftool btf dump file /sys/kernel/btf/vmlinux format c
```
生成的结果和
```sh
pahole /sys/kernel/btf/vmlinux
```
的内容相同。

### [ ] 那么问题是，为什么 libbpf-bootstrap 一个 vmlinux.h ，不可以直接现场生成吗?


https://lwn.net/Articles/818714/


## NIXOS 下构建内核模块的时候，存在这个警告，这个是为什么?
```txt
Skipping BTF generation for virtio/virtio-dummy.ko due to unavailability of vmlinux
  BTF [M] mini/mini.ko
Skipping BTF generation for mini/mini.ko due to unavailability of vmlinux
  BTF [M] simplefs/simplefs.ko
Skipping BTF generation for simplefs/simplefs.ko due to unavailability of vmlinux
  BTF [M] martins3.ko
Skipping BTF generation for martins3.ko due to unavailability of vmlinux
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
