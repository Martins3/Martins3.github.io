# DAMO
./damo report access

## 整理下思路
- For controlling DAMON (monitoring and monitoring-based system optimization)
  - `start`, `tune`, and `stop` are included
- For snapshot and visualization of DAMON's monitoring results and running status
  - `show`, and `status` are included
- For recording the access monitoring results and utilizing the records
  - `record`, `report` and `replay` are included
- For more convenient use of damo
  - `version` and `fmt_json` are included

- [ ] replay 是什么个搞法

## 为什么使用 perf

perf_event_damon_aggregated = 'damon:damon_aggregated'
perf_event_damon_monitor_intervals_tune = 'damon:damon_monitor_intervals_tune'
perf_event_damos_before_apply = 'damon:damos_before_apply'

```txt
	TP_printk("ctx_idx=%u scheme_idx=%u target_idx=%lu nr_regions=%u %lu-%lu: %u %u",
			__entry->context_idx, __entry->scheme_idx,
			__entry->target_idx, __entry->nr_regions,
			__entry->start, __entry->end,
			__entry->nr_accesses, __entry->age)
```

```txt
        kthreadd  6497 [031]  5702.069502: damon:damon_aggregated: target_id=0 nr_regions=12 140539517206528-140539615776768: 0 240
        kthreadd  6497 [031]  5702.069502: damon:damon_aggregated: target_id=0 nr_regions=12 140539615776768-140539713220608: 0 240
        kthreadd  6497 [031]  5702.069502: damon:damon_aggregated: target_id=0 nr_regions=12 140539713220608-140539812114432: 0 240
        kthreadd  6497 [031]  5702.069502: damon:damon_aggregated: target_id=0 nr_regions=12 140539812114432-140539817660416: 0 68
```

## scheme 中的 filter 和 quota 是做什么的?

## snapshot 是做什么的?

## temperature

## 好像这个东西就可以了

不需要配置的那么复杂的
```txt
./damo report damon
kdamond 0
    state: on, pid: 10030
    context 0
        ops: vaddr
        target 0
            pid: 2958
        intervals: sample 5 ms, aggr 100 ms, update 1 s
        nr_regions: [10, 1,000]
```
这里的 1s 的 update 是什么意思?

admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/update_us:1000000

而且这个完全不去使用 nr_regions 也是 nb 啊，似乎只有在需要获取数据的时候，才会读取这些东西。
### 似乎 scheme 这个东西完全没有作用啊

## 这里的 filter 的概念是什么?
https://github.com/damonitor/damo/blob/next/USAGE.md


## style 的内容

detailed,simple-boxes,cold,hot,
recency-sz-hist,recency-percentiles,temperature-percentiles
temperature-sz-hist,
```txt
snapshot time: [34.829 s, 34.930 s] (100.927 ms)
7[...]20000000000000000000000000000000000000000000000088888888888888888888888888888888[...]9
# min/max temperatures: -3,600,000,000, -981,030, column size: 175.597 MiB
intervals: sample 5 ms aggr 100 ms (max access hz 200)
0   addr 170.667 TiB  size 166.688 MiB access 0 hz   age 6.500 s
1   addr 255.985 TiB  size 1.371 GiB   access 0 hz   age 36 s
2   addr 255.987 TiB  size 1.367 GiB   access 0 hz   age 36 s
3   addr 255.988 TiB  size 1.351 GiB   access 0 hz   age 36 s
4   addr 255.989 TiB  size 1.359 GiB   access 0 hz   age 35.800 s
5   addr 255.991 TiB  size 1.355 GiB   access 0 hz   age 35.500 s
6   addr 255.992 TiB  size 1.364 GiB   access 0 hz   age 34.400 s
7   addr 255.993 TiB  size 1.363 GiB   access 0 hz   age 2.400 s
8   addr 255.995 TiB  size 1.323 GiB   access 0 hz   age 200 ms
9   addr 255.996 TiB  size 14.004 MiB  access 10 hz  age 100 ms
10  addr 255.996 TiB  size 1.356 GiB   access 0 hz   age 600 ms
11  addr 255.997 TiB  size 1.334 GiB   access 0 hz   age 1.100 s
12  addr 256.000 TiB  size 168.000 KiB access 0 hz   age 10.500 s
memory bw estimate: 140.039 MiB per second
total size: 13.718 GiB
record DAMON intervals: sample 5 ms, aggr 100 ms
```

