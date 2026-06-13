## 需求
构建 kernel ，从对于 native_apic_msr_eoi 增加 trace 点，然后快速启动，可以吗?

理论上可以，甚至可以完全没有盘的:

## 有趣的东西
https://carjorvaz.com/posts/ipxe-booting-with-nixos/
https://nixos.wiki/wiki/Netboot


## 思考的东西
1. seabios 是如何加载 ipxe 的，或者说，ipxe 是放到哪里的
  - 在物理机中，server 是需要部署 ipxe.iso 的，但是在 QEMU 中似乎直接就有了
  - 可以参考这个: https://gist.github.com/mcastelino/7ab9dba51b0dbb230bd18c448d935312
    - 这个 ipxe 是放到 virtio 网卡的 option rom 中的吗?
  - 看看 QEMU 是如何加载 pc-bios/efi-virtio.rom 的吧
    - src/drivers/net/virtio-net.c 地方

2. 物理机的网卡如何 flash 的 rom ?
  - 从目前网卡的测试来看，发现网卡自动可以知道有没有 tftp server
3. 无法启动的原因是: https://mirrors.aliyun.com/openeuler/openEuler-22.03-LTS/everything/aarch64/images/pxeboot
4. 是不是还有其他的 pxe 实现? 不算那个 gpxe 之类的死掉的项目
5. 测试下 UEFI 的功能，因为物理机中测试 UEFI 是可以加载 ipxe.iso 过来执行的


## TODO
- QEMU 是可以直接模拟的，看看实现的原理
  - https://www.brianlane.com/post/qemu-pxeboot/
- https://blog.haschek.at/2019/build-your-own-datacenter-with-pxe-and-alpine.html
  - 测试一下，使用 nfs 作为基础，整个系统是没有盘的
  - 当然，也可以测试下 nvme tcp
- https://www.dddns.icu/posts/pxe/
  - 参考他的 windows 搭建方法
  - linux 的各种启动参数是哪里来的
    - 例如 https://wiki.alpinelinux.org/wiki/PXE_boot 有 alpine_repo
  - 如何搭建一个 iSCSI server 来着
  - https://ipxe.org/appnote/ubuntu_live
    - 这里的 nfs 入口到底是什么?
- https://wiki.alpinelinux.org/wiki/PXE_boot
  - 配置 nfs 加载模块
1. ubuntu 的网络安装方法
  - https://wiki.ubuntu.com/UEFI/PXE-netboot-install
  - https://discourse.ubuntu.com/t/netbooting-the-live-server-installer/14510
  - fedora 的差不多 : https://docs.fedoraproject.org/en-US/fedora/f36/install-guide/advanced/Network_based_Installations/
- 既然 ipxe 可以直接支持 http 了，为什么还需要这样
  - 怀疑，是只是 ipxe 支持，但是 UEFI 默认携带的只是支持

## 其他材料
- https://netboot.xyz/
  - https://news.ycombinator.com/item?id=41293850
    - 有趣的讨论
- https://networkboot.org/
  1. One of the most interesting features iPXE enables, is to boot a computer without an iSCSI host bus adapter from an iSCSI volume. This is possible, because iPXE implements a full-featured software-based iSCSI initiator.
- [ ] https://fogproject.org/


## 参考这个操作一下

journalctl -u dnsmasq
```txt
Aug 17 16:41:52 bogon dnsmasq-dhcp[3051]: DHCPACK(ens4) 10.0.0.249 58:47:ca:76:2d:a0
Aug 17 16:41:52 bogon dnsmasq-tftp[3051]: file /tftpboot/ipxe.efi not found for 10.0.0.249
Aug 17 16:43:17 bogon dnsmasq-dhcp[3051]: DHCPDISCOVER(ens4) 58:47:ca:76:2d:9f
Aug 17 16:43:17 bogon dnsmasq-dhcp[3051]: DHCPOFFER(ens4) 10.0.0.248 58:47:ca:76:2d:9f
Aug 17 16:43:20 bogon dnsmasq-dhcp[3051]: DHCPREQUEST(ens4) 10.0.0.248 58:47:ca:76:2d:9f
Aug 17 16:43:20 bogon dnsmasq-dhcp[3051]: DHCPACK(ens4) 10.0.0.248 58:47:ca:76:2d:9f
Aug 17 16:43:20 bogon dnsmasq-tftp[3051]: file /tftpboot/ipxe.efi not found for 10.0.0.248 <--- 这里的报错
Aug 17 16:47:30 bogon dnsmasq-dhcp[3051]: DHCPDISCOVER(ens4) 58:47:ca:76:2d:9f
Aug 17 16:47:30 bogon dnsmasq-dhcp[3051]: DHCPOFFER(ens4) 10.0.0.248 58:47:ca:76:2d:9f
Aug 17 16:47:34 bogon dnsmasq-dhcp[3051]: DHCPREQUEST(ens4) 10.0.0.248 58:47:ca:76:2d:9f
```
发现需要做一个重命名才可以的

## 构建的方法
参考: https://github.com/ipxe/ipxe/discussions/961
```sh
make bin-x86_64-efi/ipxe.efi -j32
```

## 看看这个东西
https://mp.weixin.qq.com/s/-9UP3V0Yxe-XQype5_laFg


## 看看这里 netboot 提供的 netboot
https://fedoraproject.org/coreos/download?stream=stable#arches

所以，系统可以就这样直接启动?
- Fedora CoreOS Netboot kernel
- Fedora CoreOS Netboot initramfs
- Fedora CoreOS Netboot rootfs

## flexboot 是什么东西?

## fedeora 提供了一个网络安装的 iso ，可以看看如何使用
https://fedoraproject.org/server/download

## 如果树莓派可以，那么是不是任何机器都可以
https://www.xda-developers.com/booting-raspberry-pi-network-huge-performance-difference/


## 看看这个
https://www.kraxel.org/blog/2021/09/vm-network-boot/

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
