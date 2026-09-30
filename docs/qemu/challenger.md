# QEMU 的挑战者

- https://github.com/intel/nemu

引用自 https://github.com/astro/microvm.nix :

| Hypervisor                                                              | Language | Restrictions                             |
|-------------------------------------------------------------------------|----------|------------------------------------------|
| [qemu](https://www.qemu.org/)                                           | C        |                                          |
| [cloud-hypervisor](https://www.cloudhypervisor.org/)                    | Rust     | no 9p shares                             |
| [firecracker](https://firecracker-microvm.github.io/)                   | Rust     | no 9p/virtiofs shares                    |
| [crosvm](https://chromium.googlesource.com/chromiumos/platform/crosvm/) | Rust     | 9p shares broken                         | 35w 行|
| [kvmtool](https://github.com/kvmtool/kvmtool)                           | C        | no virtiofs shares, no control socket    |
| [stratovirt](https://github.com/openeuler-mirror/stratovirt)            | Rust     | no 9p/virtiofs shares, no control socket |

还有的:
- https://github.com/containers/libkrun
  - https://github.com/containers/krunvm : 其配套的工具
  - https://github.com/AsahiLinux/muvm : 也是配套的，还有 GPU 的支持
    - 不过，这个是 AsahiLinux 开发的，

- https://gitee.com/openeuler/rust_shyper

想不到在 kata 中内部还有一个: dragonball
- https://github.com/kata-containers/kata-containers/tree/main/src/dragonball


- https://github.com/ubicloud/ubicloud
	- https://www.ubicloud.com/blog/cloud-virtualization-red-hat-aws-firecracker-and-ubicloud-internals : 有趣的总结
- Chrome OS Virtual Machine Monitor : https://news.ycombinator.com/item?id=15346269

## 不开源的
- https://cloud.google.com/blog/products/gcp/7-ways-we-harden-our-kvm-hypervisor-at-google-cloud-security-in-plaintext
- aws 连 kvm 都有替换
	- https://www.reddit.com/r/vmware/comments/1983lti/what_hypervisor_does_amazon_cloud_use/
	- https://news.ycombinator.com/item?id=15814161
	- > Turns out Nitro is not only an improved KVM but also work exclusively with their Nitro Custom Silicon

asure 自然就不用说了，必须是 hyper-v 了

## 挑战者联盟: rust-vmm

rust-vmm 的生态:
<img width="1428" height="1494" alt="Image" src="https://github.com/user-attachments/assets/297898d6-4dc7-4df8-96b6-0ddaf3db1c4c" />

- https://github.com/rust-vmm
- https://github.com/microsoft/openvmm : 2024-10-13 开源的，当然，是侧重于 windows 的，40 万行左右，还是
可以研究一下的。
  - https://news.ycombinator.com/item?id=41866742
  - https://techcommunity.microsoft.com/t5/windows-os-platform-blog/openhcl-the-new-open-source-paravisor/ba-p/4273172

## Cloud Hypervisor

https://github.com/cloud-hypervisor/cloud-hypervisor

这个项目的存在，意味着完全没有任何的必要看 kvmtool 的了。

- 无论如何，那么存储是如何进行的
  - 既然可以使用 docker 来构成 rootfs 的话，其实最后必然也是 qcow2 了吧
    - 所以，其将 QEMU 中的什么东西简化掉吗?

- migration 设计的好简单啊

- 是如何使用 vhost 的哇

主要的目录:
- hypervisor
- vmm

### 甚至还有 vhdx 的支持
-  https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-vhdx/83e061f8-f6e2-4de1-91bd-5d518a43d477

## stratovirt
- https://gitee.com/openeuler/stratovirt
  - https://gitee.com/openeuler/stratovirt/wikis ：还挺有意思的
  - 大约 6 万行，和 CloudHppervisor 差不多

我看基本上已经被放弃了，一年才几十个提交

## ai 时代的 agent sanbox 挑战

https://github.com/google/ax : google

https://news.ycombinator.com/item?id=49859112 : deepseek
- https://mp.weixin.qq.com/s/ulYhZjII7e4tz2B-2afiww
- https://mp.weixin.qq.com/s/XsSSwonygjXpezYttW-T-g

腾讯:
https://mp.weixin.qq.com/s/AU8XrQb6wqjrMXF78iT-Zg
https://github.com/TencentCloud/CubeSandbox

2026-09-27 还没仔细看，为什么不去直接使用 firecracker 之类的东西啊

不过我很赞同这个想法， https://news.ycombinator.com/item?id=49879883#49880271

> A new chip solves nothing. Nobody wants to hear this but there is no solution
> for the security risks posed by agents today. You can put it in a sandbox, it
> doesn't make a difference, for it to be useful it inherently needs wide,
> unattended access. Put a human in the loop and you just end up bottlenecking
> it and throwing away any purported productivity gains. Auto mode doesn't
> matter either, it's trivial to trick and for the agent to break out.

sandbox 其实会影响效率，ai 基本上都可以自动的在 /tmp 上创建文件，分析测试，根本不需要
我们来控制。

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
