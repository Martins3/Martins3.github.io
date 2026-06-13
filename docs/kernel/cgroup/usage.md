# cgroup 的操作手册

需要注意到，如果切换到 /sys/fs/cgroup/user.slice/user-1000.slice/user@1000.service
就不存在权限问题了。

## 所有的全部都集中到
code/src/m/scripts/cgroup.sh 中去吧

## cpuset
https://www.redhat.com/en/blog/world-domination-cgroups-part-6-cpuset

```sh
sudo cgcreate -g cpuset:testset
sudo cgset -r cpuset.cpus=3 testset
sudo cgset -r cpuset.mems=1 testset
sudo cgexec -g cpuset:testset stress-ng --vm-bytes 6500M --vm-keep --vm 3
```
这个需要配合 qemu numa 工具来解决

当前的状态可以通过
cat /proc/self/status 中来检查，例如在一个 numa node 为 16 的环境中，
```txt
Cpus_allowed:   3fffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff
Cpus_allowed_list:      0-253
Mems_allowed:   00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,0000ffff
Mems_allowed_list:      0-15
```
似乎 Mems_allowed 展示了系统中一共支持多少个内存。

问题是，我系统中并没有那么多 numa node 啊！

## io
```sh
sudo cgcreate -g io:A
sudo cgexec -g io:A fio test.fio
```

```txt
➜  ~ cat /sys/fs/cgroup/A/io.stat
259:0 rbytes=859275264 wbytes=0 rios=209784 wios=0 dbytes=0 dios=0
253:0 rbytes=5566464 wbytes=0 rios=13 wios=0 dbytes=0 dios=0
```


## memcontrol
1. 在第一个终端中运行运行

```sh
cgcreate -g memory:mem
watch --interval 1 cgget -g memory:mem # 此时，其中大多数都是 0
```

2. 在另一个 shell 中运行
```sh
cgexec -g memory:mem stress-ng --vm-bytes 150M --vm-keep --vm 1
```

3. 两边都关闭，然后设置 limit
```sh
cgset -r memory.max=100m mem
```

4. 重新运行 stress 之后，可以得到 oom

## io
```sh
cgcreate -g io:duck
cd /sys/fs/cgroup/duck
echo "8:16  wiops=1000" > io.max
cgexec -g io:duck dd if=/dev/zero of=/dev/sdb bs=1M count=1000
```

## cpu
- [基本含义](https://facebookmicrosites.github.io/cgroup2/docs/cpu-controller.html)

- cpu.idle
- cpu.max && cpu.max.burst : bandwidth 限制
- cpu.pressure && cpu.stat
- cpu.weight && cpu.weight.nice :

## cpu

- cpu.uclamp.max && cpu.uclamp.min

```sh
cgcreate -g cpu:C

# -c 0 是所有的 CPU，其他数值表示 worker 的数量
# -l 使用 CPU 的多少
# 自动运行所有的负载
cgexec -g cpu:C stress-ng -c 0 -l 100 &

# 设置之前，会占满一个 CPU，之后只会占用一个 CPU 的 10%
cgset -r cpu.max="1000000" C

cgset -r cpu.weight.nice=19 C

cgdelete -g cpu:C
```

## 其他的 libcgroup
1. cgcreate -g memory,hugeltb:duck # 多个限制
2. cgcreate -g cpu:a/b/c 创建嵌套的


## cpuset
taskset 和 numactl

```sh
cgcreate -g cpuset:C
cgset -r cpuset.cpus="2-4" C
cgset -r cpuset.cpus.partition="root" C

cgexec -g cpu:C stress-ng -c 0 -l 100 &
```

将 partition 设置为 root 之后，此外 `cpuset.cpus` 不见了:
```txt
➜  ~ cat /sys/fs/cgroup/cpuset.cpus.effective
0-7
➜  ~ cgset -r cpuset.cpus.partition="root" C
➜  ~ cat /sys/fs/cgroup/cpuset.cpus.effective
0-1,5-7
```

## 检查当前配置
lscgroup


才知道有这个小技巧，通过 CGROUP_LOGLEVEL 来显示更多的信息:
https://unix.stackexchange.com/questions/725112/using-cgroups-v2-without-root
```txt
🤒  CGROUP_LOGLEVEL=INFO cgexec -g memory:user.slice/user-1000.slice/user@1000.service/user.slice/qemu ls
Warning: cannot write tid 495573 to /sys/fs/cgroup/user.slice/user-1000.slice/user@1000.service/user.slice/qemu/cgroup.procs:Permission denied
Warning: cgroup_attach_task_pid failed: 50007
cgroup change of group failed
```
cgexec 的这个技术，提前知道进程 pid，到时候用于 ftrace 来跟踪。

## 系统配置项目
- https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/6/html/resource_management_guide/sec-cgsnapshot
- https://manpages.debian.org/experimental/cgroup-tools/cgconfig.conf.5.en.html
- 了解下这个是做什么的 /etc/cgconfig.d/ ?
	- https://stackoverflow.com/questions/29131227/how-to-overwrite-default-cgroup-cgconfig-conf-using-cgconfig-d

## cgroup 一些不受控的地方
https://engineering.linkedin.com/blog/2016/08/don_t-let-linux-control-groups-uncontrolled

## 这个接口有使用过?
https://stackoverflow.com/questions/55507022/how-to-make-cpuset-cpu-exclusive-function-of-cpuset-work-correctly

## 有趣的 slides
https://www.usenix.org/system/files/lisa21_slides_down.pdf


## 网络为什么没有 cgroup ?

```txt
root@192:/sys/fs/cgroup# cat cgroup.controllers
cpuset cpu io memory hugetlb pids rdma misc
```

```txt
config CGROUP_NET_PRIO
	bool "Network priority cgroup"
	depends on CGROUPS
	select SOCK_CGROUP_DATA
	help
	  Cgroup subsystem for use in assigning processes to network priorities on
	  a per-interface basis.

config CGROUP_NET_CLASSID
	bool "Network classid cgroup"
	depends on CGROUPS
	select SOCK_CGROUP_DATA
	help
	  Cgroup subsystem for use as general purpose socket classid marker that is
	  being used in cls_cgroup and for netfilter matching.
```

net/core/netprio_cgroup.c

看上去还是只是 v1 支持，
```c
struct cgroup_subsys net_prio_cgrp_subsys = {
	.css_alloc	= cgrp_css_alloc,
	.css_online	= cgrp_css_online,
	.css_free	= cgrp_css_free,
	.attach		= net_prio_attach,
	.legacy_cftypes	= ss_files,
};
```

猜测是因为本来存在 qdisc 以及拥塞控制，导致内核无需

## 勉强看看
https://www.redhat.com/en/blog/world-domination-cgroups-part-6-cpuset

https://engineering.linkedin.com/blog/2016/08/don_t-let-linux-control-groups-uncontrolled

## v2 是存在详细的内核文档的
Documentation/admin-guide/cgroup-v2.rst

https://stackoverflow.com/questions/70064457/move-a-process-to-a-new-cgroup-in-cgroup-v2

## 这个不错
- https://www.alibabacloud.com/help/zh/alinux/support/differences-between-cgroup-v1-and-cgroup-v2#17831f21b2k3q

## 应该可以配合 docker 来测试

看看如果限制了 container 的资源之后，结果是什么样的

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
