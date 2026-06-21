# 基本的使用

### loop device
```sh
modprobe ublk_drv
./ublk add -t loop -f /dev/nvme0n1
```

ublk 的线程的状态:
```txt
   - 20.46% do_syscall_64
      - 20.39% __do_sys_io_uring_enter
         - 12.79% io_cqring_wait
            - 12.17% io_run_task_work
               - 12.14% task_work_run
                  - 12.11% tctx_task_work
                     - tctx_task_work_run
                        - 12.07% io_handle_tw_list
                           - 11.95% ublk_rq_task_work_cb
                              - 11.86% ublk_copy_user_pages
                                 - 0.79% iov_iter_get_pages2
                                    - __iov_iter_get_pages_alloc
                                       - 0.78% get_user_pages_fast
                                          - 0.78% internal_get_user_pages_fast
                                               0.51% try_grab_folio
            - 0.56% schedule_hrtimeout_range_clock
               - 0.51% schedule
                    __schedule
         - 7.53% io_submit_sqes
            - 7.42% io_issue_sqe
               - 5.96% io_uring_cmd
                  - 5.94% ublk_ch_uring_cmd
                     - __ublk_ch_uring_cmd
                        - 5.53% blk_update_request
                           - 4.40% end_swap_bio_write
                              - 3.77% folio_end_writeback
                                 - 2.41% folio_rotate_reclaimable
                                    - 1.96% folio_batch_move_lru
                                       - 1.30% lru_move_tail_fn
                                            0.57% lru_gen_add_folio
                                 - 1.00% __folio_end_writeback
                                      0.52% lruvec_stat_mod_folio
                                0.61% __end_swap_bio_write
                             0.68% kmem_cache_free
               - 1.34% io_write
                  - 1.31% blkdev_write_iter
                     - 1.26% blkdev_direct_IO.part.0
                        - 0.68% bio_iov_iter_get_pages
                           - 0.56% iov_iter_extract_pages
                              - 0.55% pin_user_pages_fast
                                   0.55% internal_get_user_pages_fast
```
看上去很合理。

# ublk

https://lwn.net/Articles/903855/


```txt
- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - ksys_mmap_pgoff
        - vm_mmap_pgoff
          - do_mmap
            - mmap_region
              - call_mmap
                - ublk_ch_mmap
```

```txt
- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - __do_sys_io_uring_enter
        - io_submit_sqes
          - io_submit_sqe
            - io_queue_sqe
              - io_issue_sqe
                - io_uring_cmd
                  - ublk_ch_uring_cmd
```

```txt
- ret_from_fork
  - kthread
    - worker_thread
      - process_one_work
        - __blk_mq_run_hw_queue
          - blk_mq_sched_dispatch_requests
            - __blk_mq_sched_dispatch_requests
              - blk_mq_do_dispatch_sched
                - __blk_mq_do_dispatch_sched
                  - blk_mq_dispatch_rq_list
                    - ublk_queue_rq
```

```txt
- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - __do_sys_io_uring_enter
        - io_cqring_wait
          - io_cqring_wait_schedule
            - io_run_task_work_sig
              - io_run_task_work_ctx
                - io_run_task_work
                  - io_run_task_work
                    - task_work_run
                      - ublk_rq_task_work_fn
```

```txt
- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - __do_sys_io_uring_enter
        - io_submit_sqes
          - io_submit_sqe
            - io_queue_sqe
              - io_issue_sqe
                - io_uring_cmd
                  - ublk_ch_uring_cmd
                    - ublk_commit_completion
                      - ublk_complete_rq
                        - ublk_unmap_io
                          - ublk_copy_user_pages
```

```txt
- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - __do_sys_io_uring_enter
        - io_cqring_wait
          - io_cqring_wait_schedule
            - io_run_task_work_sig
              - io_run_task_work_ctx
                - io_run_task_work
                  - io_run_task_work
                    - task_work_run
                      - ublk_rq_task_work_fn
```

