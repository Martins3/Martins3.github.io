# ubuntu 使用的问题合集
不知道为什么

https://kernel.ubuntu.com 可以找到
https://kernel.ubuntu.com/git/ 但是不能找到下面这个:
https://git.launchpad.net/~ubuntu-kernel/ubuntu/+source/linux/+git/jammy

## ubuntu kernel patch 的含义
https://wiki.ubuntu.com/Kernel/Dev/StablePatchFormat

## ubuntu 如何切换内核
https://support.huaweicloud.com/intl/en-us/trouble-ecs/ecs_trouble_0327.html

git clone https://git.launchpad.net/\~ubuntu-kernel/ubuntu/+source/linux/+git/noble

## 删除密码
ssh-copy-id -p 5557  root@localhost
参考 https://askubuntu.com/questions/281074/can-i-set-my-user-account-to-have-no-password
首先 visudo 将 %wheel  ALL=(ALL)       NOPASSWD: ALL 取消掉注释
然后 passwd -d root
ssh 没有密码，passwd root 临时增加，然后增加

此外: https://askubuntu.com/questions/180402/how-to-set-a-short-password-on-ubuntu

作为对比，在 centos 上删掉密码老简单了

## crash

### 配置 kdump 机制
https://ubuntu.com/server/docs/kernel-crash-dump

```txt
sudo apt install linux-crashdump
```
安装的过程中，两个 yes 就可以了，然后重启，通过工具可以看到如何重启的。

### 使用 crash
- apt install crash
- apt-get install linux-image-$(uname -r)-dbgsym

- 安装的位置在 /usr/lib/debug/boot 中:

crash /usr/lib/debug/boot/vmlinux-5.4.0-28-generic 的时候，可以直接在 kernel 中调试:

编辑一下:
vim /boot/grub/grub.cfg

将 [/boot/vmlinuz 转换为 vmlinux](https://superuser.com/questions/298826/how-do-i-uncompress-vmlinuz-to-vmlinux) 是不行的，里面没有调试信息

###  获取 debuginfo
- 关键参考: https://askubuntu.com/questions/197016/how-to-install-a-package-that-contains-ubuntu-kernel-debug-symbols
- https://superuser.com/questions/62575/where-is-vmlinux-on-my-ubuntu-installation

官方文档:
https://wiki.ubuntu.com/Debug%20Symbol%20Packages
```sh
echo "deb http://ddebs.ubuntu.com $(lsb_release -cs) main restricted universe multiverse
deb http://ddebs.ubuntu.com $(lsb_release -cs)-updates main restricted universe multiverse
deb http://ddebs.ubuntu.com $(lsb_release -cs)-proposed main restricted universe multiverse" | \
sudo tee -a /etc/apt/sources.list.d/ddebs.list

sudo apt install ubuntu-dbgsym-keyring

sudo apt-get update
sudo apt-get install linux-image-$(uname -r)-dbgsym
```
安装这个操作是可以处理

- 完了，现在我遇到完全相同的问题了:
  - https://askubuntu.com/questions/1446930/there-is-no-kernel-dbgsym-package-for-my-ubuntu-22-04

### 获取 srouce code
```sh
echo "deb-src http://archive.ubuntu.com/ubuntu $(lsb_release -cs) main restricted universe multiverse
deb-src http://archive.ubuntu.com/ubuntu $(lsb_release -cs)-updates main restricted universe multiverse
deb-src http://security.ubuntu.com/ubuntu $(lsb_release -cs)-security main restricted universe multiverse" | \
sudo tee -a /etc/apt/sources.list

sudo apt-get update
sudo apt-get install dpkg-dev -y
sudo apt-get source linux-image-$(uname -r)
```
但是实际上，可以有更加简单的方法:
- https://git.launchpad.net/~ubuntu-kernel/ubuntu/+source/linux/+git/focal


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
