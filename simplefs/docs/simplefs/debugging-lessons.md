# xfstests 调试方法与案例

## 先保存可重复 workload

随机 fsx 失败时，第一目标不是立即修改代码，而是保存：

- seed、完整命令行和操作数；
- `.fsxops` 操作序列和 `.fsxgood` 期望内容；
- 失败文件镜像；
- `dmesg`、extent dump 和必要的 dynamic debug 日志。

然后用固定 seed 缩小第一个失败操作，比较 `N-1` 和 `N` 的文件内容、extent 与 `filefrag` 输出。标准 xfstests 随机重跑只能证明概率变化，固定 workload 才能证明某个根因被修复。

## 案例一：过早转换 unwritten extent

现象是 buffered write 结束后的部分块出现旧数据。根因是 write iomap 建立阶段已经把预分配 extent 转为 written，iomap 因而看不到 `IOMAP_UNWRITTEN`，不会对未覆盖的块尾执行正确的零化语义。

修复是保持 unwritten 状态直到数据 bio 成功完成，再在 worker 中只转换实际写入范围。验证包括 `generic/092`、`generic/363` 的固定操作回放和 100000-op fsx。

## 案例二：`IOMAP_F_NEW` 跨越旧块

固定 seed 在 `copy_file_range` 后把目标末尾的旧数据变成零。日志显示一次 hole 分配得到 1 个新块，extent normalization 又把它和后面的 6 个旧 written 块合并；映射函数错误地返回了合并后的 7 块，并对全部范围设置 `IOMAP_F_NEW`。iomap 合理地把 partial final block 当成新块清零，于是破坏了旧数据。

不变量是：映射返回长度和 extent 的最终长度不是一回事。新分配路径必须保存 `allocated_len`，精确增长路径必须返回 `grow_len`，`IOMAP_F_NEW` 只覆盖当前调用真正获得所有权的块。

## 案例三：元数据 clean folio 复活

块已经在位图中释放并被其他对象复用，但 bdev mapping 中仍可能留有 clean 元数据 folio。之后再次读取该物理块时，缓存命中会得到旧结构，而不是磁盘上的新内容。这类问题表面可能表现为随机 extent 损坏、目录异常或并发 fsx 失败。

解决方法是在元数据块归还位图前，对 bdev mapping 执行 write-and-wait 与 invalidate。审计时要覆盖正常删除、错误回滚、extent leaf 缩减、xattr 和 inode 初始化失败等所有释放路径。

## 案例四：collapse range 的双重所有权

collapse range 同时改变逻辑页偏移和 extent 树。旧页缓存必须先写回并从移动起点开始失效。重建 extent buffer 时，还要把旧 buffer 的 leaf block 所有权转给新 buffer，让同步代码原位复用或释放多余 leaf。遗漏前者会读到按旧偏移缓存的数据，遗漏后者会持续泄漏文件系统块。

## 案例五：空闲 inode 的旧磁盘内容和 VFS union

inode 位图中的“空闲”不表示 inode table 槽已经清零。通过 `iget` 取得一个复用 inode 时，它可能先读到上一任的 mode、nlink、extent root 和 xattr block。更危险的是，读取旧 symlink 会设置 `inode->i_link`，它与 `i_cdev`、`i_pipe` 共用 VFS inode 中的 union。如果同一个新 VFS inode 随后被改造为字符设备，旧 `i_link` 就会被 `chrdev_open()` 当作 `i_cdev`，最终在 `try_module_get()` 崩溃。

安全做法是新建路径用 `new_inode()` 获得完全初始化的 VFS inode，设置 inode 号后用 `insert_inode_locked()` 加入 inode hash；只有查找已存在 inode 才走 `iget`和磁盘解码。失败时对 I_NEW inode 使用 `discard_new_inode()`，由 eviction 统一释放本次已经建立的资源和 inode 位。ACL、security xattr、symlink、mknod 等分支也必须传播初始化错误。

