# device mapper

## lvm2 是做啥的

## 为什么现在的内核使用这种方式安装

## device mapper 还可以提供什么功能

## dm
- [ ] device-mapper 和 lvm2 是什么关系 ?

- https://docs.kernel.org/admin-guide/device-mapper/index.html#
  - 内核文档，相关内容好多啊
- 相关的代码所在的位置: drivers/dm/md*.c

- https://en.wikipedia.org/wiki/Logical_Volume_Manager_(Linux)
- https://wiki.archlinux.org/index.php/LVM : 卷 逻辑卷 到底是啥 ?
- https://opensource.com/business/16/9/linux-users-guide-lvm

最近安装 Ubunut server 作为 root，其启动参数如下，
```sh
root=/dev/mapper/ubuntu--vg-ubuntu--lv
```
这种模式，常规的切换内核是没有办法的。
