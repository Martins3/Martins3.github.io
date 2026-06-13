# cgroup 的操作手册

## cpuset
https://www.redhat.com/en/blog/world-domination-cgroups-part-6-cpuset

```sh
sudo cgcreate -g cpuset:testset
sudo cgset -r cpuset.cpus=3 testset
sudo cgset -r cpuset.mems=0 testset
sudo cgexec -g cpuset:testset stress-ng --vm-bytes 6500M --vm-keep --vm 3
```

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