## 运行时间
```txt
- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - __do_sys_io_uring_enter
        - io_submit_sqes
          - io_submit_sqe
            - io_queue_sqe
              - io_issue_sqe
                - io_read
                  - io_iter_do_read
                    - call_read_iter
                      - ext4_file_read_iter
                        - ext4_dio_read_iter
                          - iomap_dio_rw
                            - __iomap_dio_rw
                              - iomap_iter
                                - ext4_iomap_begin
                                  - ext4_map_blocks
                                    - ext4_es_lookup_extent
                                      - percpu_counter_inc
                                        - percpu_counter_add
                                          - percpu_counter_add_batch
                                            - _raw_spin_lock_irqsave
                                              - __raw_spin_lock_irqsave
                                                - do_raw_spin_lock
                                                  - queued_spin_lock
                                                    - atomic_try_cmpxchg_acquire
                                                      - arch_atomic_try_cmpxchg
```

```txt
- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - __x64_sys_io_submit
        - __se_sys_io_submit
          - __do_sys_io_submit
            - io_submit_one
              - __io_submit_one
                - aio_write
                  - call_write_iter
                    - blkdev_write_iter
                      - __generic_file_write_iter
                        - generic_file_direct_write
                          - __blkdev_direct_IO_async
                            - bio_alloc_bioset
                              - mempool_alloc
                                - kmem_cache_alloc
                                  - __kmem_cache_alloc_lru
                                    - slab_alloc
                                      - slab_alloc_node
                                        - __slab_alloc
                                          - ___slab_alloc
                                            - new_slab
```
- [ ] 为什么这里存在一个 mempool_alloc 的?

```txt
- do_syscall_64
  - do_syscall_x64
    - __x64_sys_io_submit
      - __se_sys_io_submit
        - __do_sys_io_submit
          - io_submit_one
            - __io_submit_one
              - aio_read
                - call_read_iter
                  - blkdev_read_iter
                    - blkdev_direct_IO
                      - blkdev_direct_IO
                        - __blkdev_direct_IO_async
                          - bio_iov_iter_get_pages
                            - __bio_iov_iter_get_pages
                              - iov_iter_get_pages2
                                - __iov_iter_get_pages_alloc
                                  - get_user_pages_fast
                                    - internal_get_user_pages_fast
                                      - lockless_pages_from_mm
                                        - gup_pgd_range
                                          - gup_p4d_range
                                            - gup_pud_range
                                              - gup_pmd_range
```
- 这里是调用到 gup，似乎是所有的读写都会发送给 userspace 的位置，然后 userspace 通过 iouring 将数据发送到内核中。


好吧，大致是理解了，但是需要更加深入的理解一下 io uring


## 原始的讨论看看
https://lore.kernel.org/io-uring/YoKmFYjIe1AWk%2FP8@stefanha-x1.localdomain/


https://lwn.net/Articles/900690/
https://lwn.net/Articles/903855/


## 思考
提供了一些 qcow2 的实现，将 qcow2 直接放到内核态中，

## 的确看上去是有可能的

有趣，相当有启发的讨论:

- https://news.ycombinator.com/item?id=32508659

## nbd
https://libguestfs.org/nbdublk.1.html

https://rwmj.wordpress.com/tag/network-block-device/

## [ ] zero copy

我理解 zero copy 应该是可以容易实现的。

可以把这个 page 直接发送给 server 吧，相当于这个 page 同时被映射到两个空间中。

https://lore.kernel.org/lkml/afed0772-3626-44e6-a33c-7134a7d623f0@linux.alibaba.com/T/#ma8e12b1c5d58c40435d3acb963005262bb9bc009

> zero copy depends on user copy

所以，看来 ublk 也是只能做到如此了。

- https://github.com/spdk/spdk/issues/3444
- https://lwn.net/Articles/926118/

### 只要 ublk 可以 zero copy ，那么 uswap 一定可以 ?

关键是 swap in 的实现，发送命令中提供一个 page ，
server 需要 map 这个 page 才可以向其中填充内容。


## 忽然发现已经可以看懂这些讨论了
https://lore.kernel.org/linux-block/20220518063808.GA168577@storage2.sh.intel.com/

## [ ] 为什么后来增加了这么多的代码?

v2 的时候只有这一点:
```txt
 drivers/block/ubd_drv.c      | 1444 ++++++++++++++++++++++++++++++++++
```

从 1000 多行增加到 3000 行

## 为什么不去直接复用这里的代码
https://lore.kernel.org/linux-block/bdff8d00-c936-72df-cac1-3c1d3131339f@easystack.cn/#t

## 看看完整的对话
- 原始讨论: https://lore.kernel.org/linux-block/20220517055358.3164431-1-ming.lei@redhat.com/

