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
- [ ] 各种 perf ，bpftrace，ftrace 之类的
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

## 能否让 ci 运行在 github ci 中？
或者提交给 github，让本地的 ci 自动检测

## 能否直接访问网络

## 搭建 bios 调试环境

## 测试 cpu 和内存的热插拔

## 一些 Guest 的细节问题

- oe20.04 的网卡无法自动打开

将 /etc/sysconfig/network-scripts/enp1s0 中的 onboot 修改为 yes

## 嵌套虚拟机化的支持
- 继续使用 oe 来测试
- 使用 Guest 自动登录的为 tmux 的方法
- 自动备份机制
  - 真的需要使用逐个字节拷贝的方式吗？qemu 存在什么好的机制来辅助吗？

### 测试嵌套虚拟化的性能问题

## jenkins 的 node 是本地的虚拟机，而且本地虚拟机被 cgroup 约束

## 虚拟机中搭建这个
https://github.com/meienberger/runtipi


## 参考 goffinet 的 packer 脚本
- https://github.com/goffinet/packer-kvm
- https://github.com/goffinet/virt-scripts

```txt
qemu-system-x86  141151  141073    0 /home/martins3/core/qemu/build/qemu-system-x86_64 -vnc 127.0.0.1:26 -drive file=artifacts/qemu/centos9/packer-cen
tos9,if=virtio,cache=none,discard=unmap,format=qcow2 -drive file=/home/martins3/.cache/packer/7df0e601c1b6b1d629ebd8ddb382c34f976417d6.iso,media=cdrom
 -netdev user,id=user.0,hostfwd=tcp::4053-:22 -cpu host -boot once=d -name packer-centos9 -machine type=pc,accel=kvm -device virtio-net,netdev=user.0
-m
```

尝试一下下面的集中方法:
- https://github.com/linuxkit/linuxkit
- https://fedoraproject.org/wiki/Changes/OstreeNativeContainerStable
- https://coreos.github.io/rpm-ostree/container/