## 案例六：文件名长度不是 C 字符串长度

目录项磁盘字段为固定 255 字节，合法名字也可以正好占满 255 字节，此时没有空间存储结尾 NUL。使用 `strncpy(..., 255)` 后再强制写 NUL 会截掉最后一个字节；对磁盘字段调用无界 `strlen` 还可能越界读取。

目录项现在按 `qstr.len` 复制，覆盖旧目录项时先清空字段，读取时使用上限为字段宽度的 `strnlen`。create、rename、symlink 和 mknod 的共用插入路径都执行相同的长度校验。

## 案例七：exportfs 的断开目录别名

文件句柄解码已经能找到正确 inode，`get_parent` 和 `get_name` 也返回正确值，但目录句柄仍然报 `ESTALE`。根因是普通 lookup 使用 `d_add()` 新建了另一个 dentry，exportfs 持有的 disconnected directory alias 始终没有接回父目录。

目录文件系统的 lookup 应使用 `d_splice_alias()`，让 VFS 复用已有目录别名并建立父子关系。generation 只解决 inode 号复用的 stale handle，不能替代 dentry 重连语义。

## 案例八：DIO fallback 不能调用错误的 buffered 接口

`iomap_dio_rw()` 返回 `-ENOTBLK` 并不一定是最终错误；它是在通知文件系统：DIO 与页缓存发生竞争，应从尚未消费的 iov 位置改走 buffered I/O。SimpleFS 最初没有处理该返回值，导致 `generic/095` 失败。第一次修复又误用了 `generic_perform_write()`，但纯 iomap address space 没有 `write_begin`，因而在 NULL `a_ops->write_begin` 上崩溃。

正确的 fallback 必须继续调用 SimpleFS 自己的 `iomap_file_buffered_write()` 包装，随后写回并丢弃 fallback 范围的页缓存，维持 O_DIRECT 对调用者承诺的可见性。选择 fallback helper 时不能只看函数名；还要核对 address-space 模型是 buffer_head、`write_begin` 还是纯 iomap。

## 案例九：invalidate 锁内不能 fault-in 同一 mapping

`generic/647` 的用户缓冲区本身来自目标文件的 mmap。SimpleFS 在 buffered write 外层先取得 `mapping.invalidate_lock` 写锁，iomap 再访问用户缓冲区时触发 page fault；readahead 对同一个 mapping 申请 invalidate 读锁，于是同一进程递归死锁。lockdep 明确显示写锁持有点在 `simplefs_file_write_iter()`，读锁等待点在 `do_page_cache_ra()`。

普通 buffered write 不需要在整个 iomap 写过程外持有 invalidate 写锁。删掉这层锁后，inode 写锁仍然串行化文件写入，真正会移动映射或丢弃页缓存的 fallocate/truncate 路径继续独立持有 invalidate 锁。`generic/647` 和增加 O_DIRECT mmap 自引用写的 `generic/729` 都必须通过，才能证明两条路径没有同类锁顺序问题。

## 案例十：journal 尾区必须同时反映在计数和位图

mkfs 曾从 `nr_free_blocks` 中扣除了 journal 尾区，却没有在 free-block bitmap 中把这些块标成已占用。挂载后的分配器因此仍可分配 journal 块，释放或重建计数时还会让无符号空闲块计数下溢，最终 `df` 显示接近 16 TiB 的虚假可用空间，`generic/275` 失败。

磁盘布局的保留区必须满足同一个事实在三处一致：布局边界合法、位图为 allocated、空闲计数不包含它。mkfs 现在标记整个 journal 尾区；挂载时校验起点、清理旧镜像中的错误 free bits，并根据实际位图重新协调计数。计数不是所有权来源，位图才是。

## 案例十一：元数据分配顺序也会改变 fiemap 语义

