# 网络基本配置

## /etc/sysconfig/network-scripts/ 下的各种配置分析总结

### GATEWAY
如果在内部环境中安装 openEuler，发现机器有 ip 地址，但是无法 ping 通
在机器上 ping 网关是没有问题的
最后发现是没有配置路由:
```sh
ip route add default via 192.168.16.1 dev enp125s0f0
```

真正的解决办法是，给配置文件添加上 GATEWAY
```txt
🧀  cat /etc/sysconfig/network-scripts/ifcfg-ens19f1
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
NAME=ens19f1
UUID=e56d21a5-0348-4203-bdad-6211b4f3e61f
DEVICE=ens19f1
IPADDR=192.168.11.3
NETMASK=255.255.255.0
GATEWAY=192.168.16.1 # 加上这一样
ONBOOT=yes
```

## 忽然发现系统中的


nixos 中:
```txt
🧀   systemctl list-unit-files | grep -i network
network.service                        generated       -
NetworkManager-dispatcher.service      disabled        enabled
NetworkManager-wait-online.service     disabled        disabled
NetworkManager.service                 masked          enabled
systemd-network-generator.service      disabled        disabled
network-online.target                  static          -
network-pre.target                     static          -
network.target                         static          -
```


sudo journalctl -u systemd-network-generator

不过，看来 fedora 就是推荐用 NetworkManager 的:
```txt
🧀  journalctl -u network.service
Feb 25 11:23:24 haoqian-arm-xue systemd[1]: Starting LSB: Bring up/down networking...
Feb 25 11:23:24 haoqian-arm-xue network[6215]: WARN      : [network] You are using 'network' service provided by 'network-scri>
Feb 25 11:23:24 haoqian-arm-xue network[6215]: WARN      : [network] 'network-scripts' will be removed from distribution in ne>
Feb 25 11:23:24 haoqian-arm-xue network[6215]: WARN      : [network] It is advised to switch to 'NetworkManager' instead for n>
```

## 麻了，困扰好几年
- https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/7/html/networking_guide/sec-configuring_ip_networking_with_ifcg_files
- https://askubuntu.com/questions/351300/networkmanager-in-disconnected-state
- https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/7/html/networking_guide/sec-configuring_ip_networking_with_nmcli

## nfs 和 ovs 的配置

sudo yum install -y network-scripts
sudo yum install -y nfs-utils # 给 nfs 的 mount 用


## 配置 ip 的 backtrace 流程
```txt
@[
        __inet_insert_ifa+1
        inet_rtm_newaddr+877
        rtnetlink_rcv_msg+882
        netlink_rcv_skb+89
        netlink_unicast+645
        netlink_sendmsg+525
        ____sys_sendmsg+927
        ___sys_sendmsg+153
        __sys_sendmsg+138
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 1
```

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
