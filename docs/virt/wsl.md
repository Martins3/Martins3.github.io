# 应该测试一下 wsl 的东西

## wsl 居然开源了?
https://www.reddit.com/r/linux/comments/1kqgwr9/the_windows_subsystem_for_linux_is_now_open_source/
https://news.ycombinator.com/item?id=44031385

https://github.com/microsoft/WSL
https://github.com/microsoft/wslg

原来一个 wsl 命令就可以了，太酷了

## 如何开启 wsl
- 首先，更新系统:
- 然后参考这个:
  - https://ubuntu.com/tutorials/install-ubuntu-on-wsl2-on-windows-11-with-gui-support#2-install-wsl

- wsl -l -v : 检查 wsl 的版本

参考内容:
- https://www.huanlintalk.com/2020/02/wsl-2-installation.html

## wsl 辅助小工具
- https://github.com/wslutilities/wslu

## wsl 内部原理探索
- https://jia.je/os/2023/10/03/wsl2-internals/#systemvhd

https://pit-ray.github.io/win-vind/

## 这的确封装的好啊
```txt
PS C:\Users\97936\data\vn> wsl --install FedoraLinux-42
正在下载: Fedora Linux 42
正在安装: Fedora Linux 42
已成功安装分发。可以通过 “wsl.exe -d FedoraLinux-42” 启动它
正在启动 FedoraLinux-42...
wsl: 检测到 localhost 代理配置，但未镜像到 WSL。NAT 模式下的 WSL 不支持 localhost 代理。
Please create a default user account. The username does not need to match your Windows username.
For more information visit: https://aka.ms/wslusers
Enter new UNIX username:
```

wsl --list --online


wsl -d FedoraLinux-42

wsl --set-default FedoraLinux-42
wsl --manage FedoraLinux-42 --set-default-user martins3
wsl # 然后这个就可以了
wsl --user martins3 -d FedoraLinux-42


```txt
wsl: 检测到 localhost 代理配置，但未镜像到 WSL。NAT 模式下的 WSL 不支持 localhost 代理。
<3>WSL (130 - Relay) ERROR: CreateProcessParseCommon:986: getpwnam(martins4) failed 0
找不到用户。
错误代码: Wsl/WSL_E_USER_NOT_FOUND
```

哦，
mount 看到的用的也是 9p ，但是主要是各种驱动:
```txt
none on /usr/lib/modules/6.6.87.2-microsoft-standard-WSL2 type overlay (rw,nosuid,nodev,noatime,lowerdir=/modules,upperdir=/lib/modules/6.6.87.2-microsoft-standard-WSL2/rw/upper,workdir=/lib/modules/6.6.87.2-microsoft-standard-WSL2/rw/work,uuid=on)
none on /mnt/wsl type tmpfs (rw,relatime)
drivers on /usr/lib/wsl/drivers type 9p (ro,nosuid,nodev,noatime,aname=drivers;fmask=222;dmask=222,cache=5,access=client,msize=65536,trans=fd,rfd=8,wfd=8)
```

磁盘还是普通的磁盘:
```txt
/dev/sdd on /mnt/wslg/distro type ext4 (ro,relatime,discard,errors=remount-ro,data=ordered)
[user@martins3 distro]$ lsblk
NAME MAJ:MIN RM   SIZE RO TYPE MOUNTPOINTS
sda    8:0    0 388.4M  1 disk
sdb    8:16   0   186M  1 disk
sdc    8:32   0     4G  0 disk [SWAP]
sdd    8:48   0     1T  0 disk /mnt/wslg/distro
                               /
sde    8:64   0     1T  0 disk
```

和 hyper-v 一样的网络配置:
```txt
[user@martins3 ~]$ ip a
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
    inet 10.255.255.254/32 brd 10.255.255.254 scope global lo
       valid_lft forever preferred_lft forever
    inet6 ::1/128 scope host proto kernel_lo
       valid_lft forever preferred_lft forever
2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1280 qdisc mq state UP group default qlen 1000
    link/ether 00:15:5d:24:3c:03 brd ff:ff:ff:ff:ff:ff
    altname enx00155d243c03
    inet 172.26.166.97/20 brd 172.26.175.255 scope global eth0
       valid_lft forever preferred_lft forever
    inet6 fe80::215:5dff:fe24:3c03/64 scope link proto kernel_ll
       valid_lft forever preferred_lft forever
```

## 当执行 wsl 的时候，使用的是什么网络呢?
估计还是 nat 了

