## perf

1. 记录所有的到达 `tcp_sendmsg` 的 backtrace 和比例
```sh
perf probe 'tcp_sendmsg'
perf record -e probe:tcp_sendmsg -a -g sleep 10
```
得到的数据可以使用 flamegraph 处理一下，可以得到非常形象的图形。

而使用 bpftrace 可以获取到更加精细的数值统计。

2. 统计所有的 trace point
```c
sudo perf stat -e 'kvm:*' -a sleep 1s
```

3. 实时统计
perf top -e kvm:kvm_nested_vmrun

4. kprobe 获取返回值
  - perf probe cpufreq_cpu_get%return


### [ ] 处理一下 perf 中 unknow 的数值情况

例如下面，几乎显示所有的函数都是被一个 unknow 的函数调用的，是不是
哪里有点问题吧!
![](../img/qemu-guest-fio.svg)

### [ ] 为什么 perf 无法正确 perf 到用户态的程序
perf report 的比例明显不对啊

### 常用的操作方法
- perf list kvm
- perf diff perf-1.data perf-2.data ： 按照 diff 差距来分析

更多的参考:
- https://www.brendangregg.com/perf.html
- https://jvns.ca/perf-zine.pdf

## 这个完整的文档可以看看
- https://perf.wiki.kernel.org/index.php/Main_Page
- https://perf.wiki.kernel.org/index.php/Tutorial#Benchmarking_with_perf_bench

## perf 还可以支持 python
- https://docs.python.org/pt-br/dev/howto/perf_profiling.html

## perf 可以作为 ftrace 的前端
也可以作为 ftrace 使用:
perf ftrace is a simple *wrapper* for kernel's ftrace functionality, and only supports single thread tracing now.
```plain
perf ftrace -T __kmalloc ./add_vec
perf ftrace ./add_vec
```

## perf sched
- perf sched record
- perf report

## 使用 perf stat -e cycles 是不是可以测试 CPU 频率
