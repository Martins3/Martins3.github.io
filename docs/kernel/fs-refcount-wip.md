分析的很有趣， https://zhuanlan.zhihu.com/p/93142176

但是有两个问题:
1. inode 中的 i_count 什么时候超过 1
2. bash 正在一个文件夹中，但是文件夹删除了，cd .. 的结果什么?

## 补充一下其他的
```c
static inline struct task_struct *get_task_struct(struct task_struct *t)
{
	refcount_inc(&t->usage);
	return t;
}
```

## unlink
让 inode ref 减去 1 ，当没有进程 open 之后，那么就会自动删除。
