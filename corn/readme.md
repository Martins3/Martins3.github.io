# corn

一键 hacking 环境，收集各种 hacking 环境的脚本:
- avocado
- ansible
- grafana
- libvirt
- ovs
- perf / kvm_stat
- QEMU 调试
- [ ] 虚拟机的自动安装
  - [ ] avocado
- [ ] 虚拟机的自动初始化
  - [ ] cloudinit 的
- [ ] 虚拟机备份
- [ ] core latency
- [ ] cache latency
- [ ] numa latency
- [ ] 各种 perf ，bpftrace 之类的
- [ ] 收集各种 backtrace 和 bpftrace 的
- [ ] 错误注入


## [ ] python perf 有点问题

## perf 脚本

## dracut 生成规则是什么
安装系统和安装驱动的过程中，

了解下这里面的参数:
- https://man7.org/linux/man-pages/man7/dracut.cmdline.7.html

```txt
       The traditional root=/dev/sda1 style device specification is
       allowed, but not encouraged. The root device should better be
       identified by LABEL or UUID. If a label is used, as in
       root=LABEL=<label_of_root> the initramfs will search all
       available devices for a filesystem with the appropriate label,
       and mount that device as the root filesystem.
       root=UUID=<uuidnumber> will mount the partition with that UUID as
       the root filesystem.
```
忽然意识到，实际上，kernel 的参数修改成为这个样子会更加好。

## 其实是两者都有问题:

## 调试一下 udev
journalctl -u systemd-udevd.service

## dracut 的源码可以分析下，主要是 dracut install 中，还是非常简单的

## 使用 grubby 自动切换内核，修改 kernel cmdline，从而

## 正常安装，然后也能够编译

## 能否让 ci 运行在 github ci 中？
或者提交给 github，让本地的 ci 自动检测

## 能否直接访问网络

## 搭建 bios 调试环境
