# pstore
<!-- ab68ad3f-884b-46be-9942-0f0fc113d7c5 -->

- Documentation/admin-guide/ramoops.rst
- Documentation/admin-guide/pstore-blk.rst
- Documentation/admin-guide/ramoops.rst.
- Documentation/devicetree/bindings/reserved-memory/ramoops.txt

- https://blogs.oracle.com/linux/post/pstore-linux-kernel-persistent-storage-file-system
- https://docs.kernel.org/admin-guide/pstore-blk.html
- fs/pstore/

其他的参考
- https://mp.weixin.qq.com/s/w2_cfXQUq1yxM1JgzpTQNA

## 基本测试

```txt
[  826.012002] pstore: Using crash dump compression: deflate
[  826.019085] printk: legacy console [pstore_blk-1] enabled
[  826.020383] pstore: Registered pstore_blk as persistent store backend
[  826.020549] pstore_blk: attached /dev/vdc (16777216) (no dedicated panic_write!)
[  828.041391] printk: legacy console [pstore_blk-1] disabled
[  828.057831] pstore: Unregistered pstore_blk as persistent store backend
[  828.127622] pstore_zone: registered pstore_blk as backend for kmsg(Oops) pmsg console
[  828.128043] pstore: Using crash dump compression: deflate
[  828.136716] printk: legacy console [pstore_blk-1] enabled
[  828.137942] pstore: Registered pstore_blk as persistent store backend
[  828.138124] pstore_blk: attached /dev/vdc (16777216) (no dedicated panic_write!)
```

配置参数:
```txt
memmap=4M$0x100000000 ramoops.mem_address=0x100000000 ramoops.mem_size=0x400000

ramoops.console_size=0x200000 ramoops.pmsg_size=0x100000

pstore_blk.blkdev=/dev/vdc pstore_blk.best_effort=1 pstore.backend=pstore_blk
```

检查后端
```txt
/sys/module/pstore/parameters/backend
```

```txt
[root@Node-172-20-128-106 18:56:09 pstore]$ mount | grep pstore
pstore on /sys/fs/pstore type pstore (rw,nosuid,nodev,noexec,relatime)
```

在 /sys/fs/pstore 下面可以直接写入 ram 中吗，还是只读的


有时候可以观察到写入失败:
```txt
[  556.369810][ T3839] pstore: backend (erst) writing error (-28)
```

```txt
[  556.195087][ T3839] Call Trace:
[  556.198622][ T3839]  <TASK>
[  556.201775][ T3839]  dump_stack_lvl+0x32/0x50
[  556.206688][ T3839]  panic+0x340/0x360
[  556.210915][ T3839]  ? _printk+0x60/0x80
[  556.215340][ T3839]  sysrq_handle_crash+0x11/0x20
[  556.220644][ T3839]  __handle_sysrq+0x9b/0x190
[  556.225652][ T3839]  write_sysrq_trigger+0x24/0x40
[  556.231049][ T3839]  proc_reg_write+0x55/0xa0
[  556.235960][ T3839]  vfs_write+0xe9/0x3d0
[  556.240482][ T3839]  ? __count_memcg_events+0x4e/0xb0
[  556.246172][ T3839]  ? handle_mm_fault+0x9d/0x370
[  556.251473][ T3839]  ksys_write+0x6b/0xf0
[  556.255992][ T3839]  do_syscall_64+0x3f/0xa0
[  556.260805][ T3839]  entry_SYSCALL_64_after_hwframe+0x78/0xe2
[  556.267276][ T3839] RIP: 0033:0x7fdba44efba0
[  556.272087][ T3839] Code: 73 01 c3 48 8b 0d d0 72 2d 00 f7 d8 64 89 01 48 83 c8 ff c3 66 0f 1f 44 00 00 83 3d 1d d4 2d 00 00 75 10 b8 01 00 00 00 0f 05 <48> 3d 01 f0 ff ff 73 31 c3 48 83 ec 08 e8 7e cc 01 00 48 89 04 24
[  556.293977][ T3839] RSP: 002b:00007ffdbc7072f8 EFLAGS: 00000246 ORIG_RAX: 0000000000000001
[  556.303280][ T3839] RAX: ffffffffffffffda RBX: 0000000000000002 RCX: 00007fdba44efba0
[  556.312093][ T3839] RDX: 0000000000000002 RSI: 00007fdba53b9000 RDI: 0000000000000001
[  556.320904][ T3839] RBP: 00007fdba53b9000 R08: 000000000000000a R09: 00007fdba53b1740
[  556.329717][ T3839] R10: 00007fdba53b1740 R11: 0000000000000246 R12: 00007fdba47c8400
[  556.338526][ T3839] R13: 0000000000000002 R14: 0000000000000001 R15: 0000000000000000
[  556.347338][ T3839]  </TASK>
[  556.350643][ T3839] Kernel Offset: disabled
[  556.369810][ T3839] pstore: backend (erst) writing error (-28)
```

