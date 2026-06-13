- [ ] vlan/[802.1q](https://en.wikipedia.org/wiki/IEEE_802.1Q)

## tracking
https://www.educative.io/answers/what-is-trunking-in-networking

> LA 链路聚合，也称为trunking

## vlan
参考 https://www.practicalnetworking.net/stand-alone/vlans/

A simple representation of a single entry in a MAC address table would be: MAC Address | Port.

A simple representation of a single entry in a MAC address table of a VLAN aware switch would be: VLAN# | MAC Address | Port.

The primary benefit of extending a VLAN to different physical switches is that the Layer 2 topology no longer has to be tied to the Physical Topology.

## 从 sfboot 看，vlans 甚至是可以做到硬件中去的
```txt
  pf-vlans=<tag>[,<tag>[,...]]|None
    Specifies a VLAN tag for each PF on the port in a comma-separated
    list. Each VLAN tag must be unique for the port and be in the
    range [0..4094]. Specifying pf-vlans=None will clear all VLAN tags
    on the port. Pf-vlans should be included after pf-count on the
    sfboot command line.
    If the number of PFs is changed then the VLAN tags will be cleared.
```

https://wiki.ubuntu.com/vlan

https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/7/html/networking_guide/sec-configure_802_1q_vlan_tagging_using_the_command_line

https://www.redhat.com/sysadmin/vlans-configuration

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