### recency-percentiles
```txt
snapshot time: [34.829 s, 34.930 s] (100.927 ms)
# total recency percentiles
<percentile> <idle time>
  0      0 ns  |                    |
  1    300 ms  |                    |
 25   1.200 s  |                    |
 50  34.500 s  |******************* |
 75  36.100 s  |********************|
 99  36.100 s  |********************|
100  36.100 s  |********************|
memory bw estimate: 140.039 MiB per second
total size: 13.718 GiB
record DAMON intervals: sample 5 ms, aggr 100 ms
```

### simple-boxes cold hot

```txt
snapshot time: [100.954 ms, 202.068 ms] (101.114 ms)
       |000000000000000000000000000000000| size 166.688 MiB access 0 hz   age 1.600 s
       |000000000000000000000000000000000| size 1.371 GiB   access 0 hz   age 1.600 s
       |000000000000000000000000000000000| size 1.367 GiB   access 0 hz   age 1.600 s
       |000000000000000000000000000000000| size 1.256 GiB   access 0 hz   age 1.600 s
       |000000000000000000000000000000000| size 1.371 GiB   access 0 hz   age 1.600 s
       |000000000000000000000000000000000| size 1.032 GiB   access 0 hz   age 1.600 s
        |00000000000000000000000000000000| size 1.156 GiB   access 0 hz   age 1.500 s
        |00000000000000000000000000000000| size 1.196 GiB   access 0 hz   age 1.100 s
                                       |0| size 914.418 MiB access 0 hz   age 0 ns
                                       |5| size 46.738 MiB  access 30 hz  age 0 ns
                                       |0| size 70.109 MiB  access 0 hz   age 0 ns
              |11111111111111111111111111| size 157.746 MiB access 10 hz  age 100 ms
                                       |0| size 1.334 GiB   access 0 hz   age 0 ns
                                       |1| size 1.353 GiB   access 10 hz  age 0 ns
        |00000000000000000000000000000000| size 981.539 MiB access 0 hz   age 1.400 s
        |00000000000000000000000000000000| size 168.000 KiB access 0 hz   age 1.500 s
memory bw estimate: 16.439 GiB per second
total size: 13.718 GiB
```

### temperature-percentiles

```txt
snapshot time: [34.829 s, 34.930 s] (100.927 ms)
# total temperature percentiles
<percentile> <temperature (weights: [0, 100, 100])>
  0  -3,600,000,000  |                    |
  1  -3,600,000,000  |                    |
 25  -3,600,000,000  |                    |
 50  -3,440,000,000  |                    |
 75    -110,000,000  |******************* |
 99     -20,000,000  |******************* |
100      10,000,500  |********************|
memory bw estimate: 140.039 MiB per second
total size: 13.718 GiB
record DAMON intervals: sample 5 ms, aggr 100 ms
```

### recency-percentiles
```txt
snapshot time: [34.829 s, 34.930 s] (100.927 ms)
# total recency percentiles
<percentile> <idle time>
  0      0 ns  |                    |
  1    300 ms  |                    |
 25   1.200 s  |                    |
 50  34.500 s  |******************* |
 75  36.100 s  |********************|
 99  36.100 s  |********************|
100  36.100 s  |********************|
memory bw estimate: 140.039 MiB per second
total size: 13.718 GiB
record DAMON intervals: sample 5 ms, aggr 100 ms
```

### recency-sz-hist

```txt
<idle time (us)> <total size>
[0 ns, 3.610 s)      5.389 GiB   |**************      |
[3.610 s, 7.220 s)   166.688 MiB |*                   |
[7.220 s, 10.830 s)  168.000 KiB |*                   |
[10.830 s, 14.440 s) 0 B         |                    |
[14.440 s, 18.050 s) 0 B         |                    |
[18.050 s, 21.660 s) 0 B         |                    |
[21.660 s, 25.270 s) 0 B         |                    |
[25.270 s, 28.880 s) 0 B         |                    |
[28.880 s, 32.490 s) 0 B         |                    |
[32.490 s, 36.100 s) 8.166 GiB   |********************|
memory bw estimate: 140.039 MiB per second
total size: 13.718 GiB
record DAMON intervals: sample 5 ms, aggr 100 ms
```

## 是的确可以不去使用 perf 的

sudo ./damo start
sudo ./damo report access

这个就可以直接看到原始数据了。
sudo ./damo report access --json

nb 的 auto-tuning 啊:

## 调试的小技巧

src/_damo_fs.py 中，将其修改为 debug_do_print = True 之后，可以看到和文件的交换过程是什么