新文件第一次写 64 KiB 时，旧代码先分配第一个数据块，随后为了保存 extent 再分配 leaf，最后分配余下 15 个数据块。逻辑上虽然是一个连续 extent，物理上却被 leaf 插成 `1 + 15` 两段，`generic/473` 的 fiemap 连续性检查失败。

当空 extent tree 第一次需要 leaf 时，现在一次预留 `[leaf][data run]`：首块归 leaf，紧随其后的连续块归数据。同步成功才提交 leaf 所有权；任何错误回滚都分别退役元数据块并归还数据块。这里的教训是，内部元数据的分配时机不仅影响空间效率，也会成为用户可见的物理布局。

## 案例十二：容量常量和 statfs 含义必须来自磁盘格式

目录 extent 固定覆盖 8 个块，但目录最大条目数曾错误复用普通文件单 extent 的 1024 块上限。结果满目录测试会继续尝试创建数百万条目，而真实目录格式最多只能容纳 40,920 条。修正后使用独立的 `SIMPLEFS_DIR_BLOCKS_PER_EXTENT`，达到格式上限立即返回 `EMLINK`，`generic/558` 的运行时间也显著下降。

同一轮还发现 `statfs.f_files` 填成了已用 inode 数。该字段应报告文件系统可管理的 inode 总数，已用/空闲关系由 `f_ffree` 表达。测试很慢或 `df` 数字怪异时，先检查是否把“容量”“已用量”“单次分配长度”三种含义混在了同一常量或字段中。

## 案例十三：`i_blocks` 的单位固定为 512 字节

`generic/701` 创建 5 GiB 文件再截到 4 GiB，SimpleFS 报告约 1,048,581 个 block，而期望约 8,388,608。两者恰好相差 8 倍：旧代码直接把 4 KiB 文件系统块数写进 `inode->i_blocks`，但 VFS 的 `i_blocks` 和用户态 `stat.st_blocks` 无论文件系统块大小是多少，都以 512 字节扇区为单位。

磁盘 inode 仍保存 SimpleFS 块数，读 inode 时转换成 512B 扇区，写 inode 时转换回磁盘块。数据 extent、extent leaf、目录 extent、长 symlink 和 xattr block 的所有增减点也必须使用同一转换 helper。只修 `stat` 输出会掩盖内存计数和持久化计数不一致；正确边界是“磁盘格式单位”和“VFS ABI 单位”的双向转换。

## 案例十四：测试 runner 自己也会制造 mount 泄漏

runner 为隔离临时文件，把工作目录 bind mount 到 `/tmp`。测试 VM 的 `/tmp` 原本位于 shared propagation 域；直接替换 shared target 会把新 mount 传播到 peer。重复执行后 mount 栈近似指数增长，最终出现 8192 个 `/tmp` 和 8191 个工作目录挂载，任何扫描 `/proc/*/mountinfo` 的工具都变得极慢，看起来像文件系统 hang。

setup 现在先确保 `/tmp` 是独立 mount，再执行 `mount --make-private /tmp`，bind 替换后再次设为 private；cleanup 只在 FSROOT 确认是本 runner 的工作目录时卸载。连续运行前后都要记录 mountinfo 行数。测试基础设施的资源泄漏会污染性能、超时和 hang 判断，不能把它归因给被测模块。

## 案例十五：DIO、mmap fault 与 GUP 的三方锁序

仅删除 buffered write 外层的 invalidate lock 还不够。`generic/095` 中 mmap 脏页可以在异步 DIO 的前后失效之间重新进入 page cache，iomap 会给 mapping 的 `wb_err` 记入 `-EIO`，随后无关的 `msync` 随机失败。把 DIO 改成同步并持有 invalidate 写锁能关闭这个窗口，却又产生两种递归：用户源 iov 来自同一文件 mmap 时，fault/readahead 申请 invalidate 读锁；普通用户 iov 的 GUP slow fallback 还可能在 invalidate 锁内申请 `mmap_lock`，与 filemap fault 的反向顺序形成 lockdep 环。

