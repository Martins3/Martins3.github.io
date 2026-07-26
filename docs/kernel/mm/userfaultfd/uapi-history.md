# userfaultfd UAPI history

本文盘点 userfaultfd 用户态可见接口的首次合入时间、主线版本、提交和动机。

口径：

- 时间使用 commit date。
- 版本写成 first rc / first final。
- 当前 ABI 定义主要在 `include/uapi/linux/userfaultfd.h`。
- 文档入口是 `Documentation/admin-guide/mm/userfaultfd.rst`。

## 创建与基础协议

| 接口                                                           |               首次版本 / 时间 | commit         | 动机                                                                                                             |
| -------------------------------------------------------------- | ----------------------------: | -------------- | ---------------------------------------------------------------------------------------------------------------- |
| `userfaultfd(2)` syscall                                       |   v4.3-rc1 / v4.3, 2015-09-04 | `86039bd3b4e6` | 引入 memory externalization：用户态 manager 通过 fd 接管指定 VA range 的缺页处理。                               |
| `read()`/`poll()` + `struct uffd_msg` / `UFFD_EVENT_PAGEFAULT` |   v4.3-rc1 / v4.3, 2015-09-04 | `a9b85f9415fd` | 把 read 协议改成结构化 `uffd_msg`，给后续事件和 feature 扩展留 ABI 空间。                                        |
| `UFFDIO_API`                                                   |   v4.3-rc1 / v4.3, 2015-09-04 | `1038628d80e9` | 初始化并协商 API、feature bit 和可用 ioctl bitmask。                                                             |
| `UFFDIO_REGISTER`                                              |   v4.3-rc1 / v4.3, 2015-09-04 | `1038628d80e9` | 注册一个 VA range，并指定 missing/wp/minor 等跟踪模式。                                                          |
| `UFFDIO_UNREGISTER`                                            |   v4.3-rc1 / v4.3, 2015-09-04 | `1038628d80e9` | 注销 range。                                                                                                     |
| `UFFDIO_WAKE`                                                  |   v4.3-rc1 / v4.3, 2015-09-04 | `1038628d80e9` | 配合 `DONTWAKE`，允许用户态批量填页后显式唤醒 faulting thread。                                                  |
| `UFFD_USER_MODE_ONLY`                                          | v5.11-rc1 / v5.11, 2020-12-15 | `37cd0575b851` | 安全收敛：允许创建只能处理 user-mode fault 的 uffd，避免 unprivileged 用户拖住 kernel fault 来扩大漏洞竞态窗口。 |
| `/dev/userfaultfd` + `USERFAULTFD_IOC_NEW`                     |   v6.1-rc1 / v6.1, 2022-09-11 | `2d5de004e009` | 更细粒度授权：用文件权限授予 userfaultfd 能力，避免给 QEMU 等进程 `CAP_SYS_PTRACE`。                             |

## 核心 ioctl

| ioctl                 |               首次版本 / 时间 | commit         | 动机                                                                                                                                                                                                     |
| --------------------- | ----------------------------: | -------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `UFFDIO_COPY`         |   v4.3-rc1 / v4.3, 2015-09-04 | `1f1c6f075904` | missing fault 时由用户态提供页内容，原子复制到 fault 地址。实际 ioctl 实现同日提交 `ad465cae96b4`。                                                                                                      |
| `UFFDIO_ZEROPAGE`     |   v4.3-rc1 / v4.3, 2015-09-04 | `1f1c6f075904` | missing fault 时原子映射零页或填零，QEMU postcopy 中用于零页优化。实际 ioctl 实现同日提交 `ad465cae96b4`。                                                                                               |
| `UFFDIO_WRITEPROTECT` |   v5.7-rc1 / v5.7, 2020-04-07 | `63b2d4174c4a` | 引入 uffd-wp：用户态对 range 做页级写保护，用于变更跟踪和 dirty tracking；同系列正式启用在 `e06f1e1dd499`。                                                                                              |
| `UFFDIO_CONTINUE`     | v5.13-rc1 / v5.13, 2021-05-05 | `f619147104c8` | minor fault 的解析接口：底层页已存在，用户态确认或修正内容后让内核继续建立映射。                                                                                                                         |
| `UFFDIO_POISON`       |   v6.6-rc1 / v6.6, 2023-08-18 | `fc71884a5f59` | VM live migration 时保留 memory poison 语义：目标机遇到 poisoned page 可安装 marker，之后访问直接 `SIGBUS`。feature/ioctl 广告补充在 `f442ab50f5fb`。                                                    |
| `UFFDIO_MOVE`         |   v6.8-rc1 / v6.8, 2023-12-29 | `adef440691ba` | 避免 `UFFDIO_COPY` 的分配和 memcpy，面向 heap compaction/page recycling；提交信息提到 Pixel 6 benchmark 中 compacting thread 完成时间下降 40% 以上，也支持同 VMA 内移动 swapped-out pages 而不触碰内容。 |

