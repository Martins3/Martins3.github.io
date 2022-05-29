# busybox
https://busybox.net/

```sh
sudo mkdir -p rootfs/dev
mknod dev/tty1 c 4 1
mknod dev/tty2 c 4 2
mknod dev/tty3 c 4 3
mknod dev/tty4 c 4 4
```
## 一个小问题
不知道为什么，当运行一个 hello world 的 initrd 的 kernel 参数是这个:

```bash
arg_kernel_args="nokaslr console=ttyS0 root=/dev/ram rdinit=/hello.out"
```

而运行 initrd 的时候:

```bash
arg_kernel_args="nokaslr console=ttyS0"
```
