# 在 Linux kernel 上如何玩游戏
## 笑死
https://www.reddit.com/r/linux/comments/1p74ovx/switched_to_gaming_on_linux_and_can_confirm_its/#lightbox

https://news.ycombinator.com/item?id=46126446

社区的 iso 必须安装一个菜可以的
https://bazzite.gg/

但是开机之后，在物理机中观察到了大量的这种错误，这是预期的吗?
```txt
[2251047.907013] x86/split lock detection: #AC: CPU 24/KVM/3010259 took a split_lock trap at address: 0x5692286f
[2251054.241988] x86/split lock detection: #AC: CPU 31/KVM/3010267 took a split_lock trap at address: 0x5692286f
[2251054.463864] x86/split lock detection: #AC: CPU 26/KVM/3010261 took a split_lock trap at address: 0x5692286f
[2251056.142750] x86/split lock detection: #AC: CPU 1/KVM/3010231 took a split_lock trap at address: 0x5692286f
[2251056.287511] x86/split lock detection: #AC: CPU 3/KVM/3010235 took a split_lock trap at address: 0x5692286f
[2251064.362058] x86/split lock detection: #AC: CPU 5/KVM/3010237 took a split_lock trap at address: 0x5692286f
[2251067.930588] x86/split lock detection: #AC: CPU 2/KVM/3010233 took a split_lock trap at address: 0x5692286f
[2251068.356546] x86/split lock detection: #AC: CPU 21/KVM/3010255 took a split_lock trap at address: 0x5692286f
[2251068.716481] x86/split lock detection: #AC: CPU 8/KVM/3010240 took a split_lock trap at address: 0x5692286f
[2251069.778550] x86/split lock detection: #AC: CPU 18/KVM/3010252 took a split_lock trap at address: 0x5692286f
[2251070.948536] x86/split lock detection: #AC: CPU 6/KVM/3010238 took a split_lock trap at address: 0x5692286f
[2251073.308919] x86/split lock detection: #AC: CPU 10/KVM/3010243 took a split_lock trap at address: 0x5692286f
[2251073.882550] x86/split lock detection: #AC: CPU 13/KVM/3010246 took a split_lock trap at address: 0x5692286f
[2251094.272327] x86/split lock detection: #AC: CPU 20/KVM/3010254 took a split_lock trap at address: 0x5692286f
[2251094.539520] x86/split lock detection: #AC: CPU 27/KVM/3010262 took a split_lock trap at address: 0x5692286f
[2251097.550543] x86/split lock detection: #AC: CPU 28/KVM/3010263 took a split_lock trap at address: 0x5692286f
[2251097.713567] x86/split lock detection: #AC: CPU 22/KVM/3010256 took a split_lock trap at address: 0x5692286f
[2251099.520781] x86/split lock detection: #AC: CPU 7/KVM/3010239 took a split_lock trap at address: 0x5692286f
[2251100.062472] x86/split lock detection: #AC: CPU 9/KVM/3010241 took a split_lock trap at address: 0x5692286f
[2251100.296330] x86/split lock detection: #AC: CPU 11/KVM/3010244 took a split_lock trap at address: 0x5692286f
[2251102.085852] x86/split lock detection: #AC: CPU 15/KVM/3010248 took a split_lock trap at address: 0x5692286f
[2251104.730460] x86/split lock detection: #AC: CPU 16/KVM/3010249 took a split_lock trap at address: 0x5692286f
[2251105.404260] x86/split lock detection: #AC: CPU 12/KVM/3010245 took a split_lock trap at address: 0x5692286f
[2251106.013584] x86/split lock detection: #AC: CPU 14/KVM/3010247 took a split_lock trap at address: 0x5692286f
[2251106.304128] x86/split lock detection: #AC: CPU 30/KVM/3010266 took a split_lock trap at address: 0x5692286f
[2251111.988572] x86/split lock detection: #AC: CPU 0/KVM/3010230 took a split_lock trap at address: 0x5692286f
[2251115.900906] x86/split lock detection: #AC: CPU 23/KVM/3010257 took a split_lock trap at address: 0x5692286f
[2251117.321562] x86/split lock detection: #AC: CPU 19/KVM/3010253 took a split_lock trap at address: 0x5692286f
[2251120.634105] x86/split lock detection: #AC: CPU 29/KVM/3010264 took a split_lock trap at address: 0x5692286f
```