## 事件与 feature 扩展

| feature / event                                               |               首次版本 / 时间 | commit         | 动机                                                                                                                            |
| ------------------------------------------------------------- | ----------------------------: | -------------- | ------------------------------------------------------------------------------------------------------------------------------- |
| `UFFD_FEATURE_EVENT_FORK` / `UFFD_EVENT_FORK`                 | v4.11-rc1 / v4.11, 2017-02-22 | `893e26e61d04` | non-cooperative monitor 需要跟踪被监控进程 fork 后的新 uffd context。                                                           |
| `UFFD_FEATURE_EVENT_REMAP` / `UFFD_EVENT_REMAP`               | v4.11-rc1 / v4.11, 2017-02-22 | `72f87654c696` | 通知 `mremap()` 导致的 VA 移动。                                                                                                |
| `UFFD_FEATURE_EVENT_REMOVE` / `UFFD_EVENT_REMOVE`             | v4.11-rc1 / v4.11, 2017-02-24 | `d811914d8757` | 通知 `MADV_DONTNEED/MADV_REMOVE`，manager 后续 fault 时应 zeromap。原始 madvise event 提交是 `05ce77249d50`。                   |
| `UFFD_FEATURE_EVENT_UNMAP` / `UFFD_EVENT_UNMAP`               | v4.11-rc1 / v4.11, 2017-02-24 | `897ab3e0c49e` | 通知 unmap，避免 manager 对已经消失的 range 继续 `UFFDIO_COPY`。                                                                |
| `UFFD_FEATURE_SIGBUS`                                         | v4.14-rc1 / v4.14, 2017-09-06 | `2d6d6f5a09a9` | 不发 pagefault event，直接向 faulting process 发 `SIGBUS`；典型场景是数据库显式管理 hugetlbfs hole。                            |
| `UFFD_FEATURE_THREAD_ID`                                      | v4.14-rc1 / v4.14, 2017-09-06 | `9d4ac934829a` | 在 `uffd_msg.pagefault.feat.ptid` 返回 faulting thread id，用于 postcopy 迁移停顿统计等。                                       |
| `UFFDIO_REGISTER_MODE_MINOR` / `UFFD_FEATURE_MINOR_HUGETLBFS` | v5.13-rc1 / v5.13, 2021-05-05 | `7677f7fd8be7` | 支持 hugetlbfs minor fault 拦截，live migration 目标端可先运行，按需校验或修正已存在页。                                        |
| `UFFD_FEATURE_MINOR_SHMEM`                                    | v5.14-rc1 / v5.14, 2021-06-30 | `964ab0040ff9` | 把 minor fault 支持扩展并广告到 shmem。                                                                                         |
| `UFFD_FEATURE_EXACT_ADDRESS`                                  | v5.18-rc1 / v5.18, 2022-03-22 | `824ddc601adc` | 返回未 page-align mask 的真实 fault address，方便用户态做对象级 prefetch 决策，同时保持旧行为兼容。                             |
| `UFFD_FEATURE_WP_HUGETLBFS_SHMEM`                             | v5.19-rc1 / v5.19, 2022-05-13 | `b1f9e876862d` | 在 shmem/hugetlbfs 上启用 uffd-wp。                                                                                             |
| `UFFD_FEATURE_WP_UNPOPULATED`                                 |   v6.4-rc1 / v6.4, 2023-04-05 | `2bad466cc9d9` | 允许 anonymous memory 对 none PTE 也建立 uffd-wp marker，QEMU snapshot/dirty tracking 可避免预读和预 fault。                    |
| `UFFD_FEATURE_WP_ASYNC`                                       |   v6.7-rc1 / v6.7, 2023-10-18 | `d61ea1cb0095` | 异步 dirty tracking：写保护 fault 由内核自动解除并记录，目标是替代或增强 soft-dirty，支持 `PAGEMAP_SCAN`/GetWriteWatch 类场景。 |
| `UFFD_FEATURE_MOVE`                                           |   v6.8-rc1 / v6.8, 2023-12-29 | `adef440691ba` | 广告 `UFFDIO_MOVE` 能力。                                                                                                       |

## 注意点

- `UFFDIO_REGISTER_MODE_WP` 这个宏在 v4.3 初始 uapi 里已有占位，但真正可用的 write-protect API 是 v5.7 的 `UFFDIO_WRITEPROTECT` 系列。
- `UFFDIO_COPY` 和 `UFFDIO_ZEROPAGE` 的 uapi 定义提交是 `1f1c6f075904`，真正 ioctl 实现提交是同日的 `ad465cae96b4`。
- `UFFDIO_POISON` 的结构和 ioctl 首次出现在 `fc71884a5f59`，但加入 `UFFD_API_FEATURES` 和 range ioctl 广告是在 `f442ab50f5fb`。
- `UFFDIO_MOVE` 是页表 remap 语义，不是从任意 userspace buffer memcpy 到 fault 地址；当前约束主要面向 non-shared anonymous pages。

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