## 前端

pstore 主要有 4 个前端：

 前端       Kconfig                  作用                                           重启后常见文件
━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━
 dmesg      CONFIG_PSTORE            保存 Oops/Panic 的内核日志                     dmesg-<backend>-*
─────────  ───────────────────────  ─────────────────────────────────────────────  ─────────────────────
 console    CONFIG_PSTORE_CONSOLE    持续保存 console/printk 输出                   console-<backend>-*
─────────  ───────────────────────  ─────────────────────────────────────────────  ─────────────────────
 ftrace     CONFIG_PSTORE_FTRACE     持久化函数调用轨迹                             ftrace-<backend>-*
─────────  ───────────────────────  ─────────────────────────────────────────────  ─────────────────────
 pmsg       CONFIG_PSTORE_PMSG       提供 /dev/pmsg0，允许用户空间主动写持久日志    pmsg-<backend>-*


1. ftrace 通过来制作

  echo 1 > /sys/kernel/debug/pstore/record_ftrace

2. pmsg 的使用:
```bash
# 如果配置了 pmsg，可以写入用户态消息
# 需加载 pstore_pmsg 模块
lsmod | grep pmsg

# 检查 pmsg 设备
cat /sys/fs/pstore/pmsg-ramoops-0 2>/dev/null

# 通过 /dev/pmsg0 写入（如果存在）
# echo "test message" | sudo tee /dev/pmsg0 2>/dev/null || echo "pmsg not available"
```


## 后端
### zone
### mtd

尝试过，但是失败了
```txt
arg_pstore=""
function setup_pstore() {
	local pstore_bin=$vm_dir/pstore.bin
	if [[ ! -f $pstore_bin ]]; then
		dd if=/dev/zero of="$pstore_bin" bs=1M count=1
	fi
	# 这个是不行的
	arg_pstore=" -drive if=pflash,format=raw,file=$pstore_bin,id=pstore-flash"
	arg_pstore+=" -device mtd,drive=pstore-flash"
}
```

### erst 后端

   数据通路是这样的：

   ```
     kernel panic → pstore → erst 驱动 → ACPI ERST 方法（固件执行）→ 主板 NVRAM flash
   ```

同时可以观察到:
/sys/firmware/acpi/tables/ERST

### efi

检查
/sys/firmware/efi/efivars/dump-*

### blk

pstore-blk 验证用的哪一个
```bash
# 检查是否配置了 pstore-blk 后端
grep CONFIG_PSTORE_BLK /boot/config-$(uname -r)

# 查看块设备后端配置
cat /sys/module/pstore_blk/parameters/blkdev 2>/dev/null

# 检查是否有专用的 pstore 分区
lsblk | grep -i pstore
blkid | grep -i pstore
```


似乎 virtio-blk 不能支持

