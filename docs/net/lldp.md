# lldp
https://serverfault.com/questions/465472/what-is-lldpad-rhel


- lldp : 链路层，用于告知其他的网络设备自己的基本信息的
  - [wiki](https://en.wikipedia.org/wiki/Link_Layer_Discovery_Protocol)
  - [官网](https://lldpd.github.io/)

## lldpd 和 lldpad 啥关系

nixos 上
```txt
  # 使用方法 : sudo lldpcli show neighbor
  services.lldpd.enable = true;
```

x86 fedora 上:
```txt
   sudo yum install lldpd
   sudo lldpcli show neighbor
   sudo systemctl restart lldpd
```
但是可以找到 lldpad ，不知道两者的关系是什么:
https://manpages.ubuntu.com/manpages/bionic/man8/lldpad.8.html

## 测试结果
在 13900k 上测试:
```txt
🤒  sudo lldpcli show neighbor
-------------------------------------------------------------------------------
LLDP neighbors:
-------------------------------------------------------------------------------
Interface:    enp5s0, via: LLDP, RID: 1, Time: 0 day, 00:04:41
  Chassis:
    ChassisID:    mac 00:e0:4c:68:0c:0c
    SysName:      nixos
    SysDescr:     NixOS 24.05 (Uakari) Linux 6.9.7 #1-NixOS SMP PREEMPT_DYNAMIC Thu Jun 27 11:52:32 UTC 2024 x86_64
    MgmtIP:       192.168.11.3
    MgmtIface:    4
    MgmtIP:       fd7a:115c:a1e0:ab12:4843:cd96:625d:6902
    MgmtIface:    7
    Capability:   Bridge, on
    Capability:   Router, on
    Capability:   Wlan, on
    Capability:   Station, off
  Port:
    PortID:       mac a0:36:bc:ad:c2:ce
    PortDescr:    enp6s0
    TTL:          120
-------------------------------------------------------------------------------
Interface:    enp6s0, via: LLDP, RID: 1, Time: 0 day, 00:04:41
  Chassis:
    ChassisID:    mac 00:e0:4c:68:0c:0c
    SysName:      nixos
    SysDescr:     NixOS 24.05 (Uakari) Linux 6.9.7 #1-NixOS SMP PREEMPT_DYNAMIC Thu Jun 27 11:52:32 UTC 2024 x86_64
    MgmtIP:       192.168.11.3
    MgmtIface:    4
    MgmtIP:       fd7a:115c:a1e0:ab12:4843:cd96:625d:6902
    MgmtIface:    7
    Capability:   Bridge, on
    Capability:   Router, on
    Capability:   Wlan, on
    Capability:   Station, off
  Port:
    PortID:       mac 00:e0:4c:68:0c:0c
    PortDescr:    enp5s0
    TTL:          120
-------------------------------------------------------------------------------
Interface:    vif55.2, via: LLDP, RID: 2, Time: 0 day, 00:04:40
  Chassis:
    ChassisID:    mac 52:54:00:12:34:56
    SysName:      localhost
    SysDescr:     CentOS Linux 7 (Core) Linux 5.10.0.x86_64 #1 SMP Sun Jun 16 06:23:27 UTC 2024 x86_64
    MgmtIP:       10.0.55.0
    MgmtIface:    3
    MgmtIP:       fec0::5054:ff:fe12:3456
    MgmtIface:    2
    Capability:   Bridge, on
    Capability:   Router, on
    Capability:   Wlan, off
    Capability:   Station, off
  Port:
    PortID:       mac 52:54:00:00:02:37
    PortDescr:    eth1
    TTL:          120
-------------------------------------------------------------------------------
Interface:    vif55.3, via: LLDP, RID: 2, Time: 0 day, 00:04:40
  Chassis:
    ChassisID:    mac 52:54:00:12:34:56
    SysName:      localhost
    SysDescr:     CentOS Linux 7 (Core) Linux 5.10.0.x86_64 #1 SMP Sun Jun 16 06:23:27 UTC 2024 x86_64
    MgmtIP:       10.0.55.0
    MgmtIface:    3
    MgmtIP:       fec0::5054:ff:fe12:3456
    MgmtIface:    2
    Capability:   Bridge, on
    Capability:   Router, on
    Capability:   Wlan, off
    Capability:   Station, off
  Port:
    PortID:       mac 52:54:00:00:03:37
    PortDescr:    eth2
    TTL:          120
-------------------------------------------------------------------------------
```

在 m1 mac 上执行:
```txt
🧀  sudo lldpcli show neighbor

-------------------------------------------------------------------------------
LLDP neighbors:
-------------------------------------------------------------------------------
Interface:    enu1c2, via: LLDP, RID: 1, Time: 0 day, 00:00:21
  Chassis:
    ChassisID:    mac 00:e0:4c:68:0c:0c
    SysName:      nixos
    SysDescr:     NixOS 24.05 (Uakari) Linux 6.9.7 #1-NixOS SMP PREEMPT_DYNAMIC Thu Jun 27 11:52:32 UTC 2024 x86_64
    MgmtIP:       192.168.11.3
    MgmtIface:    4
    MgmtIP:       fd7a:115c:a1e0:ab12:4843:cd96:625d:6902
    MgmtIface:    7
    Capability:   Bridge, on
    Capability:   Router, on
    Capability:   Wlan, on
    Capability:   Station, off
  Port:
    PortID:       mac a0:36:bc:ad:c2:ce
    PortDescr:    enp6s0
    TTL:          120
-------------------------------------------------------------------------------
Interface:    enu1c2, via: LLDP, RID: 1, Time: 0 day, 00:00:21
  Chassis:
    ChassisID:    mac 00:e0:4c:68:0c:0c
    SysName:      nixos
    SysDescr:     NixOS 24.05 (Uakari) Linux 6.9.7 #1-NixOS SMP PREEMPT_DYNAMIC Thu Jun 27 11:52:32 UTC 2024 x86_64
    MgmtIP:       192.168.11.3
    MgmtIface:    4
    MgmtIP:       fd7a:115c:a1e0:ab12:4843:cd96:625d:6902
    MgmtIface:    7
    Capability:   Bridge, on
    Capability:   Router, on
    Capability:   Wlan, on
    Capability:   Station, off
  Port:
    PortID:       mac 00:e0:4c:68:0c:0c
    PortDescr:    enp5s0
    TTL:          120
```


13900k 中的虚拟机:
```txt
martins3@bogon:~$    sudo lldpcli show neighbor
-------------------------------------------------------------------------------
LLDP neighbors:
-------------------------------------------------------------------------------
Interface:    ens5, via: LLDP, RID: 1, Time: 0 day, 00:04:30
  Chassis:
    ChassisID:    mac 00:e0:4c:68:0c:0c
    SysName:      nixos
    SysDescr:     NixOS 24.05 (Uakari) Linux 6.9.7 #1-NixOS SMP PREEMPT_DYNAMIC Thu Jun 27 11:52:32 UTC 2024 x86_64
    MgmtIP:       10.11.0.1
    MgmtIface:    8
    MgmtIP:       fd7a:115c:a1e0:ab12:4843:cd96:625d:6902
    MgmtIface:    7
    Capability:   Bridge, on
    Capability:   Router, on
    Capability:   Wlan, on
    Capability:   Station, off
  Port:
    PortID:       mac d2:2d:d0:90:43:33
    PortDescr:    vif67.2
    TTL:          120
-------------------------------------------------------------------------------
Interface:    ens6, via: LLDP, RID: 1, Time: 0 day, 00:04:30
  Chassis:
    ChassisID:    mac 00:e0:4c:68:0c:0c
    SysName:      nixos
    SysDescr:     NixOS 24.05 (Uakari) Linux 6.9.7 #1-NixOS SMP PREEMPT_DYNAMIC Thu Jun 27 11:52:32 UTC 2024 x86_64
    MgmtIP:       10.11.0.1
    MgmtIface:    8
    MgmtIP:       fd7a:115c:a1e0:ab12:4843:cd96:625d:6902
    MgmtIface:    7
    Capability:   Bridge, on
    Capability:   Router, on
    Capability:   Wlan, on
    Capability:   Station, off
  Port:
    PortID:       mac 26:bb:f9:8d:fb:60
    PortDescr:    vif67.3
    TTL:          120
-------------------------------------------------------------------------------
```
注意，这里的 ens5 正好对应的是 host 中的 vif67.3

所以，lldp show neighbor 的工作看似就是那些线连到一起了。

## 问题

在 kernel 中找 lldp 的信息，可以发现很多在具体驱动中的 lldp 内容，但是
mlx 和 virtio_net 中完全没有。

这个是可以同时实现在内核态和用户态的都可以吗?

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