Xiaoguang Wang

- v1 : https://lore.kernel.org/io-uring/20220509092312.254354-1-ming.lei@redhat.com/?s=09
- v2 : https://lore.kernel.org/linux-block/20220523145643.GA232396@storage2.sh.intel.com/#r
- v3 : https://lore.kernel.org/lkml/8735fg4jhb.fsf@collabora.com/#r
- v4 : https://lore.kernel.org/lkml/afed0772-3626-44e6-a33c-7134a7d623f0@linux.alibaba.com/T/#r377324cfd01011a246b7c08cdfb5b5df9cdcbfcc

## 原来还有竞品啊
- https://www.kernel.org/doc/Documentation/target/tcmu-design.txt

## ublk 可以指定队列的数量吗?

- 一个队列对应一个 io uring 吗?
- io uring 可以实现多队列吗，或者说，需要实现多 thread 吗?
  - 其实我感觉，只是需要

## ublk 和 vDUSE 有什么区别吗?

## 原来通过 tcmu 也可以实现用户态的 ublk

Documentation/target/tcmu-design.rst

也就是:
https://github.com/containerd/overlaybd/blob/main/docs/README.md
中提到的，也就是其中的内核的代码:

verlaybd 作为 TCMU 的后端存储工作：
- 通过 JSON 配置文件描述镜像层（本地文件、目录或远程 blob，如从镜像仓库拉取）。
- 创建虚拟块设备（e.g., /dev/sdX），下层（只读层）按从下到上的顺序堆叠，上层可选为可写层。
- 启动时，按需从远程仓库（如 Docker Registry）懒加载数据，使用 Zfile 进行可寻址解压。
- I/O 操作通过优化引擎（e.g., libaio）处理，查找使用 B+ 树加速；后台下载可预加载 blob 到本地缓存。
- 可写操作：使用 overlaybd-create 创建上层，写入后通过 overlaybd-commit 提交并压缩为 Zfile。
- 支持 iSCSI 远程访问和内核模块（DADI_kmod）将 overlaybd 文件作为 loop 设备使用。

```txt
┌─────────────────┐    ┌──────────────────┐
│   containerd    │    │     QEMU/KVM     │
│   (overlayfs)   │    │   (virtio-blk)   │
└─────────┬───────┘    └────────┬─────────┘
          │                     │
          └───────────┬─────────┤
                      │         │
          ┌─────────-─▼────────▼─────────┐
          │     TCMU Kernel Module       │
          │    (/dev/sdX block device)   │
          └──────────┬───────────────────┘
                     │
          ┌──────────▼──────────┐
          │  overlaybd Userland │  ←── **用户态 Block Server**
          │   (TCMU Runner)     │
          └────────┬────────────┘
                   │
      ┌────────────┼────────────┐
      │            │            │
  ┌──▼───┐  ┌─────▼────┐  ┌───▼───┐
  │Cache │  │ Zfile    │  │ P2P   │
  │      │  │ Engine   │  │ Agent │
  └──────┘  └──────────┘  └───────┘
      │            │            │
      └────────────┼────────────┘
                   │
            ┌──────▼──────┐
            │Docker Reg   │
            │   / S3      │
            └─────────────┘
```

## 真的可以实现这个吗?

```txt
1. daemon 和 client 都 mmap 同一块共享内存（memfd 或 hugetlbfs，MAP_SHARED），所以它们实际访问的是同一组物理 page。
2. daemon 通过 UBLK_U_CMD_REG_BUF 把这块共享内存注册给内核；内核把里面的 page pin 住，并用 maple tree 建立 PFN -> (buffer_index, offset) 的查找表。
3. 当 client 对 /dev/ublkb* 发 direct I/O 时，内核检查该 IO 的 page 是否命中已注册的 PFN：
    • 命中：在 descriptor 里设置 UBLK_IO_F_SHMEM_ZC，addr 编码成 buffer_index + offset，不再拷贝。
    • 没命中：静默回退到普通 copy 路径。
4. daemon 看到 UBLK_IO_F_SHMEM_ZC 后，直接通过自己的 mmap 访问对应偏移的数据。

它的优点是不需要 per-I/O 的 io_uring fixed buffer 注册，一次注册后自动 zero-copy；缺点是需要 client 配合，且只对 O_DIRECT、连续 buffer 有效。

所以对你们 uswap 来说，这相当于“方案 3（固定 buffer / zero-copy）”的一个具体落地方式，但内核和 Rust 复杂度确实最高。先按推荐的“per-tag mmap + copy”做
MVP，后续再考虑 UBLK_F_SHMEM_ZC 这种路径，是比较稳妥的路线。
```