开启一个 process 的跟踪之后，可以看到结果是这样的:
```txt
read '/sys/kernel/mm/damon/admin/kdamonds/0/state': 'off'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/nr_contexts': '1'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/nr_regions/min': '10'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/nr_regions/max': '1000'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/sample_us': '5000'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/aggr_us': '100000'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/update_us': '1000000'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/min_sample_us': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/aggrs': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/access_bp': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/max_sample_us': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/operations': 'paddr'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/nr_targets': '1'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/regions/nr_regions': '1'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/regions/0/end': '9496821759'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/regions/0/start': '1073741824'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/pid_target': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/nr_schemes': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/avail_operations': 'vaddr
fvaddr
paddr'
read '/sys/kernel/mm/damon/admin/kdamonds/0/state': 'off'
read '/sys/kernel/mm/damon/admin/kdamonds/0/pid': '-1'
read '/sys/kernel/mm/damon/admin/kdamonds/nr_kdamonds': '1'
read '/sys/kernel/mm/damon/admin/kdamonds/nr_kdamonds': '1'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/nr_contexts': '1'
write 'vaddr' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/operations'
write '5000' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/sample_us'
write '100000' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/aggr_us'
write '1000000' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/update_us'
write '0' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/access_bp'
write '0' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/aggrs'
write '0' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/min_sample_us'
write '0' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/max_sample_us'
write '10' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/nr_regions/min'
write '1000' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/nr_regions/max'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/nr_targets': '1'
write '97336' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/pid_target'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/regions/nr_regions': '1'
write '0' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/regions/nr_regions'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/nr_schemes': '0'
write 'on' to '/sys/kernel/mm/damon/admin/kdamonds/0/state'
read '/sys/kernel/mm/damon/admin/kdamonds/nr_kdamonds': '1'
read '/sys/kernel/mm/damon/admin/kdamonds/0/state': 'on'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/nr_contexts': '1'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/nr_regions/min': '10'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/nr_regions/max': '1000'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/sample_us': '5000'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/aggr_us': '100000'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/update_us': '1000000'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/min_sample_us': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/aggrs': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/access_bp': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/max_sample_us': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/operations': 'vaddr'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/nr_targets': '1'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/regions/nr_regions': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/pid_target': '97336'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/nr_schemes': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/avail_operations': 'vaddr
fvaddr
paddr'
read '/sys/kernel/mm/damon/admin/kdamonds/0/state': 'on'
read '/sys/kernel/mm/damon/admin/kdamonds/0/pid': '103699'
read '/sys/kernel/mm/damon/admin/kdamonds/nr_kdamonds': '1'
```
所以，就是使用 perf 来观察所有的内容的，不需要这些 region 的东西

## 如果是 damon 的情况后

```txt
read '/sys/kernel/mm/damon/admin/kdamonds/nr_kdamonds': '1'
read '/sys/kernel/mm/damon/admin/kdamonds/0/state': 'on'
read '/sys/kernel/mm/damon/admin/kdamonds/nr_kdamonds': '1'
read '/sys/kernel/mm/damon/admin/kdamonds/0/state': 'on'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/nr_contexts': '1'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/nr_regions/min': '10'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/nr_regions/max': '1000'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/sample_us': '5000'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/aggr_us': '100000'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/update_us': '1000000'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/min_sample_us': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/aggrs': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/access_bp': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/max_sample_us': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/operations': 'paddr'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/nr_targets': '1'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/regions/nr_regions': '1'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/regions/0/end': '9496821759'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/regions/0/start': '1073741824'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/pid_target': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/nr_schemes': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/avail_operations': 'vaddr
fvaddr
paddr'
read '/sys/kernel/mm/damon/admin/kdamonds/0/state': 'on'
read '/sys/kernel/mm/damon/admin/kdamonds/0/pid': '110285'
read '/sys/kernel/mm/damon/admin/kdamonds/nr_kdamonds': '1'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/nr_contexts': '1'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/nr_regions/min': '10'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/nr_regions/max': '1000'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/sample_us': '5000'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/aggr_us': '100000'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/update_us': '1000000'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/min_sample_us': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/aggrs': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/access_bp': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/max_sample_us': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/operations': 'paddr'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/nr_targets': '1'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/regions/nr_regions': '1'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/regions/0/end': '9496821759'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/regions/0/start': '1073741824'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/pid_target': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/nr_schemes': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/avail_operations': 'vaddr
fvaddr
paddr'
read '/sys/kernel/mm/damon/admin/kdamonds/0/state': 'on'
read '/sys/kernel/mm/damon/admin/kdamonds/0/pid': '110285'
read '/sys/kernel/mm/damon/admin/kdamonds/nr_kdamonds': '1'
read '/sys/kernel/mm/damon/admin/kdamonds/nr_kdamonds': '1'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/nr_contexts': '1'
write 'paddr' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/operations'
write '5000' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/sample_us'
write '100000' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/aggr_us'
write '1000000' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/update_us'
write '0' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/access_bp'
write '0' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/aggrs'
write '0' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/min_sample_us'
write '0' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/max_sample_us'
write '10' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/nr_regions/min'
write '1000' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/nr_regions/max'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/nr_targets': '1'
write '0' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/pid_target'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/regions/nr_regions': '1'
write '1073741824' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/regions/0/start'
write '9496821759' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/regions/0/end'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/nr_schemes': '0'
write '1' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/nr_schemes'
write '0' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/access_pattern/sz/min'
write '18446744073709551615' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/access_pattern/sz/max'
write '0' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/access_pattern/nr_accesses/min'
write '3689348814741910528' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/access_pattern/nr_accesses/max'
write '0' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/access_pattern/age/min'
write '184467440737095' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/access_pattern/age/max'
write 'stat' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/action'
write '0' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/quotas/ms'
write '0' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/quotas/bytes'
write '18446744073709551615' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/quotas/reset_interval_ms'
write '0' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/quotas/weights/sz_permil'
write '0' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/quotas/weights/nr_accesses_permil'
write '0' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/quotas/weights/age_permil'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/quotas/goals/nr_goals': '0'
write 'none' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/watermarks/metric'
write '0' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/watermarks/interval_us'
write '0' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/watermarks/high'
write '0' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/watermarks/mid'
write '0' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/watermarks/low'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/core_filters/nr_filters': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/ops_filters/nr_filters': '0'
write '5000' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/apply_interval_us'
write 'commit' to '/sys/kernel/mm/damon/admin/kdamonds/0/state'
write 'update_schemes_stats' to '/sys/kernel/mm/damon/admin/kdamonds/0/state'
write 'update_tuned_intervals' to '/sys/kernel/mm/damon/admin/kdamonds/0/state'
write 'update_schemes_tried_regions' to '/sys/kernel/mm/damon/admin/kdamonds/0/state'

read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/nr_contexts': '1'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/nr_regions/min': '10'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/nr_regions/max': '1000'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/sample_us': '5000'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/aggr_us': '100000'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/update_us': '1000000'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/min_sample_us': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/aggrs': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/access_bp': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/max_sample_us': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/operations': 'paddr'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/nr_targets': '1'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/regions/nr_regions': '1'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/regions/0/end': '9496821759'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/regions/0/start': '1073741824'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/pid_target': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/nr_schemes': '1'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/ops_filters/nr_filters': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/filters/nr_filters': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/17/age': '261'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/17/end': '4852350976'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/17/nr_accesses': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/17/sz_filter_passed': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/17/start': '4431872000'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/45/age': '17'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/45/end': '9472180224'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/45/nr_accesses': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/45/sz_filter_passed': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/45/start': '9250668544'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/35/age': '43'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/35/end': '8563195904'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/35/nr_accesses': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/35/sz_filter_passed': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/35/start': '7987134464'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/7/age': '284'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/7/end': '2749038592'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/7/nr_accesses': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/7/sz_filter_passed': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/7/start': '2664955904'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/25/age': '20'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/25/end': '6271860736'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/25/nr_accesses': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/25/sz_filter_passed': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/25/start': '6113722368'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/15/age': '280'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/15/end': '4431872000'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/15/nr_accesses': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/15/sz_filter_passed': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/15/start': '3926851584'

read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/43/age': '6'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/43/end': '9250668544'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/43/nr_accesses': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/43/sz_filter_passed': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/43/start': '9250144256'

read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/33/age': '43'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/33/end': '7987134464'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/33/nr_accesses': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/33/sz_filter_passed': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/33/start': '7740260352'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/5/age': '284'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/5/end': '2664955904'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/5/nr_accesses': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/5/sz_filter_passed': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/5/start': '1908342784'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/23/age': '18'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/23/end': '6113722368'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/23/nr_accesses': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/23/sz_filter_passed': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/23/start': '5525143552'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/13/age': '280'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/13/end': '3926851584'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/13/nr_accesses': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/13/sz_filter_passed': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/13/start': '3590193152'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/41/age': '6'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/41/end': '9250144256'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/41/nr_accesses': '1'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/41/sz_filter_passed': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/41/start': '9249619968'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/31/age': '28'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/31/end': '7740260352'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/31/nr_accesses': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/31/sz_filter_passed': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/31/start': '7656636416'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/3/age': '286'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/3/end': '1908342784'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/3/nr_accesses': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/3/sz_filter_passed': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/3/start': '1324089344'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/21/age': '18'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/21/end': '5525143552'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/21/nr_accesses': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/21/sz_filter_passed': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/21/start': '5272895488'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/11/age': '283'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/11/end': '3590193152'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/11/nr_accesses': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/11/sz_filter_passed': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/11/start': '2917269504'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/1/age': '286'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/1/end': '1324089344'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/1/nr_accesses': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/1/sz_filter_passed': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/1/start': '1073741824'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/total_bytes': '8423079936'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/39/age': '32'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/39/end': '9249619968'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/39/nr_accesses': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/39/sz_filter_passed': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/39/start': '8700428288'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/29/age': '28'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/29/end': '7656636416'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/29/nr_accesses': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/29/sz_filter_passed': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/29/start': '6904479744'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/19/age': '261'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/19/end': '5272895488'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/19/nr_accesses': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/19/sz_filter_passed': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/19/start': '4852350976'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/47/age': '17'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/47/end': '9496821760'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/47/nr_accesses': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/47/sz_filter_passed': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/47/start': '9472180224'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/37/age': '32'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/37/end': '8700428288'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/37/nr_accesses': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/37/sz_filter_passed': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/37/start': '8563195904'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/9/age': '283'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/9/end': '2917269504'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/9/nr_accesses': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/9/sz_filter_passed': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/9/start': '2749038592'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/27/age': '20'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/27/end': '6904479744'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/27/nr_accesses': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/27/sz_filter_passed': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/27/start': '6271860736'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/apply_interval_us': '5000'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/stats/qt_exceeds': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/stats/nr_applied': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/stats/sz_ops_filter_passed': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/stats/sz_tried': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/stats/nr_tried': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/stats/sz_applied': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/core_filters/nr_filters': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/quotas/goals/nr_goals': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/quotas/effective_bytes': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/quotas/bytes': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/quotas/weights/age_permil': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/quotas/weights/sz_permil': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/quotas/weights/nr_accesses_permil': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/quotas/ms': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/quotas/reset_interval_ms': '18446744073709551615'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/target_nid': '-1'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/action': 'stat'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/access_pattern/age/min': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/access_pattern/age/max': '184467440737095'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/access_pattern/nr_accesses/min': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/access_pattern/nr_accesses/max': '3689348814741910528'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/access_pattern/sz/min': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/access_pattern/sz/max': '18446744073709551615'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/watermarks/mid': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/watermarks/high': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/watermarks/low': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/watermarks/interval_us': '0'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/watermarks/metric': 'none'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/avail_operations': 'vaddr
fvaddr
paddr'
read '/sys/kernel/mm/damon/admin/kdamonds/0/state': 'on'
read '/sys/kernel/mm/damon/admin/kdamonds/0/pid': '110285'
read '/sys/kernel/mm/damon/admin/kdamonds/nr_kdamonds': '1'
read '/sys/kernel/mm/damon/admin/kdamonds/nr_kdamonds': '1'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/nr_contexts': '1'
write 'paddr' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/operations'
write '5000' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/sample_us'
write '100000' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/aggr_us'
write '1000000' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/update_us'
write '0' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/access_bp'
write '0' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/aggrs'
write '0' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/min_sample_us'
write '0' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/max_sample_us'
write '10' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/nr_regions/min'
write '1000' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/nr_regions/max'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/nr_targets': '1'
write '0' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/pid_target'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/regions/nr_regions': '1'
write '1073741824' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/regions/0/start'
write '9496821759' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/regions/0/end'
read '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/nr_schemes': '1'
write '0' to '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/nr_schemes'
write 'commit' to '/sys/kernel/mm/damon/admin/kdamonds/0/state'
```

## 似乎是一个 context 下可以多一个
https://docs.kernel.org/admin-guide/mm/damon/usage.html#sysfs-schemes

## 是一个 context 监控一个 process 还是多个 process

## 似乎 damon 可以自动跟踪被 fork 出来的 process

## 为什么不可以支持细粒度的分析，既然 lru 都支持了。
似乎是可以慢慢解决的

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
