## net_cls

似乎相关的文件为:
net/sched/cls_cgroup.c
net/core/netclassid_cgroup.c

还是看不太懂
```sh
sudo cgcreate -g net_cls:/test_group
# 设置 classid (0xAAAABBBB 格式)
sudo cgset -r net_cls.classid=0x100001 test_group

# 将当前shell加入cgroup
sudo cgexec -g net_cls:test_group bash
# 或者将特定PID加入cgroup
sudo cgclassify -g net_cls:test_group $pid

# 添加排队规则
sudo tc qdisc add dev eth0 root handle 1: htb
# 添加cgroup过滤器
sudo tc filter add dev eth0 parent 1: handle 1: cgroup
# 可选：添加带宽限制类
sudo tc class add dev eth0 parent 1: classid 1:1 htb rate 1mbit
```

## net_prio

net/core/netprio_cgroup.c

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