## zc 如何实现的

要让 UBLK_F_SHMEM_ZC 真正工作，核心只有一句话：

> 客户端 I/O buffer 对应的物理页，必须和 ublk server 已经注册到内核的共享内存页是同一组物理页。

内核并不关心虚拟地址，它只比较 PFN（物理页帧号）。只要 PFN 匹配，就走零拷贝路径；不匹配则静默回退到普通拷贝路径。

实际应用中有两种典型做法：memfd + SCM_RIGHTS 和 hugetlbfs/文件 MAP_SHARED。下面分别说明。

────────────────────────────────────────────────────────────────────────────────

1. 方案一：memfd + SCM_RIGHTS（进程间传 fd）

适合客户端和 ublk server 是两个独立进程、需要动态协商共享内存的场景。

1.1 客户端侧

```c
  /* 1. 创建匿名共享内存 */
  int memfd = memfd_create("ublk_buf", MFD_ALLOW_SEALING);
  ftruncate(memfd, BUF_SIZE);          /* 必须页对齐 */

  /* 2. 以 MAP_SHARED 映射到客户端地址空间 */
  void *client_buf = mmap(NULL, BUF_SIZE,
                          PROT_READ | PROT_WRITE,
                          MAP_SHARED | MAP_POPULATE,
                          memfd, 0);

  /* 3. 把 memfd 通过 unix socket 的 SCM_RIGHTS 传给 ublk server */
  send_fd_over_unix_socket(sock_fd, memfd);

  /* 4. 后续所有针对 /dev/ublkbN 的 I/O 都必须从这个区域分配 buffer
   *    并且必须用 O_DIRECT */
  void *io_buf = client_buf + offset;  /* offset 在 BUF_SIZE 内 */
  preadv2/blk_io(..., io_buf, len, ...);
```

关键点：

• BUF_SIZE 必须 页对齐（PAGE_ALIGNED），起始地址也要页对齐。
• I/O 必须是 direct I/O（O_DIRECT）。走 page cache 的 buffer 是内核新分配的页，PFN 不会匹配。
• 传给 ublk server 的只是 fd，真正的共享靠 MAP_SHARED 实现。

1.2 ublk server 侧

```c
  /* 1. 通过 SCM_RIGHTS 接收客户端发来的 memfd */
  int memfd = recv_fd_from_unix_socket(client_fd);

  /* 2. server 也用 MAP_SHARED 映射同一块物理内存 */
  void *server_buf = mmap(NULL, BUF_SIZE,
                          PROT_READ | PROT_WRITE,
                          MAP_SHARED | MAP_POPULATE,
                          memfd, 0);

  /* 3. 把 server 自己的 VA 范围注册到内核 */
  struct ublk_shmem_buf_reg reg = {
      .addr = (__u64)server_buf,
      .len  = BUF_SIZE,
      .flags = 0,
  };

  struct ublksrv_ctrl_cmd cmd = {
      .dev_id = dev_id,
      .queue_id = -1,
      .addr = (__u64)&reg,
      .len = sizeof(reg),
  };

  int idx = ioctl(ctrl_fd, UBLK_U_CMD_REG_BUF, &cmd);
  /* idx >= 0 就是内核返回的 buffer index，I/O 处理时要用 */
```

内核在 ublk_ctrl_reg_buf() 里会：

1. pin_user_pages_fast() 把这些页 pin 住。
2. 把 PFN 范围插入 ub->buf_tree。
3. 返回 buffer index。

1.3 server 处理 I/O 时

当收到一个 I/O descriptor：

```c
  if (iod->op_flags & UBLK_IO_F_SHMEM_ZC) {
      __u32 idx = ublk_shmem_zc_index(iod->addr);   /* buffer index */
      __u32 off = ublk_shmem_zc_offset(iod->addr);  /* 字节偏移 */
      void *data = shmem_table[idx].mmap_base + off;
      /* data 就是和客户端共享的同一块内存，直接读/写即可 */
  }
```

────────────────────────────────────────────────────────────────────────────────

