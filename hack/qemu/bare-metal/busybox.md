# busybox
https://busybox.net/


似乎是一个比较靠谱的文章:
https://www.cnblogs.com/hellogc/p/7482066.html
busybox 是用于提供 init 程序

这个方法 : 告知 drive + root 指定，使用 linuxrc 来启动，老的做法
> 这个方法成功了，那么 Ubuntu20 的还是没有办法，是不是因为 Ubuntu20 由于 ext4，所以，存在问题
> **可以尝试的内容 : 不使用 busybox，在其中只是简单的写入一个 init 程序 : 估计是不可以的**

> 1. 这个blog中间最后的部分，mount 各种文件夹的操作暂时没有实现
> 2. 关于创建 init 的部分，感觉不科学，maybe out of dated，内核的文档中间才说到 linurc 作为启动的不科学

cnblog 的那个，其中 tty 的报错，也许可以使用如下解决

```sh
sudo mkdir -p rootfs/dev 
sudo mknod rootfs/dev/tty1 c 4 1  
sudo mknod rootfs/dev/tty2 c 4 2  
sudo mknod rootfs/dev/tty3 c 4 3  
sudo mknod rootfs/dev/tty4 c 4 4
```