```txt
=============================
[ BUG: Invalid wait context ]
7.1.2-00001-gfb512e2a3eed #34 Not tainted
-----------------------------
tee/4947 is trying to lock:
ffff88863fffa8e8 (&zone->lock){..-.}-{3:3}, at: get_page_from_freelist+0x616/0x1980
other info that might help us debug this:
context-{5:5}
3 locks held by tee/4947:
 #0: ffff888106087410 (sb_writers#4){.+.+}-{0:0}, at: ksys_write+0x7b/0xf0
 #1: ffffffff82b5f080 (rcu_read_lock){....}-{1:3}, at: kmsg_dump_desc+0x45/0x190
 #2: ffffffffc1027278 (&psinfo->buf_lock){....}-{2:2}, at: pstore_dump+0x249/0x390
stack backtrace:
CPU: 5 UID: 0 PID: 4947 Comm: tee Not tainted 7.1.2-00001-gfb512e2a3eed #34 PREEMPT(full)
Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.17.0-4-g0026c353eb4e 04/01/2014
Call Trace:
 <TASK>
 dump_stack_lvl+0x75/0xb0
 __lock_acquire+0xf11/0x2170
 ? lock_release+0x278/0x470
 ? lock_release+0x278/0x470
 lock_acquire+0xc0/0x2f0
 ? get_page_from_freelist+0x616/0x1980
 ? arch_stack_walk+0xb7/0x100
 ? number+0x4a9/0x5a0
 _raw_spin_lock_irqsave+0x45/0xa0
 ? get_page_from_freelist+0x616/0x1980
 get_page_from_freelist+0x616/0x1980
 __alloc_frozen_pages_noprof+0x19b/0x3b0
 ? psz_pstore_write+0x9c/0x340 [pstore_zone]
 alloc_pages_mpol+0x4c/0x100
 ___kmalloc_large_node+0xbf/0xe0
 __kmalloc_large_node_noprof+0x25/0x130
 __kmalloc_noprof+0x4e3/0x7d0
 ? pstore_compress+0x90/0xd0
 psz_pstore_write+0x9c/0x340 [pstore_zone]
 pstore_dump+0x105/0x390
 kmsg_dump_desc+0xa2/0x190
 vpanic+0x2c6/0x420
 panic+0x6b/0x70
 sysrq_handle_crash+0x1e/0x20
 __handle_sysrq.cold+0xa0/0xe3
 write_sysrq_trigger+0x71/0xa0
 proc_reg_write+0x5d/0xb0
 vfs_write+0xd6/0x5c0
 ksys_write+0x7b/0xf0
 do_syscall_64+0xab/0x6d0
 ? irq_exit_rcu+0x12/0x20
 entry_SYSCALL_64_after_hwframe+0x76/0x7e
RIP: 0033:0x7f915d2af73e
Code: 4d 89 d8 e8 d4 bc 00 00 4c 8b 5d f8 41 8b 93 08 03 00 00 59 5e 48 83 f8 fc 74 11 c9 c3 0f 1f 80 00 00 00 00 48 8b 45 10 0f 05 <c9> c3 83 e2 39 83 fa 08 75 e7 e8 13 ff ff ff 0f 1f 00 f3 0f 1e fa
RSP: 002b:00007fffa3069bb0 EFLAGS: 00000202 ORIG_RAX: 0000000000000001
RAX: ffffffffffffffda RBX: 0000000000000002 RCX: 00007f915d2af73e
RDX: 0000000000000002 RSI: 00007fffa3069d60 RDI: 0000000000000003
RBP: 00007fffa3069bc0 R08: 0000000000000000 R09: 0000000000000000
R10: 0000000000000000 R11: 0000000000000202 R12: 0000000000000002
R13: 00007fffa3069d60 R14: 00005599158072c0 R15: 00005599158072c0
 </TASK>
Rebooting in 10 seconds..
```

## 历史变化

```text
2007–2008 ─ Google 开发 RAM console
     │       主线导入：adc567e8a9c2（v2.6.29）
     │
2010 ─ 添加独立 ramoops
     │       56d611a04fb2（v2.6.35）
     │
2010–2011 ─ Tony Luck/Intel 创建并向主线合入 pstore 框架
     │       ca01d6dd2d7a（v2.6.39）
     │       • 引入统一的 struct pstore_info backend 接口
     │       • 通过文件系统导出平台持久化记录
     │
2012 ─ ramoops 接入 pstore
     │       9ba80d99c86f、1894a253db97（v3.5）
     │       • 同年加入 ECC
     │
2012 ─ 添加 persistent ftrace
     │       060287b8c467、a694d1b5916a（v3.6）
     │
2013 ─ 添加 /sys/fs/pstore 和通用压缩
     │       fb0af3f2b1b6（v3.9）
     │       b0aad7a99c1d（v3.12）
     │
2015 ─ 添加 pmsg
     │       9d5438f462ab（v4.0）
     │
2018 ─ 添加 zstd 压缩
     │       1021bcf44d0e（v4.19）
     │
2020 ─ 添加 storage-zone 公共层和 pstore/blk
     │        d26c3321fe18、17639f67c1d6（v5.8）
     │
2023 ─ 将 zstd 替换为 zlib
             438b805003a（v6.6）
```

