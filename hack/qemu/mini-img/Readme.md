# 使用最小的 image 从而用于调试 Qemu 和 内核

- [ ] create_net.sh 的作用是什么 ?
- [ ] netdev 上两个 model 是怎么回事 ?

- [ ] 为什么需要创建 pipe ? 本质上，是无法理解 -serial 参数的含义
  - [ ] 创建多个 -serial 参数的意义是什么

使用 pipe 的方式，可以将内核的输入，输出定向到提供的 fd 上
例如使用 raw.sh ，构建一个 `-serial pipe:gg` 那么:

将内核日志导出:
```
cat gg.out
--------- 省略内核日志 --------------
Poky (Yocto Project Reference Distro) 3.1 qemux86-64 /dev/ttyS0

qemux86-64 login: [    3.953029] urandom_read: 3 callbacks suppressed
[    3.953037] random: udevd: uninitialized urandom read (16 bytes read)
[    3.958044] random: udevd: uninitialized urandom read (16 bytes read)
```

将 root 写入，然后提供两个
```
➜  mini-img git:(master) ✗ printf "root\n" > gg.in
➜  mini-img git:(master) ✗ cat gg.out
[   32.300536] EXT4-fs error (device vda): ext4_validate_block_bitmap:390: comm kworker/u2:0: bg 2: bad block bitmap checksum
root
                                                                                                                                                                       ^
root@qemux86-64:~#
```
	

https://fadeevab.com/how-to-setup-qemu-output-to-console-and-automate-using-shell-script/

[yoctoproject 官方文档指导了如果制作镜像](https://docs.yoctoproject.org/index.html)

## Licence
Copyright belongs to https://github.com/linux-kernel-labs
