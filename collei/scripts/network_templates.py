from __future__ import annotations

import uuid


def temporary_ip_commands(guest: int, level: int) -> str:
    mac0 = f"52:54:00:{level:02x}:{guest:02x}:00"
    mac1 = f"52:54:00:{level:02x}:{guest:02x}:01"
    return f"""nmcli c show
sudo systemctl start NetworkManager

# 给 vfio 配置 ip 地址
sudo nmcli connection add type ethernet ifname ens1f0np0 ipv4.method manual ipv4.addresses 172.22.129.26/17

sudo nmcli connection add type ethernet con-name vhost ifname "*" mac {mac0} ip4 10.0.{guest}.{level}/16
sudo nmcli connection add type ethernet con-name user ifname "*" mac 52:54:00:12:34:56 ip4 10.0.2.2/16
sudo nmcli c up vhost
sudo nmcli c up user

# 似乎直接配置 nmcli 就是最好的
# sudo nmcli connection add type ethernet con-name user ifname "*" mac 52:54:00:12:34:56 ipv4.method auto
# 可能还需要配置上
# ip route add default via 10.0.2.2 dev ens5

sudo nmcli connection add type ethernet con-name tap ifname "*" mac {mac1} ip4 172.213.0.2/24
sudo nmcli connection modify X ipv4.gateway 10.0.0.2
ip addr add dev ens4 10.0.{guest}.{level}/16

sudo nmcli connection add type bridge ifname br9527
sudo nmcli connection add type bridge-slave ifname enp125s0f0 master br9527
sudo nmcli connection modify br9527 ipv4.addresses 10.0.{guest}.{level}/16 ipv4.gateway 10.0.0.2 ipv4.method manual
sudo nmcli connection modify br9527 ipv4.addresses 192.168.19.60/20 ipv4.gateway 192.168.16.3 ipv4.method manual

# 这个不可以工作
sudo ovs-vsctl add-br br-in
sudo nmcli connection add type ovs-bridge conn.interface br-in con-name br-in
sudo nmcli connection add type ovs-port conn.interface br-in-port master br-in con-name br-in-port
sudo nmcli connection add type ovs-interface slave-type ovs-port conn.interface br-in master br-in-port con-name br-in-intf ipv4.method manual ipv4.addresses 10.0.{guest}.0/16 ipv4.gateway "" ethernet.cloned-mac-address 6A:D6:5E:4E:00:44
sudo nmcli connection up br-in-intf
# 这一步遇到问题:
# Error: Connection activation failed: Open vSwitch database connection failed
#
# 不知道为什么需要这一步，但是这一步的确是需要的
sudo ip route add default via 192.168.16.3 dev br-in
"""


def network_configurations(guest: int, level: int) -> str:
    return f"""== ifcfg-slirp ==
NAME=qemu
TYPE=Ethernet
PROXY_METHOD=none
BROWSER_ONLY=no
BOOTPROTO=dhcp
DEFROUTE=yes
IPV4_FAILURE_FATAL=no
IPV6INIT=yes
IPV6_AUTOCONF=yes
IPV6_DEFROUTE=yes
IPV6_FAILURE_FATAL=no
IPV6_ADDR_GEN_MODE=eui64
UUID={uuid.uuid4()}
DEVICE=qemu-slirp
ONBOOT=yes
HWADDR=52:54:00:12:34:56

# 不用 ovs 的时候
== ifcfg-vhost ==
TYPE=Ethernet
PROXY_METHOD=none
BROWSER_ONLY=no
BOOTPROTO=static
DEFROUTE=yes
IPV4_FAILURE_FATAL=no
IPV6INIT=yes
IPV6_AUTOCONF=yes
IPV6_DEFROUTE=yes
IPV6_FAILURE_FATAL=no
IPV6_ADDR_GEN_MODE=eui64
NAME=qemu
UUID={uuid.uuid4()}
DEVICE=vhost
IPADDR=10.0.{guest}.{level}
NETMASK=255.255.0.0
ONBOOT=yes
HWADDR=52:54:00:00:02:{guest:02x}

# 配置 ovs 的时候，无需配置网卡，只用配置 ovs 就可以了
# 但是如果内核配置变化(打开 CONFIG_HOTPLUG_PCI)，网卡名称变化了，ovs 关联的网络就断了
== ifcfg-br-in ==
DEVICE=br-in
BOOTPROTO=static
ONBOOT=yes
DEVICETYPE=ovs
TYPE=OVSIntPort
IPADDR=10.0.{guest}.0
NETMASK=255.255.0.0
OVS_BRIDGE=br-in
HOTPLUG=no
MACADDR=6A:D6:5E:4E:00:44

# 配置 bridge 的配置
== ifcfg-br9527 ==
DEVICE=br9527
TYPE=Bridge
ONBOOT=yes
BOOTPROTO=static
NM_CONTROLLED=no
DELAY=0
IPADDR=10.0.{guest}.{level}
NETMASK=255.255.0.0
# GATEWAY=192.168.16.1
ONBOOT=yes
# 不用带 HWADDR ，br9527 没有 mac

# 添加 bridge 关联的网卡
== ifcfg-br-eth ==
NAME=ifcfg-br-eth
TYPE=Ethernet
PROXY_METHOD=none
BROWSER_ONLY=no
BOOTPROTO=static
DEFROUTE=yes
IPV4_FAILURE_FATAL=no
IPV6INIT=yes
IPV6_AUTOCONF=yes
IPV6_DEFROUTE=yes
IPV6_FAILURE_FATAL=no
IPV6_ADDR_GEN_MODE=eui64
NM_CONTROLLED=no
UUID={uuid.uuid4()}
# 控制 device 的名称是可以不用的 ?
# DEVICE=br-eth
BRIDGE=br9527
ONBOOT=yes
HWADDR=52:54:00:00:02:{guest:02x}

# 物理机参考
TYPE=Ethernet
PROXY_METHOD=none
BROWSER_ONLY=no
BOOTPROTO=static # hdcp -> static
DEFROUTE=yes
IPV4_FAILURE_FATAL=no
IPV6INIT=yes
IPV6_AUTOCONF=yes
IPV6_DEFROUTE=yes
IPV6_FAILURE_FATAL=no
IPV6_ADDR_GEN_MODE=eui64
NAME=enp125s0f0
UUID=8816c4ff-1ecb-47a9-81c1-d1628f21082d
DEVICE=enp125s0f0
ONBOOT=yes
# 添加如下内容
IPADDR=192.168.19.60
NETMASK=255.255.240.0
HWADDR=F0:33:E5:D1:A4:FD
GATEWAY=192.168.16.3
"""
