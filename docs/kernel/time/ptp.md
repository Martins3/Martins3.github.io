# 网卡模块中

https://docs.redhat.com/en/documentation/red_hat_enterprise_linux/7/html/system_administrators_guide/ch-configuring_ptp_using_ptp4l#sec-Checking_for_Driver_and_Hardware_Support

```txt
[root@arm1960 12:15:43 crash]$ ethtool -T  enp125s0f0
Time stamping parameters for enp125s0f0:
Capabilities:
        software-receive      (SOF_TIMESTAMPING_RX_SOFTWARE)
        software-system-clock (SOF_TIMESTAMPING_SOFTWARE)
PTP Hardware Clock: none
Hardware Transmit Timestamp Modes: none
Hardware Receive Filter Modes: none
```
似乎这个也是没有的哇。

13900k 的网卡
```txt
🤒  sudo ethtool -T enp5s0
[sudo] password for martins3:
Time stamping parameters for enp5s0:
Capabilities:
        software-transmit
        software-receive
        software-system-clock
PTP Hardware Clock: none
Hardware Transmit Timestamp Modes: none
Hardware Receive Filter Modes: none
```

n100 的主机上:
```txt
🧀  ethtool -T enp1s0
Time stamping parameters for enp1s0:
Capabilities:
        hardware-transmit
        software-transmit
        hardware-receive
        software-receive
        software-system-clock
        hardware-raw-clock
PTP Hardware Clock: 0
Hardware Transmit Timestamp Modes:
        off
        on
Hardware Receive Filter Modes:
        none
        all
```

## kvm 也是有 ptp 的
- https://patchwork.kernel.org/project/kvm/patch/20170113120319.777765254@redhat.com/

https://stackoverflow.com/questions/51244769/how-can-i-get-a-dev-ptp0-ptp-hardware-clock-on-a-vm-running-linux


## kvm ptp

看来是用于校准时间的，但是为什么不去直接 ntp 就可以了
```txt
modprobe kvm_ptp
Update the reference clock in the chrony config file (/etc/chrony.conf):

refclock PHC /dev/ptp0 poll 3 dpoll -2 offset 0
systemctl status chronyd.service
systemctl restart chronyd.servic
```

## 看内核的这个选项做啥的 : PTP_1588_CLOCK_OPTIONAL

## 看看这个文件做什么的
 /dev/ptp0

```txt
🧀  ls /dev/ptp0
 /dev/ptp0
~ 🐶
🧀  ls /dev/ptp_kvm
 /dev/ptp_kvm
```

那么物理机中的 /dev/ptp0 是做什么的?

## 这个目录这么多内容啊
drivers/ptp/

### 不去考虑 ptp ，该如何?
https://docs.kernel.org/driver-api/ptp.html

### 如果虚拟机配置了 ptp ，也配置了 ntp ，听哪一个的?

## arm 的 ptp 功能
- Documentation/virt/kvm/arm/ptp_kvm.rst

## sysfs
### mlnx5 是机器
0000:82:02.1 是 mlnx5 的一个 vf ，这里仅仅列举了其中的一个内容:
一共相关的:
```txt
/sys/class/ptp
/sys/class/ptp/ptp9
/sys/devices/pci0000:80/0000:80:04.0/0000:82:02.1/ptp
/sys/devices/pci0000:80/0000:80:04.0/0000:82:02.1/ptp/ptp9
```

在 /sys/class/ptp/ptp9

```txt
 clock_name
 dev
 device -> ../../../0000:82:02.1
 max_adjustment
 n_alarms
 n_external_timestamps
 n_periodic_outputs
 n_programmable_pins
 power
 pps_available
 subsystem -> ../../../../../../class/ptp
 uevent
```

### 虚拟机

```txt
/sys/class/ptp
/sys/class/ptp/ptp0
/sys/devices/virtual/ptp
/sys/devices/virtual/ptp/ptp0
```