| 事件 | Commit | 首次正式版本 | 说明 |
| --- | --- | --- | --- |
| Google RAM console | `adc567e8a9c25f08e91eb18b83bdaa5ff9705919` `Staging: android: add ram_console driver` | v2.6.29 | 源码版权为 Google 2007–2008；主线导入发生在 2008/2009 年 |
| 独立 ramoops 驱动 | `56d611a04fb2db77334e06274de4daed92e2c626` `char drivers: RAM oops/panic logger` | v2.6.35 | ramoops 在 2010 年已经进入主线，并非 2012 年首次添加 |
| pstore 核心框架 | `ca01d6dd2d7a2652000307520777538740efc286` `pstore: new filesystem interface to platform persistent storage` | v2.6.39 | Tony Luck/Intel 贡献；同时引入 `struct pstore_info` |
| pstore 合入 Linus 主线 | `6d1e9a42e7176bbce9348274784b2e5f69223936` | v2.6.39 | 2011 年 3 月进入 Linus 主线 |
| ramoops 改用 pstore | `9ba80d99c86f1b76df891afdf39b44df38bbd35b` `ramoops: use pstore interface` | v3.5 | 这是 ramoops 在 2012 年的关键里程碑 |
| ramoops 移入 `fs/pstore` | `1894a253db97059bc299b834b76f665bc6586b1d` `ramoops: Move to fs/pstore/ram.c` | v3.5 | 正式确立 ramoops 的 pstore RAM backend 定位 |
| ramoops ECC | `39eb7e9791866973dbb7a3a6d2061d70356c7d90` `pstore/ram: Add ECC support` | v3.5 | 为 persistent RAM zone 增加 ECC 支持 |
| persistent ftrace 核心 | `060287b8c467bf49a594d8d669e1986c6d8d76b0` `pstore: Add persistent function tracing` | v3.6 | 首次支持持久化 function tracing，发生在 2012 年而非 2016 年 |
| ramoops ftrace backend | `a694d1b5916a486ce25fb5f2b39f2627f7afd5f3` `pstore/ram: Add ftrace messages handling` | v3.6 | 与 persistent ftrace 核心提交配套 |
| `/sys/fs/pstore` 挂载点 | `fb0af3f2b1b613e5ea75426d454c7e5b1d1eef49` `pstore: Create a convenient mount point for pstore` | v3.9 | 初始 pstore 框架使用 `/dev/pstore`；该提交增加现代挂载点 |
| 通用压缩支持 | `b0aad7a99c1df90c23ff4bac76eea9cf25049e9e` `pstore: Add compression support to pstore` | v3.12 | 2013 年加入，最初使用 zlib |
| pmsg | `9d5438f462abd6398cdb7b3211bdcec271873a3b` `pstore: Add pmsg - user-space accessible pstore object` | v4.0 | 通用 pmsg 支持在 2015 年加入，而非 2020 年 |
| zstd 压缩 | `1021bcf44d0e876b10f8739594ad7e6e9c746026` `pstore: add zstd compression support` | v4.19 | 2018 年加入 |
| storage zone 公共层 | `d26c3321fe18dc74517dc1f518d584aa33b0a851` `pstore/zone: Introduce common layer to manage storage zones` | v5.8 | 为 pstore/blk 等 backend 提供分区或 zone 管理接口 |
| block device backend | `17639f67c1d61aba3c05e7703f75cd468f9d484f` `pstore/blk: Introduce backend for block devices` | v5.8 | 2020 年进入主线；主线历史中没有对应的 2014 年提交 |
| pstore/blk 支持 pmsg | `0dc068265a1c5923ffebf40388fbe93050a77ad1` `pstore/zone,blk: Add support for pmsg frontend` | v5.8 | 如果“2020 添加 pmsg”特指 block backend，则对应此提交 |

## systemd-pstore
系统启动后将 pstore 中的崩溃信息持久化到磁盘。
避免 pstore 空间被占满导致新崩溃无法记录

开机会自动将日志移动到
/var/lib/systemd/pstore/ 中开机就会放到这里来

参考 https://man7.org/linux/man-pages/man8/systemd-pstore.service.8.html

## 如果想要记录日志

- 触发 panic
- pmsg
	- echo xxx | sudo tee /dev/pmsg0 (ramoops/pstore_blk 支持，erst 不支持）
- console
	- 内核开了 CONFIG_PSTORE_CONSOLE 后自动持续记录，无需任何操作
- 内核中触发 oops/WARN
	- 内核 WARN 会写一条 dmesg 记录（不致命）

这个不会立刻回显出来，然后在 /var/lib/systemd/pstore/ 中

```txt
sudo cat pmsg-ramoops-0
```
就可以看到 xxx

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
