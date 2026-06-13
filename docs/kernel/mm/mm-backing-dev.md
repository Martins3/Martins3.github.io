# Backing Device

之所以不是 block dev ，是为了照顾 nfs 9p 之类的 backend

backing-dev.c 主要提供的 bdi 就是为了 page-writeback 使用。
backing-dev.c 主要使用了 sysfs 和 debugfs

如果理解这个参数就差不多了吧。

## bdi

```c
struct backing_dev_info
```

- ret_from_fork
  - kernel_init
    - kernel_init_freeable
      - do_basic_setup
        - do_initcalls
          - do_initcall_level
            - do_one_initcall
              - loop_init
                - loop_add
                  - __blk_mq_alloc_disk
                    - __alloc_disk_node
                      - bdi_alloc
                        - bdi_init
                          - cgwb_bdi_init
                            - wb_init

每一个 device 初始化两个 workqueue
- wb_workfn
- wb_update_bandwidth_workfn

如果想要进一步分析，看看 page-writeback 机制吧

## 基本结构
- wb_get_create : bdi + memcg_css 得到 bdi_writeback

bdi_collect_stats 有多个
	list_for_each_entry_rcu(wb, &bdi->wb_list, bdi_node)

## 和 cgroup 的关系

- wb_get_create : 这个参数为什么是 memcg_css ，wb 不是存在自己的 cgroup 吗
  - cgwb_create

cgroup 需要使用 CONFIG_CGROUP_WRITEBACK 来控制

## 如果系统内存很小，快速的 dirty page，如何防止的?

猜测是，dirty page 就需要分配 page ，然后这个时候需要将老的 dirty page 写回去。

但是现在没有什么直观的测试方法!


## nfs 挂载，为什么会有四个调用

```txt
^C
  bdi_alloc
  super_setup_bdi_name
  nfs_get_tree_common
  vfs_get_tree
  nfs_do_submount
  nfs4_submount
  nfs_d_automount
  __traverse_mounts
  step_into
  path_lookupat
  filename_lookup
  vfs_path_lookup
  mount_subtree
  do_nfs4_mount
  nfs4_try_get_tree
  vfs_get_tree
  do_new_mount
  __se_sys_mount
  do_syscall_64
  entry_SYSCALL_64_after_hwframe
  [unknown]
    1

  bdi_alloc
  super_setup_bdi_name
  nfs_get_tree_common
  vfs_get_tree
  fc_mount
  do_nfs4_mount
  nfs4_try_get_tree
  vfs_get_tree
  do_new_mount
  __se_sys_mount
  do_syscall_64
  entry_SYSCALL_64_after_hwframe
  [unknown]
    1

  bdi_alloc
  super_setup_bdi_name
  nfs_get_tree_common
  vfs_get_tree
  nfs_do_submount
  nfs4_submount
  nfs_d_automount
  __traverse_mounts
  step_into
  link_path_walk
  path_lookupat
  filename_lookup
  vfs_path_lookup
  mount_subtree
  do_nfs4_mount
  nfs4_try_get_tree
  vfs_get_tree
  do_new_mount
  __se_sys_mount
  do_syscall_64
  entry_SYSCALL_64_after_hwframe
  [unknown]
    2

```

nfs_get_tree_common : 如何理解 nfs 的 s_dev
```c
		error = super_setup_bdi_name(s, "%u:%u", MAJOR(server->s_dev),
					     MINOR(server->s_dev));
```

## CONFIG_CGROUP_WRITEBACK 的确是占据一半内容
如果一个 cgroup 中同时用了多个 bdi ，如何控制?

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