## PCI passthrough via OVMF
好诡异的标题:

https://wiki.archlinux.org/title/PCI_passthrough_via_OVMF#Setting_up_an_OVMF-based_guest_virtual_machine

## 星布谷地
https://planet.mihoyo.com/home


## proton

## https://www.bilibili.com/video/BV1FGq5BwE5R
太好了

## steam os 安装
https://help.steampowered.com/en/faqs/view/65B4-2AA3-5F37-4227#install

bzcat steamdeck-recovery-4.img.bz2 | sudo dd if=/dev/stdin of=/dev/sdX oflag=sync status=progress bs=128M

记得必须使用 ovmf 启动，这个是一个安装好的 os

默认情况:
```txt
NAME   MAJ:MIN RM  SIZE RO TYPE MOUNTPOINTS
sda      8:0    0   10G  0 disk
sdb      8:16   0   10G  0 disk
zram0  253:0    0  3.9G  0 disk [SWAP]
vda    254:0    0  7.2G  0 disk
├─vda1 254:1    0   64M  0 part
├─vda2 254:2    0  128M  0 part
├─vda3 254:3    0    5G  0 part /
├─vda4 254:4    0  256M  0 part /var
└─vda5 254:5    0  1.6G  0 part /var/tmp
                                /var/log
                                /var/lib/systemd/coredump
                                /var/lib/steamos-log-submitter
                                /var/cache/pacman
                                /var/lib/flatpak
                                /var/lib/docker
                                /srv
                                /root
                                /opt
                                /nix
                                /home
```

开始安装系统之后:
```txt
NAME        MAJ:MIN RM  SIZE RO TYPE MOUNTPOINTS
sda           8:0    0   10G  0 disk
sdb           8:16   0   10G  0 disk
zram0       253:0    0  3.7G  0 disk [SWAP]
vda         254:0    0  7.2G  0 disk
├─vda1      254:1    0   64M  0 part /esp
├─vda2      254:2    0  128M  0 part /efi
├─vda3      254:3    0    5G  0 part /
├─vda4      254:4    0  256M  0 part /var
└─vda5      254:5    0  1.6G  0 part /var/tmp
                                     /var/log
                                     /var/lib/systemd/coredump
                                     /var/lib/steamos-log-submitter
                                     /var/lib/flatpak
                                     /var/lib/docker
                                     /var/cache/pacman
                                     /srv
                                     /root
                                     /opt
                                     /nix
                                     /home
nvme0n1     259:0    0  350G  0 disk
├─nvme0n1p1 259:1    0  256M  0 part /tmp/tmp.fuUl3K8GRB/esp
├─nvme0n1p2 259:2    0   64M  0 part /tmp/tmp.fuUl3K8GRB/efi
├─nvme0n1p3 259:3    0   64M  0 part
├─nvme0n1p4 259:4    0    5G  0 part /tmp/tmp.fuUl3K8GRB
├─nvme0n1p5 259:5    0    5G  0 part
├─nvme0n1p6 259:6    0  256M  0 part /tmp/tmp.fuUl3K8GRB/var
├─nvme0n1p7 259:7    0  256M  0 part
└─nvme0n1p8 259:8    0  100M  0 part
```

## 又一次讨论

https://news.ycombinator.com/item?id=46457770

这里提到的 https://lanparty.house/ 简直就是我梦想中的样子
终于知道为什么大人喜欢过年了，因为他们可以聚在一起打麻将。

## 啊?
树莓派配 5090 ?
https://scottjg.com/posts/2026-01-08-crappy-computer-showdown/

## lpc 2025 已经有 games 的 micro conference 了

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
