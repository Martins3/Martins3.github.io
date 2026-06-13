# cgroup 的基础设施

## 主要数据结构
- task_struct::cgroups (css_set): 指向 task 所属的 ？？
  - cgroup_move_task / copy_process => cgroup_fork 的时候维护
- css_set : 正如其名，a set of css
- cgroup_subsys_state : 只是描述一个 subsystem 的状态
  - cgroup_subsys : 描述一个 subsys 的实现
- cgroup : 感觉和 css_set 的定位有点重叠，都是一组的，但是似乎是为了支持 unified tree
- [ ] cgroup_root
- [ ] css_set::dfl_cgrp


创建 struct cgroup 的流程:
- do_syscall_64
  - do_syscall_x64
    - __x64_sys_mkdir
      - __se_sys_mkdir
        - __do_sys_mkdir
          - do_mkdirat
            - vfs_mkdir
              - kernfs_iop_mkdir
                - cgroup_mkdir
                  - cgroup_create

- [ ] 为什么 ssh login 的时候需要
- kernfs_fop_write_iter
  - cgroup_procs_write
    - __cgroup_procs_write
      - cgroup_attach_task
        - cgroup_migrate_prepare_dst
          - find_css_set
            - find_existing_css_set
              - compare_css_sets

- [ ] 还有这个
- kernfs_fop_write_iter
  - cgroup_subtree_control_write
    - cgroup_apply_control
      - cgroup_apply_control_enable
        - css_populate_dir

- kernfs_fop_write_iter
  - cgroup_procs_write
    - __cgroup_procs_write
      - cgroup_attach_task
        - cgroup_migrate_prepare_dst
          - find_css_set
            - link_css_set

## fork && exit
- copy_process
  - cgroup_can_fork : pid subsys 需要对于资源检查
    - cgroup_css_set_fork : find or create a css_set for a child process
    - cgroup_subsys::can_fork
  - cgroup_fork : 初始化
  - cgroup_post_fork
    - cgroup_subsys::can_fork

- do_exit
  - cgroup_exit
    - cgroup_subsys::exit

## 从 kernfs 到具体的行为
```c
// 读写文件的
static struct kernfs_ops cgroup_kf_single_ops = {

}

// 同上，但是应该是单个文件
static struct kernfs_ops cgroup_kf_ops = {
}

// 目录相关的
static struct kernfs_syscall_ops cgroup_kf_syscall_ops = {
}

struct cgroup_file; // kernfs 文件的文件需要携带 cgroup 相关的信息
struct cftype; // 提前定义的 cgroup file 的静态信息

struct file_system_type cgroup_fs_type;
static const struct fs_context_operations cgroup_fs_context_ops; // 处理 mount 参数
```

- cgroup_file_write
  - cftype::write
  - cftype::write_u64
  - cftype::write_s64

```c
/* look up cgroup associated with given css_set on the specified hierarchy */
static struct cgroup *cset_cgroup_from_root(struct css_set *cset,
              struct cgroup_root *root)
{
  struct cgroup *res = NULL;

  lockdep_assert_held(&cgroup_mutex);
  lockdep_assert_held(&css_set_lock);

  // 这个暂时看不懂
  if (cset == &init_css_set) {
    res = &root->cgrp;
  // 如果说是 v2 那么就靠 css_set 定义的 dfl_cgrp
  } else if (root == &cgrp_dfl_root) {
    res = cset->dfl_cgrp;
  // v1 的情况 : task should move to same root
  } else {
    struct cgrp_cset_link *link;

    list_for_each_entry(link, &cset->cgrp_links, cgrp_link) {
      struct cgroup *c = link->cgrp;

      if (c->root == root) {
        res = c;
        break;
      }
    }
  }

  BUG_ON(!res);
  return res;
}
```
利用整体框架的，内容就是展示 cgroup 中的进程

## migrate
```c
/* used to track tasks and csets during migration */
struct cgroup_taskset {
  /* the src and dst cset list running through cset->mg_node */
  struct list_head  src_csets;
  struct list_head  dst_csets;

  /* the number of tasks in the set */
  int     nr_tasks;

  /* the subsys currently being processed */
  int     ssid;

  /*
   * Fields for cgroup_taskset_*() iteration.
   *
   * Before migration is committed, the target migration tasks are on
   * ->mg_tasks of the csets on ->src_csets.  After, on ->mg_tasks of
   * the csets on ->dst_csets.  ->csets point to either ->src_csets
   * or ->dst_csets depending on whether migration is committed.
   *
   * ->cur_csets and ->cur_task point to the current task position
   * during iteration.
   */
  struct list_head  *csets;
  struct css_set    *cur_cset;
  struct task_struct  *cur_task;
};
```

- cgroup_procs_write
  `__cgroup_procs_write`
    - cgroup_attach_task
      - cgroup_migrate_add_src
      - cgroup_migrate_prepare_dst
      - cgroup_migrate
        - cgroup_migrate_add_task

- A process can be *migrated* to another cgroup. Migration of a process doesn’t affect already existing descendant processes.

操作方法:
```sh
cgclassify -g subsystems:path_to_cgroup pidlist
```

## [ ] 实际上，cgroup 的热迁移是默认不迁移内存的
- https://docs.kernel.org/admin-guide/cgroup-v1/memory.html#move-charges-at-task-migration

## 关注下的函数
- [ ] cgroup_setup_root

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
