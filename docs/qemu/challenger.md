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

rust-vmm 的生态:
<img width="1428" height="1494" alt="Image" src="https://github.com/user-attachments/assets/297898d6-4dc7-4df8-96b6-0ddaf3db1c4c" />

## 不开源的
- https://cloud.google.com/blog/products/gcp/7-ways-we-harden-our-kvm-hypervisor-at-google-cloud-security-in-plaintext

- aws 连 kvm 都有替换
- https://www.reddit.com/r/vmware/comments/1983lti/what_hypervisor_does_amazon_cloud_use/
- https://news.ycombinator.com/item?id=15814161
  - > Turns out Nitro is not only an improved KVM but also work exclusively with their Nitro Custom Silicon

asure 自然就不用说了

## 挑战者联盟
https://github.com/rust-vmm

https://github.com/microsoft/openvmm : 2024-10-13 开源的，当然，是侧重于 windows 的，40 万行左右，还是
可以研究一下的。
  - https://news.ycombinator.com/item?id=41866742
  - https://techcommunity.microsoft.com/t5/windows-os-platform-blog/openhcl-the-new-open-source-paravisor/ba-p/4273172


## 看看
- https://github.com/ubicloud/ubicloud

##
3. Chrome OS Virtual Machine Monitor : https://news.ycombinator.com/item?id=15346269

## 

https://github.com/quickemu-project/quickemu

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