2. 方案二：hugetlbfs / 普通文件 MAP_SHARED

适合双方都能访问同一个文件路径的场景，不需要 SCM_RIGHTS 传 fd。内核 selftest 里用的就是这种方式。

2.1 准备共享文件

```bash
  # 分配大页并挂载 hugetlbfs
  echo 10 > /proc/sys/vm/nr_hugepages
  mount -t hugetlbfs none /mnt/hugetlb

  # 创建一个共享文件
  fallocate -l 4M /mnt/hugetlb/ublk_buf
```

2.2 ublk server 启动时

server 打开并 mmap 这个文件：

```c
  int fd = open("/mnt/hugetlb/ublk_buf", O_RDWR);
  void *server_buf = mmap(NULL, 4 * 1024 * 1024,
                          PROT_READ | PROT_WRITE,
                          MAP_SHARED | MAP_POPULATE, fd, 0);

  /* 注册给内核 */
  struct ublk_shmem_buf_reg reg = {
      .addr = (__u64)server_buf,
      .len  = 4 * 1024 * 1024,
  };
  ioctl(ctrl_fd, UBLK_U_CMD_REG_BUF, &reg);
```

2.3 客户端侧

用 fio 时可以直接指定从这个 hugetlbfs 文件分配 I/O buffer：

```bash
  fio --filename=/dev/ublkbN \
      --direct=1 \
      --mem=mmaphuge:/mnt/hugetlb/ublk_buf \
      --bs=4k --size=4M --iodepth=32
```

--mem=mmaphuge:<file> 会让 fio 以 MAP_SHARED 映射这个文件，并从这块区域分配 I/O buffer。于是客户端和 server 的 PFN 自然相同。

────────────────────────────────────────────────────────────────────────────────

3. 必须满足的条件

┌──────────────────────────────┬────────────────────────────────────────────────────────────────────────────────────────────────┐
│ 条件                         │ 原因                                                                                           │
├──────────────────────────────┼────────────────────────────────────────────────────────────────────────────────────────────────┤
│ 页对齐                       │ UBLK_U_CMD_REG_BUF 要求 addr 和 len 都 PAGE_ALIGNED。                                          │
├──────────────────────────────┼────────────────────────────────────────────────────────────────────────────────────────────────┤
│ MAP_SHARED                   │ 只有共享映射才能保证双方看到同一组物理页。MAP_PRIVATE 会触发 COW，PFN 不再相同。               │
├──────────────────────────────┼────────────────────────────────────────────────────────────────────────────────────────────────┤
│ O_DIRECT                     │ buffered I/O 的 page cache 页是内核新分配的，不会匹配已注册 buffer。                           │
├──────────────────────────────┼────────────────────────────────────────────────────────────────────────────────────────────────┤
│ I/O 数据在单个注册 buffer    │ ublk_try_buf_match() 要求所有 bvec 落在同一个 buffer index 且偏移连续。跨多个注册 buffer 的    │
│ 内连续                       │ scatter/gather 无法匹配。                                                                      │
├──────────────────────────────┼────────────────────────────────────────────────────────────────────────────────────────────────┤
│ 只读 buffer 不能用于 READ    │ UBLK_SHMEM_BUF_READ_ONLY 表示内核不能往里面写，所以 READ I/O 不能匹配。                        │
├──────────────────────────────┼────────────────────────────────────────────────────────────────────────────────────────────────┤
│ server 需要 CAP_SYS_ADMIN    │ zero-copy / user-copy server 必须被信任。                                                      │
└──────────────────────────────┴────────────────────────────────────────────────────────────────────────────────────────────────┘

────────────────────────────────────────────────────────────────────────────────

4. 总结

1. 先让客户端和 ublk server 共享同一组物理页：
    • 要么用 memfd + SCM_RIGHTS 把 fd 传过去；
    • 要么用 hugetlbfs/普通文件 + MAP_SHARED，双方 mmap 同一个文件。
2. ublk server 把自己的映射地址通过 UBLK_U_CMD_REG_BUF 注册给内核。
3. 客户端所有 I/O buffer 都从这块共享内存分配，并且以 O_DIRECT 方式下发。
4. 内核在 I/O 到达时通过 PFN 匹配，命中则置 UBLK_IO_F_SHMEM_ZC，server 直接通过 mmap_base + offset 访问数据，实现真正的零拷贝。

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
