# drgn
<!-- 15d529a2-5499-49f5-9d08-71569631c780 -->

相关资料:
- https://lwn.net/Articles/952942/
- https://lwn.net/Articles/789641/
- https://utcc.utoronto.ca/~cks/space/blog/linux/DrgnKernelPokingPraise
- https://developers.facebook.com/blog/post/2021/12/09/drgn-how-linux-kernel-team-meta-debugs-kernel-scale/

## 环境准备
- https://drgn.readthedocs.io/en/latest/installation.html
- https://drgn.readthedocs.io/en/latest/getting_debugging_symbols.html

### Fedora 环境

Fedora 44 直接安装系统包即可:

```sh
sudo dnf install drgn
drgn --version
```

准备 debuginfo
```sh
mkdir -p  /lib/modules/$(uname -r)
scp martins3@10.0.2.2:/home/martins3/data/kernel/linux-build/vmlinux  /lib/modules/$(uname -r)
```

很容易走通，这两个东西真的震撼我了，的确比使用 crash 好太多了
```py
task = find_task(115)
cmdline(task)
```
用这个来分析内核真的不错的

```sh
cd /home/martins3/data/vn
sudo drgn --debug-directory ~/data/kernel/linux-full/vmlinux drgn-kvm-analysis.py

sudo drgn --debug-directory ~/data/kernel/linux-full/vmlinux drgn-kvm-vm-parser.py
```

## 使用的经典案例
1. 使用 drgn 来分析 workqueue : ~/docs/kernel/irq/softirq/workqueue.md
2. ./scripts/ 下
3. 查看全系统 inode-backed page cache 按文件占用:

```sh
cd /home/martins3/data/vn/docs/kernel/tutorial/drgn/scripts
sudo drgn -c /proc/kcore ./page_cache_by_file.py --top 50
```

4. 分析 QEMU/VFIO 使用 iommufd 时的核心对象关系:

```sh
cd /home/martins3/data/vn/docs/kernel/tutorial/drgn/scripts
sudo drgn -k ./iommufd_relationship.py --pid $(cat ~/data/hack/vm/yyds-nv/s/pid)
```

### 定位 swap 占用来源：

```sh
sudo drgn -k /home/martins3/data/vn/docs/kernel/tutorial/drgn/scripts/swap_usage.py --top 30
```

`swap_usage.py` 显示设备占用、按 `mm` 去重的匿名 swap 排名、按 inode 去重的
tmpfs/shmem 排名，以及 memfd 名称和目录前缀的聚合。文件行包含 fd/mmap
持有者，可以定位 QEMU 的 guest RAM；扫描所有 tmpfs inode，也能找到没有
打开者但仍占 swap 的具名文件。`--all` 显示全部，`--no-holders` 跳过持有者扫描。
支持 `drgn -c vmcore -s vmlinux ./swap_usage.py`，需要匹配的内核调试符号。

匿名 `MM_SWAPENTS` 不含 shmem 文件的 swap entries，且不同 `mm` 可能共享
同一个 swap slot。SwapCached 与归属统计存在重叠，不能直接相加；汇总差额
不等于泄漏。zram 的 USED 是未压缩逻辑大小。读取失败会在末尾明确报告。
使用的 API 见 [drgn swap helpers](https://drgn.readthedocs.io/en/latest/helpers.html#module-drgn.helpers.linux.swap)，
SwapCached 的含义见 [内核 proc 文档](https://www.kernel.org/doc/html/latest/filesystems/proc.html)。

基本的调查思路为

1. 先确定总量和设备分布
   脚本 (docs/kernel/tutorial/drgn/scripts/swap_usage.py) 的 show_devices() 遍历 swap_info_struct，读取各设备的已用页数。
   本机是 zram 约 30.75 GiB，/dev/sdb 约 50.25 GiB，合计 81 GiB。这里都是逻辑页大小，zram 的数字不是压缩后消耗的 RAM。

2. 查进程的匿名 swap
   collect_processes() 遍历所有 task，按 task.mm 地址去重，再通过 drgn 的 task_rss() 读取 MM_SWAPENTS。
   按 mm 去重，是因为多个线程通常共享同一个地址空间，不能每个线程重复算一次。
   结果只有 11.71 GiB，远小于全局的 81 GiB。这就提示：大头需要从进程匿名页以外继续找。

3. 查 tmpfs/shmem 的换出页
   collect_shmem() 沿着下面的关系遍历：

   super_blocks
     → 文件系统类型为 tmpfs 的 super_block
       → s_inodes 中的普通文件 inode
         → shmem_inode_info.swapped

   swapped 表示该文件的换出页数；i_mapping.nrpages 是常驻页数，i_size 是逻辑文件大小，三者要分开看。
   这一步覆盖普通 tmpfs 文件、memfd、SysV 共享内存和共享匿名映射，也能找到没有进程打开、但文件仍然存在的占用。
   本机查出了 68.54 GiB，大头就找到了。

4. 聚合文件名称，再追持有者
   shmem_group() 按 memfd 名称或目录前缀聚合，发现 107 个 memory-backend-memfd 合计占了 61.86 GiB。
   然后 collect_holders() 扫描进程的 fd 表和 VMA，通过 file.f_inode 与这些 inode 匹配，把文件关联到 PID。这样确认最大的 9.33 GiB 属
   于 PID 1950938 的 QEMU 内存后端。

脚本回答的是“现在哪些对象占着 swap”。
匿名页可能跨进程共享同一个 swap slot，SwapCached 也与这些统计重叠，因此不能靠简
单相加精确对账；它也不能仅凭当前快照说明这些页当初为何被换出。

## TODO
https://drgn.readthedocs.io/en/latest/tutorials/blk_rq_qos_crash.html
oracle 的扩展 : https://github.com/oracle-samples/drgn-tools

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
