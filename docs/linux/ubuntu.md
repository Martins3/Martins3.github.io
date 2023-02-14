# ubuntu 常见使用方法
不知道为什么

https://kernel.ubuntu.com 可以找到
https://kernel.ubuntu.com/git/ 但是不能找到下面这个:
https://git.launchpad.net/~ubuntu-kernel/ubuntu/+source/linux/+git/jammy

## ubuntu kernel patch 的含义
https://wiki.ubuntu.com/Kernel/Dev/StablePatchFormat

## 修改 ubuntu 的 dhcp
/etc/netplan$ cat 00-installer-config.yaml
```yaml
network:
  ethernets:
    ens4:
      dhcp-identifier: mac
      dhcp4: true
  version: 2
```