最终做法是在取得 invalidate 锁之前，用 `iov_iter_extract_bvecs()` 分批 pin 用户页；锁内交给 iomap 的是 BVEC iterator，不再访问用户虚拟地址或取得 `mmap_lock`。每批强制同步完成后 unpin。若源页就是目标文件的 mmap，pin 会使 iomap 的 pre-DIO invalidation 失败，它按协议返回 buffered fallback。这样既保留 `generic/252` 在 dm-error 下必须执行真实 DIO 的错误传播，也让 `095/647/729` 不再出现页缓存 EIO 或锁递归。

## 案例十六：ENOSPC 下先 free 再改映射会产生双重所有权

`generic/300` 并发执行 AIO/DIO、fallocate 和 punch，偶发发现 verifier 块的 magic 变成零或其他数据。punch、truncate、collapse 原先先把数据块 `put_blocks()` 到全局位图，再调用 `simplefs_file_sync_extents()`。当 extent split 需要新增 leaf、但 ENOSPC 使 leaf 分配失败时，旧 extent tree 仍映射这些块，另一 inode 却已经可以重新分配它们。extent leaf 缩减也曾先退役旧 leaf，再更新 root 指针，存在同样问题。

正确提交顺序是：先在内存中构造新 extent 集合，写回新 leaf，再写回指向它们的 root；root 已经持久地移除旧 leaf/数据映射后，才能把旧块归还位图。待释放数据范围要单独记录，extent 同步失败时一个也不能释放。fallocate/truncate 还要先 `inode_dio_wait()`，新写入洞保持 UNWRITTEN 到数据 I/O 成功完成。这个策略在错误时宁可安全泄漏，也不能让两个 inode 同时拥有同一物理块。

## 判断 hang 的方法

短超时不是 hang 证据。以 `generic/074` 为例，多个 `fstest` 子进程可能长时间处于 `R` 状态并占满 CPU，这是用户态 workload 正常运行。排查顺序是：

1. 查看进程树、状态、CPU 和 elapsed time；
2. 检查 `dmesg` 的 warning、hung task、I/O error；
3. 必要时用 gdb 查看用户态栈和循环参数；
4. 只有状态停止变化、内核栈或日志支持时才判断为 hang。

测试驱动应给慢用例足够超时，并在 `FAIL` 或 `TIMEOUT` 时返回非零。外部 TERM、中断或 VM reset 后产生的汇总不能算作 PASS。

### SSH 不响应时保留 panic 现场

SSH 和 QEMU guest agent 同时不响应时，不要立即 reset。宿主机侧仍可以：

1. 通过 HMP `info status`/`info cpus` 确认 VM 状态和 vCPU 活动；
2. 用 `dump-guest-memory -z -R <file>.out` 保存内存，并等待文件大小停止增长后再分析；
3. 为 QEMU 开启临时 gdbserver，用匹配的 `vmlinux` 执行 `thread apply all bt`；
4. 加载内核 GDB helper 后执行 `lx-dmesg`，提取来不及通过 SSH 读取的 Oops。

`generic/011` 的现场中，7 个 vCPU 已进入 `stop_this_cpu()`，剩余 vCPU 在 `panic()` 的 delay loop；`lx-dmesg` 才显示真正的 `try_module_get -> chrdev_open` 页故障。因此“QEMU 占用 CPU、guest 无响应”可能是 panic 后状态，不是 buffered write 死锁。

## 每个修复的证据层次

从强到弱建议保留：

1. 精确失败 workload 在修复前稳定失败、修复后稳定通过；
2. 对应 xfstests 多次通过；
3. 相关并发或邻接用例通过；
4. 模块构建、shell 静态检查和 `git diff --check` 通过；
5. VM `dmesg` 没有新增 warning、BUG、I/O error。

最终仍要跑完整 generic 集合。局部用例通过只说明已覆盖的语义，不能替代全量回归。

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
