# [wiki](https://en.wikipedia.org/wiki/IEEE_802.11)

IEEE 802.11 is part of the IEEE 802 set of local area network (LAN) protocols, and specifies the set of media access control (MAC) and physical layer (PHY) protocols for implementing wireless local area network (WLAN) Wi-Fi computer communication in various frequencies including, but not limited to, 2.4 GHz, 5 GHz, 6 GHz, and 60 GHz frequency bands.


# [stackoverflow](https://stackoverflow.com/questions/7157181/how-to-learn-the-structure-of-linux-wireless-drivers-mac80211)

# code
和网卡交互的标准操作:

- [ ] 但是，为什么 80211 不是在 dervier 下面的

```c
static const struct net_device_ops ieee80211_monitorif_ops = {
	.ndo_open		= ieee80211_open,
	.ndo_stop		= ieee80211_stop,
	.ndo_uninit		= ieee80211_uninit,
	.ndo_start_xmit		= ieee80211_monitor_start_xmit,
	.ndo_set_rx_mode	= ieee80211_set_multicast_list,
	.ndo_set_mac_address 	= ieee80211_change_mac,
	.ndo_select_queue	= ieee80211_monitor_select_queue,
	.ndo_get_stats64	= ieee80211_get_stats64,
};

static const struct net_device_ops e1000_netdev_ops = {
	.ndo_open		= e1000_open,
	.ndo_stop		= e1000_close,
	.ndo_start_xmit		= e1000_xmit_frame,
	.ndo_set_rx_mode	= e1000_set_rx_mode,
	.ndo_set_mac_address	= e1000_set_mac,
	.ndo_tx_timeout		= e1000_tx_timeout,
	.ndo_change_mtu		= e1000_change_mtu,
	.ndo_do_ioctl		= e1000_ioctl,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_vlan_rx_add_vid	= e1000_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid	= e1000_vlan_rx_kill_vid,
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller	= e1000_netpoll,
#endif
	.ndo_fix_features	= e1000_fix_features,
	.ndo_set_features	= e1000_set_features,
};
```

```
➜  linux git:(master) sudo lshw -C network
[sudo] password for maritns3:
  *-network
       description: Wireless interface
       product: Wireless 8265 / 8275
       vendor: Intel Corporation
       physical id: 0
       bus info: pci@0000:02:00.0
       logical name: wlp2s0
       version: 78
       serial: 40:a3:cc:ff:40:83
       width: 64 bits
       clock: 33MHz
       capabilities: pm msi pciexpress bus_master cap_list ethernet physical wireless
       configuration: broadcast=yes driver=iwlwifi driverversion=5.8.0-41-generic firmware=36.77d01142.0 8265-36.ucode ip=192.168.43.70 latency=0 link=yes multicast=yes
 wireless=IEEE 802.11
       resources: irq:136 memory:b4100000-b4101fff
  *-network:0 DISABLED
       description: Ethernet interface
       physical id: 3
       logical name: virbr0-nic
       serial: 52:54:00:e5:9e:7a
       size: 10Mbit/s
       capabilities: ethernet physical
       configuration: autonegotiation=off broadcast=yes driver=tun driverversion=1.6 duplex=full link=no multicast=yes port=twisted pair speed=10Mbit/s
  *-network:1 DISABLED
       description: Ethernet interface
       physical id: 4
       logical name: mpqemubr0-dummy
       serial: 52:54:00:2d:ab:7c
       capabilities: ethernet physical
       configuration: broadcast=yes driver=dummy driverversion=5.8.0-41-generic
```
由此可以原来使用的 driver 是 : iwlwifi