```txt
🤒  cat /sys/devices/virtual/ptp/ptp0/clock_name
KVM virtual PTP
```
### intel 13900k
```txt
🧀  find /sys -name "*ptp*"
/sys/class/ptp
/sys/class/ptp/ptp0
/sys/class/ptp/ptp1
/sys/devices/pci0000:00/0000:00:14.3/ptp
/sys/devices/pci0000:00/0000:00:14.3/ptp/ptp1
/sys/devices/pci0000:00/0000:00:1c.2/0000:06:00.0/ptp
/sys/devices/pci0000:00/0000:00:1c.2/0000:06:00.0/ptp/ptp0
/sys/module/pps_core/holders/ptp
/sys/module/ptp

🤒  lspci -s 0000:00:14.3
00:14.3 Network controller: Intel Corporation Alder Lake-S PCH CNVi WiFi (rev 11)
~ 🔥
🧀  lspci -s 0000:06:00.0
06:00.0 Ethernet controller: Intel Corporation Ethernet Controller I225-V (rev 03)
```
不过，这个机器上是有两个网卡的

## 看看 arm 的 ptp_kvm 实现

这是一个 pick :
- https://github.com/kata-containers/kata-containers/blob/main/tools/packaging/kernel/patches/5.10.x/0001-5.10.x-PTP_KVM-support-for-arm-arm64.patch

原来的 patch :
- https://patches.linaro.org/project/netdev/cover/20201209060932.212364-1-jianyong.wu@arm.com/

oe 的合并，这个代码最少，容易看
```txt
History:        #0
Commit:         f8cb5f82f66cb9d78c0c7f9ad8eda9ae1a4e7cba
Author:         Jianyong Wu <jianyong.wu@arm.com>
Committer:      Dongxu Sun <sundongxu3@huawei.com>
Author Date:    Wed 29 Nov 2023 04:02:11 PM CST
Committer Date: Thu 30 Nov 2023 12:26:56 PM CST

KVM: arm64: Add support for the KVM PTP service

mainline inclusion
from mainline-v5.13-rc1
commit 3bf725699bf62494b3e179f1795f08c7d749f061
category: feature
bugzilla: https://gitee.com/openeuler/kernel/issues/I8DWT1
CVE: NA

Reference: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=3bf725699bf62494b3e179f1795f08c7d749f061

--------------------------------

Implement the hypervisor side of the KVM PTP interface.

The service offers wall time and cycle count from host to guest.
The caller must specify whether they want the host's view of
either the virtual or physical counter.

Signed-off-by: Jianyong Wu <jianyong.wu@arm.com>
Signed-off-by: Marc Zyngier <maz@kernel.org>
Link: https://lore.kernel.org/r/20201209060932.212364-7-jianyong.wu@arm.com
Signed-off-by: Kunkun Jiang <jiangkunkun@huawei.com>
Signed-off-by: Dongxu Sun <sundongxu3@huawei.com>
```

## 问题
暂时感觉就这么些内容了，不过有一些问题之后可以看看:

- [ ] /dev/ptp 如何映射到具体的设备上去，而且虚拟机中也是有一个 /dev/ptp 的

## 到时候可以挑 drivers/ptp/ 下的驱动都看看

## 如何理解 ptp 的时间调整

```c
static struct posix_clock_operations ptp_clock_ops = {
	.owner		= THIS_MODULE,
	.clock_adjtime	= ptp_clock_adjtime,
	.clock_gettime	= ptp_clock_gettime,
	.clock_getres	= ptp_clock_getres,
	.clock_settime	= ptp_clock_settime,
	.ioctl		= ptp_ioctl,
	.open		= ptp_open,
	.release	= ptp_release,
	.poll		= ptp_poll,
	.read		= ptp_read,
};
```

## 原来 /dev/ptp0 的 api 的定义在 kernel/time/posix-clock.c

```c
static const struct file_operations posix_clock_file_operations = {
	.owner		= THIS_MODULE,
	.read		= posix_clock_read,
	.poll		= posix_clock_poll,
	.unlocked_ioctl	= posix_clock_ioctl,
	.open		= posix_clock_open,
	.release	= posix_clock_release,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= posix_clock_compat_ioctl,
#endif
};
```
所以，为什么这个功能是叫做 posix-clock.c 的

## kvm ptp 是这两个选项吗?
```txt
CONFIG_PTP_1588_CLOCK=y
CONFIG_PTP_1588_CLOCK_KVM=y
```

## PTP 环境搭建


可以看看，
https://github.com/Lularible/ptp-book

不过，这个人，为什么有这么多的不同领域的东西

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
