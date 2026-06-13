## bootc
- https://docs.fedoraproject.org/en-US/bootc/
- https://osbuild.org/
- https://github.com/osbuild/bootc-image-builder

按照这个操作:
https://github.com/osbuild/bootc-image-builder?tab=readme-ov-file#-build-config

是可以走通的
```txt
🧀  ssh -p4021 alice@localhost # 密码 bob
alice@localhost's password:
Last login: Sat Dec  7 05:53:44 2024 from 10.0.2.2
[systemd]
Failed Units: 1
  NetworkManager-wait-online.service
[alice@bogon ~]$ tree
-bash: tree: command not found
[alice@bogon ~]$ lsblk
NAME   MAJ:MIN RM  SIZE RO TYPE MOUNTPOINTS
loop0    7:0    0  6.1M  1 loop
sda      8:0    0   10G  0 disk
├─sda1   8:1    0    1M  0 part
├─sda2   8:2    0  501M  0 part /boot/efi
├─sda3   8:3    0    1G  0 part /boot
└─sda4   8:4    0  8.5G  0 part /var
                                /sysroot/ostree/deploy/default/var
                                /etc
                                /sysroot
zram0  252:0    0  1.8G  0 disk [SWAP]
[alice@bogon ~]$ cat /proc/cmdline
BOOT_IMAGE=(hd0,gpt3)/boot/ostree/default-1d93f15b5e143444a02569d5b61afc66ab95546ca2678a604a847956ae360bdf/vmlinuz-5.14.0-536.el9.x86_64 root=UUID=41a7cd3c-cef5-4c6a-88eb-793dc39e2014 rw boot=UUID=016a24a2-2dbb-4974-a88f-2bfd56cd9a7d rw console=tty0 console=ttyS0 ostree=/ostree/boot.1/default/1d93f15b5e143444a02569d5b61afc66ab95546ca2678a604a847956ae360bdf/0
```

## busybox
1. https://landley.net/toybox/quick.html

- [ ] https://github.com/marcan/takeover.sh

- https://www.cnblogs.com/wipan/p/9272255.html
- https://gist.github.com/chrisdone/02e165a0004be33734ac2334f215380e
- https://www.digi.com/resources/documentation/digidocs/90001515/task/yocto/t_configure_network.htm


## dracut 常用命令
```txt
dracut --kver $(uname -r)  --force
```


## initramfs 中应该带什么东西

### 1. 配置文件
例如 介绍在 /etc/modprobe.d/ 中配置 nvme 的 channel ，但是如果 /etc/modprobe.d/ 本身就是存储在 nvme 上，怎么办？
https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/8/html/managing_storage_devices/enabling-multipathing-on-nvme-devices_managing-storage-devices

lsinitrd 可以看到:
```txt
drwxr-xr-x   2 root     root            0 May 23  2024 etc/modprobe.d
-rw-r--r--   1 root     root          166 May 20  2024 etc/modprobe.d/firewalld-sysctls.conf
-rw-r--r--   1 root     root          674 Feb 18  2023 etc/modprobe.d/tuned.conf
```

## 现在可以我知道了
1. boot 之后，我怎么知道存在 hard disk 的存在
3. fs 怎么知道自己 mount 到哪一个 partion 上面去

## 类似的 busybox 的写法
- https://github.com/arighi/virtme-ng/blob/main/virtme/mkinitramfs.py

## 给 ubuntu 或者 debian 打包，也许可以找出什么有用
https://github.com/armbian/build


## 这里提到的 : mkinitramfs -o ramdisk.img 是什么东西
http://nickdesaulniers.github.io/blog/2018/10/24/booting-a-custom-linux-kernel-in-qemu-and-debugging-it-with-gdb/

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
