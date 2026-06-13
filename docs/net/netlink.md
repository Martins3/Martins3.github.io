# netlink
## 有趣的 netlink
```txt
🧀  t __dev_close_many
[sudo] password for martins3:
__dev_close_many
sudo bpftrace -e "kprobe:__dev_close_many { @[kstack] = count(); }"
Attaching 1 probe...
^C

@[
    __dev_close_many+1
    __dev_change_flags+422
    dev_change_flags+38
    do_setlink+927
    __rtnl_newlink+1617
    rtnl_newlink+71
    rtnetlink_rcv_msg+335
    netlink_rcv_skb+88
    netlink_unicast+419
    netlink_sendmsg+596
    ____sys_sendmsg+892
    ___sys_sendmsg+154
    __sys_sendmsg+122
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+110
]: 1
```
触发的操作:
sudo ip link set dev enp5s0 down

## netlink
似乎并不是很难实现，如果有时间，可以先看看如何实现吧:
- [Instructions for running netlink example code in c](https://gist.github.com/arunk-s/c897bb9d75a6c98733d6)
- [stackoverflow : How to use netlink socket to communicate with a kernel module?](https://stackoverflow.com/questions/3299386/how-to-use-netlink-socket-to-communicate-with-a-kernel-module)

## netlink 号称替代 ioctl ，但是真的有人用吗?
https://docs.kernel.org/userspace-api/netlink/intro.html

## rtnl_link_ops 是做什么的 ?


https://natanyellin.com/posts/buggy-netlink-process-connectors/
netlink process 什么鬼

socket(PF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR)

如何理解 PF_NETLINK ?

## 看看
https://zhuanlan.zhihu.com/p/386405400

## 这个做什么的?
```c
static const struct rtnl_msg_handler devinet_rtnl_msg_handlers[] __initconst = {
	{.protocol = PF_INET, .msgtype = RTM_NEWADDR, .doit = inet_rtm_newaddr,
	 .flags = RTNL_FLAG_DOIT_PERNET},
	{.protocol = PF_INET, .msgtype = RTM_DELADDR, .doit = inet_rtm_deladdr,
	 .flags = RTNL_FLAG_DOIT_PERNET},
	{.protocol = PF_INET, .msgtype = RTM_GETADDR, .dumpit = inet_dump_ifaddr,
	 .flags = RTNL_FLAG_DUMP_UNLOCKED | RTNL_FLAG_DUMP_SPLIT_NLM_DONE},
	{.protocol = PF_INET, .msgtype = RTM_GETNETCONF,
	 .doit = inet_netconf_get_devconf, .dumpit = inet_netconf_dump_devconf,
	 .flags = RTNL_FLAG_DOIT_UNLOCKED | RTNL_FLAG_DUMP_UNLOCKED},
};
```

## gen netlink
参考

https://www.yaroslavps.com/weblog/genl-intro/

https://lwn.net/Articles/208755/

https://wiki.linuxfoundation.org/networking/generic_netlink_howto

## 这两个案例在确认一下
kernel/taskstats.c 实现一个 genl_ops 看看:

```c
static const struct genl_ops taskstats_ops[] = {
	{
		.cmd		= TASKSTATS_CMD_GET,
		.validate = GENL_DONT_VALIDATE_STRICT | GENL_DONT_VALIDATE_DUMP,
		.doit		= taskstats_user_cmd,
		.policy		= taskstats_cmd_get_policy,
		.maxattr	= ARRAY_SIZE(taskstats_cmd_get_policy) - 1,
		.flags		= GENL_ADMIN_PERM,
	},
	{
		.cmd		= CGROUPSTATS_CMD_GET,
		.validate = GENL_DONT_VALIDATE_STRICT | GENL_DONT_VALIDATE_DUMP,
		.doit		= cgroupstats_user_cmd,
		.policy		= cgroupstats_cmd_get_policy,
		.maxattr	= ARRAY_SIZE(cgroupstats_cmd_get_policy) - 1,
	},
};
```

```c
static struct genl_family thermal_genl_family __ro_after_init = {
	.hdrsize	= 0,
	.name		= THERMAL_GENL_FAMILY_NAME,
	.version	= THERMAL_GENL_VERSION,
	.maxattr	= THERMAL_GENL_ATTR_MAX,
	.policy		= thermal_genl_policy,
	.bind		= thermal_genl_bind,
	.unbind		= thermal_genl_unbind,
	.small_ops	= thermal_genl_ops,
	.n_small_ops	= ARRAY_SIZE(thermal_genl_ops),
	.resv_start_op	= __THERMAL_GENL_CMD_MAX,
	.mcgrps		= thermal_genl_mcgrps,
	.n_mcgrps	= ARRAY_SIZE(thermal_genl_mcgrps),
};
```

## 如何理解 NETFILTER_NETLINK 和 NF_CT_NETLINK

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
