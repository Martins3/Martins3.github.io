# mknod

## `inode_operations::mknod` 的作用是什么

## 使用 chrdev 作为例子

chrdev_open :
实现一个非常诡异的操作，虽然所有的 char 开始的时候，def_char_ops ，但是最后在首次打开的时候会在此处进行替换
完成 i_cdev 的赋值
dev_t 可以查询对应的 struct cdev 通过 kobject 机制


在上面操作进行之前，然后就是需要 dev 和 cdev 准备好，**还有对应的 path**
> 关联路径 : mknod 的含义 ?
```c
int cdev_add(struct cdev *p, dev_t dev, unsigned count)

// dir 和 rdev 关联
static int ext4_mknod(struct inode *dir, struct dentry *dentry,
		      umode_t mode, dev_t rdev)
```plain

## [ ] 为什么ebbchar是没有 手动mknod 的操作, **显然**应该含有对应的操作

## [ ] driver 和 device 的区别是什么，为什么需要进行两次注册 ? 此外mknod 和 insmod 各自实现的功能是什么 ?

## [ ] 重新检查一下 /home/maritns3/core/vn/kernel/ulk/1/ulk-13.md

## [ ]  mknode syscall 的整个 path

## [ ] Professional Linux Kernel Architecture : Device Drivers
