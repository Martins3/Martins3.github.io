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
```txt
#0  cgroup_create (mode=493, name=0xffff88810c19d478 "mem", parent=0xffffffff83168350 <cgrp_dfl_root+16>) at kernel/cgroup/cgroup.c:5588
#1  cgroup_mkdir (parent_kn=0xffff8881003eb100, name=0xffff88810c19d478 "mem", mode=<optimized out>) at kernel/cgroup/cgroup.c:5738
#2  0xffffffff814dcbb8 in kernfs_iop_mkdir (idmap=<optimized out>, dir=<optimized out>, dentry=0xffff88810c19d440, mode=<optimized out>) at fs/kernfs/dir.c:1219
#3  0xffffffff8143dada in vfs_mkdir (idmap=0xffffffff831abe20 <nop_mnt_idmap>, dir=0xffff888110092490, dentry=dentry@entry=0xffff88810c19d440, mode=<optimized out>, mode@entry
=509) at fs/namei.c:4115
#4  0xffffffff814429e1 in do_mkdirat (dfd=dfd@entry=-100, name=0xffff888106312000, mode=mode@entry=509) at ./include/linux/mount.h:80
#5  0xffffffff81442c3c in __do_sys_mkdir (mode=<optimized out>, pathname=<optimized out>) at fs/namei.c:4158
#6  __se_sys_mkdir (mode=<optimized out>, pathname=<optimized out>) at fs/namei.c:4156
#7  __x64_sys_mkdir (regs=<optimized out>) at fs/namei.c:4156
#8  0xffffffff822a4e9c in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90002397f58) at arch/x86/entry/common.c:50
#9  do_syscall_64 (regs=0xffffc90002397f58, nr=<optimized out>) at arch/x86/entry/common.c:80
```

- [ ] 为什么 ssh login 的时候需要
```txt
#0  0xffffffff8122f5e3 in compare_css_sets (template=<optimized out>, new_cgrp=<optimized out>, old_cset=<optimized out>, cset=<optimized out>) at kernel/cgroup/cgroup.c:1041
#1  find_existing_css_set (template=0xffffc90001d73c40, cgrp=0xffff88811311a000, old_cset=0xffff888106ce2800) at kernel/cgroup/cgroup.c:1120
#2  find_css_set (old_cset=old_cset@entry=0xffff888106ce2800, cgrp=0xffff88811311a000) at kernel/cgroup/cgroup.c:1222
#3  0xffffffff81230a58 in cgroup_migrate_prepare_dst (mgctx=mgctx@entry=0xffffc90001d73d30) at kernel/cgroup/cgroup.c:2814
#4  0xffffffff81231b0b in cgroup_attach_task (dst_cgrp=dst_cgrp@entry=0xffff88811311a000, leader=leader@entry=0xffff888106910000, threadgroup=threadgroup@entry=true) at kernel
/cgroup/cgroup.c:2918
#5  0xffffffff81233f34 in __cgroup_procs_write (of=0xffff888110d58cc0, buf=<optimized out>, threadgroup=threadgroup@entry=true) at kernel/cgroup/cgroup.c:5172
#6  0xffffffff81234007 in cgroup_procs_write (of=<optimized out>, buf=<optimized out>, nbytes=5, off=<optimized out>) at kernel/cgroup/cgroup.c:5185
#7  0xffffffff814de799 in kernfs_fop_write_iter (iocb=0xffffc90001d73ea0, iter=<optimized out>) at fs/kernfs/file.c:334
```
- [ ] 还有这个
```txt
#0  css_populate_dir (css=css@entry=0xffff888112c89000) at kernel/cgroup/cgroup.c:1750
#1  0xffffffff812322f8 in cgroup_apply_control_enable (cgrp=cgrp@entry=0xffff888112c82800) at kernel/cgroup/cgroup.c:3257
#2  0xffffffff81233d2b in cgroup_apply_control (cgrp=0xffff888112c82800) at kernel/cgroup/cgroup.c:3331
#3  cgroup_subtree_control_write (of=0xffff88810005dbc0, buf=<optimized out>, nbytes=8, off=<optimized out>) at kernel/cgroup/cgroup.c:3485
#4  0xffffffff814de799 in kernfs_fop_write_iter (iocb=0xffffc90000017ea0, iter=<optimized out>) at fs/kernfs/file.c:334
```

```txt
#0  link_css_set (cgrp=0xffff88810702a800, cset=0xffff888107d45000, tmp_links=0xffffc90000017c30) at kernel/cgroup/cgroup.c:1183
#1  find_css_set (old_cset=old_cset@entry=0xffff88810d812c00, cgrp=0xffff88810702a800) at kernel/cgroup/cgroup.c:1264
#2  0xffffffff81230a58 in cgroup_migrate_prepare_dst (mgctx=mgctx@entry=0xffffc90000017d30) at kernel/cgroup/cgroup.c:2814
#3  0xffffffff81231b0b in cgroup_attach_task (dst_cgrp=dst_cgrp@entry=0xffff88810702a800, leader=leader@entry=0xffff88811b202300, threadgroup=threadgroup@entry=true) at kernel
/cgroup/cgroup.c:2918
#4  0xffffffff81233f34 in __cgroup_procs_write (of=0xffff88810005dbc0, buf=<optimized out>, threadgroup=threadgroup@entry=true) at kernel/cgroup/cgroup.c:5172
#5  0xffffffff81234007 in cgroup_procs_write (of=<optimized out>, buf=<optimized out>, nbytes=5, off=<optimized out>) at kernel/cgroup/cgroup.c:5185
#6  0xffffffff814de799 in kernfs_fop_write_iter (iocb=0xffffc90000017ea0, iter=<optimized out>) at fs/kernfs/file.c:334
```


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
