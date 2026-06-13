# kernel/fsnotify.c

## 分析一下调用关系

```c
audit_dupe_exe : audit_watch.c
audit_data_to_entry : auditfilter.c
  audit_alloc_mark



static const struct fsnotify_ops audit_mark_fsnotify_ops = {
	.handle_event =	audit_mark_handle_event,
	.free_mark = audit_fsnotify_free_mark,
};

static int __init audit_fsnotify_init(void)
{
	audit_fsnotify_group = fsnotify_alloc_group(&audit_mark_fsnotify_ops);
	if (IS_ERR(audit_fsnotify_group)) {
		audit_fsnotify_group = NULL;
		audit_panic("cannot create audit fsnotify group");
	}
	return 0;
}

// 分析 audit_fsnotify_group 的引用位置 :

```



