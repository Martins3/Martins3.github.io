# 首先执行 init
- https://ibugone.com/blog/2019/04/os-lab-1/ : 中科大老哥的 blog, 并没有办法复现, 文中提到，通过 -initrd 可以实现最佳是
  - https://ops.tips/notes/booting-linux-on-qemu/ 这里的 ref 指出 : initramfs 和 kernel 被 bootloader 加载内存中间，而 initramfs 被 mount 到 / ，并且执行其中的 init
- https://www.kernel.org/doc/html/latest/filesystems/ramfs-rootfs-initramfs.html : 内核文档，可以复现，整个内容都可以好好读一读

其实唯一的不同在于，到底执行是 hello 还是 init.sh 这个程序:
