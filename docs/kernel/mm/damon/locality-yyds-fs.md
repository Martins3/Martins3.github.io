# yyds-fs DAMON 局部性实验

这个实验在 host 上用 DAMON 观察 collei 虚拟机 `yyds-fs`。

本实验真正有意义的模式是 `--guest-ram`。它会解析 QEMU 进程的
`/proc/<pid>/maps`，找到 `/memfd:memory-backend-memfd`，然后用 DAMON
`fvaddr` 只监控这一段映射。

当前 `yyds-fs` 的 guest RAM 映射为：

```text
0x00007faeabfff000-0x00007fb0abfff000 8.000 GiB /memfd:memory-backend-memfd
```

这段地址是 QEMU 中承载 guest RAM 的 HVA。观察它可以回答：guest 内部的
GVA 访问经过 GVA->GPA，再表现为 QEMU RAMBlock/HVA offset 之后，原本的局部性还剩多少。

## 构建

```bash
cd /home/martins3/data/vn/docs/kernel/mm/damon
gcc -O2 -Wall -Wextra -o damon_locality.out damon_locality.c
gcc -O2 -Wall -Wextra -o damon_guest_load.out damon_guest_load.c
gcc -O2 -Wall -Wextra -o redis_resp_load.out redis_resp_load.c
ssh martins3@10.0.70.0 'cd /home/martins3/data/vn/docs/kernel/mm/damon && gcc -O2 -Wall -Wextra -o damon_guest_load.guest.out damon_guest_load.c'
ssh -p 51404 martins3@localhost 'cd /home/martins3/data/vn/docs/kernel/mm/damon && gcc -O2 -Wall -Wextra -o redis_resp_load.guest.out redis_resp_load.c'
```

## 命令

空闲基线：

```bash
sudo ./damon_locality.out --vm yyds-fs --duration 10 --snapshot-ms 1000 --top 5
sudo ./damon_locality.out --vm yyds-fs --guest-ram --duration 10 --snapshot-ms 1000 --min-regions 100 --max-regions 1000 --top 8
```

顺序访问 hotset：

```bash
ssh martins3@10.0.70.0 'cd /home/martins3/data/vn/docs/kernel/mm/damon && nohup ./damon_guest_load.guest.out --mode seq-hotset --total-mb 2048 --hot-mb 256 --seconds 20 > /tmp/damon_seq_hotset.log 2>&1 &'
sudo ./damon_locality.out --vm yyds-fs --guest-ram --duration 12 --snapshot-ms 1000 --min-regions 100 --max-regions 1000 --top 8
```

随机访问 256 MiB hotset：

```bash
ssh martins3@10.0.70.0 'cd /home/martins3/data/vn/docs/kernel/mm/damon && nohup ./damon_guest_load.guest.out --mode random-hotset --total-mb 2048 --hot-mb 256 --seconds 20 > /tmp/damon_random_hotset.log 2>&1 &'
sudo ./damon_locality.out --vm yyds-fs --guest-ram --duration 12 --snapshot-ms 1000 --min-regions 100 --max-regions 1000 --top 8
```

随机访问完整 2 GiB 区间：

```bash
ssh martins3@10.0.70.0 'cd /home/martins3/data/vn/docs/kernel/mm/damon && nohup ./damon_guest_load.guest.out --mode random-full --total-mb 2048 --seconds 20 > /tmp/damon_random_full.log 2>&1 &'
sudo ./damon_locality.out --vm yyds-fs --guest-ram --duration 12 --snapshot-ms 1000 --min-regions 100 --max-regions 1000 --top 8
```

host 物理机本地 workload 对照：

```bash
./damon_guest_load.out --mode seq-hotset --total-mb 2048 --hot-mb 256 --seconds 25 &
load_pid=$!
sudo ./damon_locality.out --pid "$load_pid" --duration 8 --snapshot-ms 1000 --min-regions 100 --max-regions 1000 --top 8
wait "$load_pid"

./damon_guest_load.out --mode random-hotset --total-mb 2048 --hot-mb 256 --seconds 25 &
load_pid=$!
sudo ./damon_locality.out --pid "$load_pid" --duration 8 --snapshot-ms 1000 --min-regions 100 --max-regions 1000 --top 8
wait "$load_pid"

./damon_guest_load.out --mode random-full --total-mb 2048 --seconds 25 &
load_pid=$!
sudo ./damon_locality.out --pid "$load_pid" --duration 8 --snapshot-ms 1000 --min-regions 100 --max-regions 1000 --top 8
wait "$load_pid"
```

Redis/Docker 对照实验：

```bash
# host Redis
docker run -d --name damon-redis-host --network host redis:7-alpine \
  redis-server --port 6380 --save "" --appendonly no --protected-mode no

# guest Redis
ssh -p 51404 martins3@localhost \
  'echo a | sudo -S docker run -d --name damon-redis-guest --network host redis:7-alpine \
  redis-server --port 6380 --save "" --appendonly no --protected-mode no'

# 准备相同数据集：262144 个 key，每个 value 4096 字节，约 1 GiB value payload。
./redis_resp_load.out --port 6380 --mode prepare \
  --total-keys 262144 --hot-keys 65536 --value-bytes 4096 --pipeline 128

ssh -p 51404 martins3@localhost \
  'cd /home/martins3/data/vn/docs/kernel/mm/damon && ./redis_resp_load.guest.out --port 6380 --mode prepare \
  --total-keys 262144 --hot-keys 65536 --value-bytes 4096 --pipeline 128'
```

host Redis GET hotset，DAMON 观察容器内 `redis-server` 的 host PID：

```bash
redis_pid=$(docker inspect -f '{{.State.Pid}}' damon-redis-host)
./redis_resp_load.out --port 6380 --mode get-hotset \
  --total-keys 262144 --hot-keys 65536 --value-bytes 4096 --pipeline 128 --seconds 45 &
load_pid=$!
echo a | sudo -S ./damon_locality.out --pid "$redis_pid" \
  --duration 15 --snapshot-ms 1000 --min-regions 100 --max-regions 1000 --top 8
wait "$load_pid"
```

guest Redis GET hotset，host DAMON 观察 QEMU guest RAM memfd：

```bash
ssh -p 51404 martins3@localhost \
  'cd /home/martins3/data/vn/docs/kernel/mm/damon && ./redis_resp_load.guest.out --port 6380 --mode get-hotset \
  --total-keys 262144 --hot-keys 65536 --value-bytes 4096 --pipeline 128 --seconds 45' &
load_pid=$!
echo a | sudo -S ./damon_locality.out --vm yyds-fs --guest-ram \
  --duration 15 --snapshot-ms 1000 --min-regions 100 --max-regions 1000 --top 8
wait "$load_pid"
```

`set-hotset` 可以替换 `get-hotset`，它会随机覆盖同一个 hotset，比 GET 更偏向写数据页。
但 Redis 的访问路径仍然会受到协议解析、client buffer、dict/robj 元数据和 jemalloc 复用影响。

## 观测结果

过滤后的空闲基线：

- 监控范围正好是 QEMU 的 8 GiB guest RAM memfd。
- 大多数 snapshot 没有访问，或者只有很少访问。
- 偶发访问来自 guest RAM 内部的 guest 背景活动，不再是 QEMU 代码段、堆、线程栈之类的噪声。

2 GiB 分配中顺序访问 256 MiB hotset：

- 第一轮稳定后，活跃 HVA 大多约为 `0.24-0.31 GiB`。
- 这个范围接近预期的 256 MiB guest hotset。
- Gini 较高，稳定 snapshot 大约在 `0.87-0.95`。
- top regions 通常贡献约 `55-70%` 的访问。
- 解释：顺序 GVA 访问的局部性，大部分可以保留到 HVA/GPA 视角。

2 GiB 分配中随机访问 256 MiB hotset：

- 忙时 snapshot 中，活跃 HVA 仍然约为 `0.24-0.35 GiB`。
- region 数上升到约 `200-300`。
- hot islands 上升到约 `30-50`。
- 后期忙时 snapshot 中，top-8 access share 降到约 `7-15%`。
- 解释：bounded hotset 经过 GVA->GPA/HVA 之后仍然存在，但 hotset 内部的局部性明显碎片化。

随机访问完整 2 GiB 分配：

- 忙时 snapshot 中，活跃 HVA 约为 `1.0-1.8 GiB`。
- region 数约为 `160-450`。
- broad-access snapshot 中，top-8 access share 明显更低，常见约 `6-23%`。
- workload 结束后的后期 snapshot，主要剩下很小的 guest 背景热页。
- 解释：完整 2 GiB 随机 GVA 访问在 HVA 上明显更分散，比 256 MiB hotset 场景弱得多。

host 物理机本地 workload：

- 顺序访问 256 MiB hotset 时，总 vaddr 监控范围约 `2.708 GiB`，活跃范围稳定在约
  `0.249-0.252 GiB`，占比约 `9.2-9.3%`。
- host 随机访问 256 MiB hotset 时，活跃范围约 `0.194-0.222 GiB`，占比约
  `9.5-10.8%`。
- host 随机访问完整 2 GiB 时，活跃范围约 `1.432-1.697 GiB`，占比约
  `61.0-72.3%`。
- 解释：在物理机本地进程上，DAMON 能非常直接地把 256 MiB hotset 和完整 2 GiB
  random-full 区分开。这个结果比 QEMU guest RAM 视角更干净，因为少了 guest 页表、
  guest allocator 和 RAMBlock 映射关系带来的扰动。

Redis/Docker workload：

- Redis 数据集：host 和 guest 都是 Docker 中运行的 `redis:7-alpine`，数据规模都约为
  `1.27G`，`used_memory_dataset` 约 `1.346G`，fragmentation ratio 约 `1.01-1.02`。
- workload 参数：`262144` 个 key，每个 value `4096` 字节；hotset 为前 `65536`
  个 key，理论 value payload 约 `256 MiB`。

- host Docker Redis，host DAMON `vaddr` 观察 `redis-server`：
  - `get-hotset`：active 大多约 `0.17-0.25 GiB`，接近理论 256 MiB hotset。
  - `set-hotset`：active 大多约 `0.035-0.153 GiB`，比理论 hotset 小，热点更多集中在
    Redis 协议解析、client buffer、dict/robj 元数据和 jemalloc 复用到的页。

- guest Docker Redis，host DAMON `fvaddr` 观察 QEMU guest RAM memfd：
  - `get-hotset`：前两个 snapshot 能看到 `0.138 GiB`、`0.548 GiB` 级别的 active
    guest RAM，随后主要变成大量 4 KiB 到几十 KiB 的高访问小页，active bytes 通常很小。
  - `set-hotset`：前两个 snapshot 为 `0.241 GiB`、`0.285 GiB`，接近 256 MiB hotset；
    随后多数 snapshot 只剩小热页，偶发 `0.047-0.188 GiB` 的 active 范围。

- guest 内部直接用 DAMON `vaddr` 观察 Docker Redis 时，`get-hotset` 也没有稳定显示
  256 MiB active，而是约 `0.016-0.075 GiB`。这说明 Redis workload 本身已经不是
  简单的线性数组 hotset，不能把 value payload 大小直接等同于 DAMON active bytes。

## 结论

对 `yyds-fs` 来说，用 DAMON `fvaddr` 过滤到 QEMU guest RAM 后可以看到：局部性不会因为 GVA->GPA->HVA 转换就完全消失。

- 顺序访问的 256 MiB GVA hotset，在 HVA 视角下仍接近 256 MiB 活跃集合。
- 随机访问的 256 MiB GVA hotset，在 HVA 视角下也仍然 bounded，但会碎成很多 HVA hot islands。
- 随机访问完整 2 GiB 时，HVA 活跃范围扩展到约 1-2 GiB，局部性显著变弱。

所以当前实验的回答是：

**hotset 的大小大体能保留下来，但细粒度连续性和局部性会被 guest 分配、guest 页表映射和背景 guest 活动削弱。**

如果目标是用 Redis 继续验证这个结论，需要更谨慎地解释：

- 对简单数组 workload，256 MiB hotset 在 host `vaddr` 和 QEMU guest RAM `fvaddr`
  中都比较容易表现为接近 256 MiB 的 active 范围。
- 对 Redis workload，host Docker Redis 的 `get-hotset` 仍能大致看出 256 MiB hotset；
  但 `set-hotset` 和 guest Docker Redis 都明显更碎，active bytes 不再稳定等于 value
  payload hotset 大小。
- 因此 Redis 更适合证明“真实应用会削弱细粒度局部性”，而不是作为纯粹证明
  “hotset 大小严格保留”的 workload。

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
