# 总结proc 实现

4. how we implemented /proc/self ?

8. generic.c : how to create entry and remove entry ?
    3. 如何形成文件树的层级结构的 ?

9. @todo plka :last two section : task sysclt
10. @todo kernel/sysctl.c is related ?



关键的几个文件:
1. base.c : process specific information 的形成。
2. task_mmu.c : 同上，但是描述的内容，并不是非常清楚。 task_nommu.c 考虑没有mmu也就是没有虚实映射的情况。
3. vmcore.c : [kdump](https://en.wikipedia.org/wiki/Kdump_(Linux)) 感觉和proc 没有关系啊 !
4. array.c : 应该是为了支持　cat /proc/self/status　但是文件名非常奇怪啊!


| File          | blank | comment | code | node |
|---------------|-------|---------|------|------|
| base.c        | 553   | 338     | 2929 |      |
| task_mmu.c    | 276   | 168     | 1470 |      |
| proc_sysctl.c | 242   | 159     | 1326 |      |
| vmcore.c      | 207   | 310     | 1068 |      |
| generic.c     | 107   | 51      | 615  |      |
| array.c       | 87    | 113     | 567  |      |
| kcore.c       | 80    | 68      | 509  |      |
| inode.c       | 61    | 35      | 395  |      |
| fd.c          | 64    | 5       | 294  |      |
| proc_net.c    | 56    | 61      | 276  |      |
| root.c        | 54    | 27      | 249  |      |
| task_nommu.c  | 52    | 20      | 230  |      |
| page.c        | 51    | 54      | 230  |      |
| internal.h    | 37    | 65      | 202  |      |
| stat.c        | 41    | 5       | 192  |      |
| namespaces.c  | 25    | 1       | 157  |      |
| meminfo.c     | 19    | 1       | 139  |      |
| proc_tty.c    | 18    | 30      | 131  |      |
| nommu.c       | 17    | 13      | 88   |      |
| consoles.c    | 13    | 7       | 78   |      |
| bootconfig.c  | 13    | 6       | 70   |      |
| thread_self.c | 9     | 4       | 60   |      |
| self.c        | 9     | 5       | 59   |      |
| devices.c     | 7     | 2       | 50   |      |
| kmsg.c        | 10    | 7       | 48   |      |
| uptime.c      | 5     | 1       | 35   |      |
| interrupts.c  | 5     | 5       | 32   |      |
| Makefile      | 3     | 4       | 29   |      |
| loadavg.c     | 4     | 1       | 28   |      |
| cpuinfo.c     | 4     | 1       | 26   |      |
| softirqs.c    | 4     | 4       | 25   |      |
| util.c        | 2     | 0       | 22   |      |
| version.c     | 2     | 1       | 20   |      |
| cmdline.c     | 2     | 1       | 16   | It's fucking really cool !      |
| fd.h          | 6     | 1       | 13   |      |

## How does this form the interface ?

```c
	union {
		const struct proc_ops *proc_ops;
		const struct file_operations *proc_dir_ops;
	};
	const struct dentry_operations *proc_dops;
	union {
		const struct seq_operations *seq_ops;
		int (*single_show)(struct seq_file *, void *);
	};
```