## 诡异的代理穿透问题

居然是 podman 虚拟机的问题，换成 fedora 虚拟机就没有问题了

不知道
```txt
unpacking 'https://github.com/NixOS/nixpkgs/archive/53951c0c1444e500585205e8b2510270b2ad188f.tar.gz' into the Git cache...
warning: error: unable to download 'https://github.com/NixOS/nixpkgs/archive/53951c0c1444e500585205e8b2510270b2ad188f.tar.gz': Could not connect to server (7) Failed to connect to 127.0.0.1 port 7897 after 0 ms: Could not connect to server; retrying in 329 ms
warning: error: unable to download 'https://github.com/NixOS/nixpkgs/archive/53951c0c1444e500585205e8b2510270b2ad188f.tar.gz': Could not connect to server (7) Failed to connect to 127.0.0.1 port 7897 after 0 ms: Could not connect to server; retrying in 535 ms
warning: error: unable to download 'https://github.com/NixOS/nixpkgs/archive/53951c0c1444e500585205e8b2510270b2ad188f.tar.gz': Could not connect to server (7) Failed to connect to 127.0.0.1 port 7897 after 0 ms: Could not connect to server; retrying in 1134 ms
warning: error: unable to download 'https://github.com/NixOS/nixpkgs/archive/53951c0c1444e500585205e8b2510270b2ad188f.tar.gz': Could not connect to server (7) Failed to connect to 127.0.0.1 port 7897 after 0 ms: Could not connect to server; retrying in 2115 ms
```

## 可以写一个 wsl 和 ubuntu 的 multipass 的对比 blog


## 什么东西是 relay

才意识到，这是一个完全不同的东西:
难怪是不需要密码的:

```txt
systemd─┬─NetworkManager───3*[{NetworkManager}]
        ├─2*[agetty]
        ├─dbus-broker-lau───dbus-broker
        ├─init-systemd(Fe─┬─SessionLeader───Relay(233)───bash───bash───sudo───sudo───yum───{yum}
        │                 ├─SessionLeader───Relay(340)───bash───pstree
        │                 ├─init───{init}
        │                 ├─login───bash
        │                 └─{init-systemd(Fe}
        ├─2*[systemd───(sd-pam)]
        ├─systemd-homed
        ├─systemd-hostnam
        ├─systemd-journal
        ├─systemd-logind
        ├─systemd-oomd
        ├─systemd-resolve
        ├─systemd-udevd
        └─systemd-userdbd───3*[systemd-userwor]
```
## 原来 wsl 有自己的半虚拟化支持

改动不多，但是:

https://github.com/microsoft/WSL2-Linux-Kernel

drivers/hv/dxgkrnl/

```txt
[  705.530923] io scheduler bfq registered
[  705.625991] ACPI: AC: AC Adapter [AC1] (on-line)
[  705.626185] ACPI: battery: Slot [BAT1] (battery present)
```

https://wsl.dev/

## wsl 测试，在虚拟机中 git clone linux kernel ，必然导致物理机宕机

关闭 嵌套虚拟化应该可以解决这个问题

## 偶遇到这个错误了

```txt
[    8.059170] io scheduler bfq registered
[    8.161826] ACPI: AC: AC Adapter [AC1] (on-line)
[    8.163436] ACPI: battery: Slot [BAT1] (battery present)
[    8.521422] kvm_amd: TSC scaling supported
[    8.521640] kvm_amd: Nested Virtualization enabled
[    8.521804] kvm_amd: Nested Paging enabled
[    8.521935] kvm_amd: kvm_amd: Hyper-V enlightened NPT TLB flush enabled
[    8.522137] kvm_amd: kvm_amd: Hyper-V Direct TLB Flush enabled
[    8.522361] kvm_amd: Virtual VMLOAD VMSAVE supported
[   16.929013] WSL (378) ERROR: CheckConnection: getaddrinfo() failed: -3
[   17.583950] WSL (2 - init-systemd(FedoraLinux-42)) ERROR: WaitForBootProcess:3393: /sbin/init failed to start within 10000ms
[   21.934798] WSL (378) ERROR: CheckConnection: getaddrinfo() failed: -3
[   48.450888] hv_balloon: Max. dynamic memory size: 16224 MB
```

难道有一个 WSL 进程么?

## wsl usb manager
https://github.com/nickbeth/wsl-usb-manager

wsl manager : 其实功能很简单
winget install Bostrot.WSLManager

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
