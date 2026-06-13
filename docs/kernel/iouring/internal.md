# iouring 实现分析

## tracepoint 的获取基本的

```txt
io_uring_cqe_overflow
io_uring_cqring_wait
io_uring_defer
io_uring_fail_link
io_uring_file_get
io_uring_link
io_uring_poll_arm
io_uring_req_failed
io_uring_short_write

# 不用说了，最重要的
io_uring_submit_req
io_uring_complete

# 也是重要的，但是不是高速路径
io_uring_create
io_uring_register

# io wq 相关
io_uring_queue_async_work
io_uring_task_add
io_uring_task_work_run
io_uring_local_work_run
```

## 关键源码

```c
io_req_flags_t io_file_get_flags(struct file *file)
{
	io_req_flags_t res = 0;

	BUILD_BUG_ON(REQ_F_ISREG_BIT != REQ_F_SUPPORT_NOWAIT_BIT + 1);

	if (S_ISREG(file_inode(file)->i_mode))
		res |= REQ_F_ISREG;
	if ((file->f_flags & O_NONBLOCK) || (file->f_mode & FMODE_NOWAIT))
		res |= REQ_F_SUPPORT_NOWAIT;
	return res;
}
```

```c
static bool io_file_supports_nowait(struct io_kiocb *req, __poll_t mask)
{
	/* If FMODE_NOWAIT is set for a file, we're golden */
	if (req->flags & REQ_F_SUPPORT_NOWAIT)
		return true;
	/* No FMODE_NOWAIT, if we can poll, check the status */
	if (io_file_can_poll(req)) {
		struct poll_table_struct pt = { ._key = mask };

		return vfs_poll(req->file, &pt) & mask;
	}
	/* No FMODE_NOWAIT support, and file isn't pollable. Tough luck. */
	return false;
}
```

## trace_io_uring_complete 的调用路径分析

- 为什么 /tmp/testfile.txt 会触发 io_uring:io_uring_queue_async_work
- 为什么 /dev/zero 不走上面任何一个路径，到底走的什么路径?

使用 code/src/c/iouring/op-read.c 测试，
也就是使用最简单的 io_uring_prep_read ，然后

### /dev/zero

不支持 O_DIRECT ，如果使用 O_DIRECT 打开，会有报错我
```txt
@[
        __io_submit_flush_completions+357
        io_submit_sqes+332
        __do_sys_io_uring_enter+520
        do_syscall_64+132
        entry_SYSCALL_64_after_hwframe+118
]: 1
```

这种，提交就把事情干完了:

```txt
# tracer: function_graph
#
# CPU  DURATION                  FUNCTION CALLS
# |     |   |                     |   |   |   |
 14)               |  io_issue_sqe() {
 14)               |    arch_irq_work_raise() {
 14)               |      x2apic_send_IPI_self() {
 14)   0.737 us    |        irq_enter_rcu();
 14)   0.251 us    |        sched_core_idle_cpu();
 14)   0.197 us    |        raw_irqentry_exit_cond_resched();
 14)   7.014 us    |      }
 14)   7.868 us    |    }
 14)               |    io_file_get_normal() {
 14)               |      fget() {
 14)   0.194 us    |        __rcu_read_lock();
 14)   0.182 us    |        __rcu_read_unlock();
 14)   0.972 us    |      }
 14)   1.396 us    |    }
 14)               |    io_read() {
 14)               |      __io_read() {
 14)               |        io_rw_init_file() {
 14)   0.147 us    |          io_file_get_flags();
 14)   0.729 us    |        }
 14)   0.160 us    |        io_file_supports_nowait();
 14)               |        rw_verify_area() {
 14)   0.428 us    |          security_file_permission();
 14)   0.774 us    |        }
 14)               |        read_iter_zero() {
 14)   0.593 us    |          lock_mm_and_find_vma();
 14)   5.185 us    |          handle_mm_fault();
 14)   0.163 us    |          up_read();
 14)   0.142 us    |          raw_irqentry_exit_cond_resched();
 14)   8.045 us    |        }
 14)   2.530 us    |          arch_irq_work_raise();
 14) + 13.935 us   |      }
 14)               |      kiocb_done() {
 14)               |        io_req_io_end() {
 14)   0.211 us    |          __fsnotify_parent();
 14)   0.960 us    |        }
 14)   0.137 us    |        io_rw_recycle();
 14)   1.699 us    |      }
 14) + 16.250 us   |    }
 14) + 28.356 us   |  }
```

例如这种，所有的核心逻辑都是在一个函数中，根本不支持提交 + 异步完成的能力:
```c
static ssize_t read_iter_zero(struct kiocb *iocb, struct iov_iter *iter)
{
	size_t written = 0;

	while (iov_iter_count(iter)) {
		size_t chunk = iov_iter_count(iter), n;

		if (chunk > PAGE_SIZE)
			chunk = PAGE_SIZE;	/* Just for latency reasons */
		n = iov_iter_zero(chunk, iter);
		if (!n && iov_iter_count(iter))
			return written ? written : -EFAULT;
		written += n;
		if (signal_pending(current))
			return written ? written : -ERESTARTSYS;
		if (!need_resched())
			continue;
		if (iocb->ki_flags & IOCB_NOWAIT)
			return written ? written : -EAGAIN;
		cond_resched();
	}
	return written;
}
```

所以，这种设备，就是在 io_uring_enter 的时候就会完成所有的功能:
```
@[
        read_iter_zero+5
        __io_read+188
        io_read+21
        __io_issue_sqe+58
        io_issue_sqe+57
        io_submit_sqes+274
        __do_sys_io_uring_enter+520
        do_syscall_64+132
        entry_SYSCALL_64_after_hwframe+118
]: 1
```

### shmem

无论是否 O_DIRECT ，都是这个结果，因为 shmem 本质不支持 O_DIRECT ，
所以使用 io-wq 来异步化。
```txt
@[
        io_req_complete_post+300
        io_issue_sqe+374
        io_wq_submit_work+188
        io_worker_handle_work+331
        io_wq_worker+222
        ret_from_fork+242
        ret_from_fork_asm+26
]: 1
```

通过 tracepoint 可以找到:
```txt
1498273.295 op-read.out/57888 io_uring:io_uring_queue_async_work(ctx: 0xffff88812e738800, req: 0xffff888106761500, opcode: 22, flags: 550834274304, work: 0xffff8881067615d8, op_str: "READ")

@[
        io_queue_iowq+193
        io_submit_sqes+711
        __do_sys_io_uring_enter+520
        do_syscall_64+132
        entry_SYSCALL_64_after_hwframe+118
]: 1
```
这个行为和 fsync 差不多了，可以自动转换

问题的本质在这里:
```c
static inline void io_queue_sqe(struct io_kiocb *req, unsigned int extra_flags)
	__must_hold(&req->ctx->uring_lock)
{
	unsigned int issue_flags = IO_URING_F_NONBLOCK |
				   IO_URING_F_COMPLETE_DEFER | extra_flags;
	int ret;

	ret = io_issue_sqe(req, issue_flags);

	/*
	 * We async punt it if the file wasn't marked NOWAIT, or if the file
	 * doesn't support non-blocking read/write attempts
	 */
	if (unlikely(ret))
		io_queue_async(req, issue_flags, ret);
}
```

```txt
@[
        __io_read+1
        io_read+21
        __io_issue_sqe+58
        io_issue_sqe+57
        io_submit_sqes+274
        __do_sys_io_uring_enter+520
        do_syscall_64+132
        entry_SYSCALL_64_after_hwframe+118
]: 1
@[
        __io_read+1
        io_read+21
        __io_issue_sqe+58
        io_issue_sqe+57
        io_wq_submit_work+188
        io_worker_handle_work+331
        io_wq_worker+222
        ret_from_fork+242
        ret_from_fork_asm+26
]: 1
```

```txt
+ sudo bpftrace -e 'kretprobe:__io_read { printf("returned: %lx\n", retval); } interval:s:1000 { exit(); }'
Attaching 2 probes...
returned: fffffff5 (-11 也就是 EAGAIN)
returned: 26
```
第一次提交得到的是 EAGAIN，然后放到 io-wq 中执行:

```txt
 31)               |    __io_read() {
 31)               |      io_rw_init_file() {
 31)   0.183 us    |        io_file_get_flags();
 31)   0.521 us    |      }
 31)               |      rw_verify_area() {
 31)               |        security_file_permission() {
 31)   0.315 us    |          selinux_file_permission();
 31)   0.089 us    |          bpf_lsm_file_permission();
 31)   0.725 us    |        }
 31)   0.952 us    |      }
 31)               |      shmem_file_read_iter() {
 31)               |        shmem_get_folio_gfp() {
 31)   0.416 us    |          filemap_get_entry();
 31)   0.833 us    |        }
 31)   0.090 us    |        folio_unlock();
 31)   0.106 us    |        folio_mark_accessed();
 31)               |        lock_mm_and_find_vma() {
 31)   0.306 us    |          down_read_trylock();
 31)   0.501 us    |          find_vma();
 31)   1.432 us    |        }
 31)               |        handle_mm_fault() {
 31)   5.508 us    |          __handle_mm_fault();
 31)   0.089 us    |          __rcu_read_lock();
 31)   0.095 us    |          mem_cgroup_from_task();
 31)   0.130 us    |          count_memcg_events();
 31)   0.274 us    |          __rcu_read_unlock();
 31)   7.369 us    |        }
 31)   0.099 us    |        up_read();
 31)   0.324 us    |        raw_irqentry_exit_cond_resched();
 31)               |        touch_atime() {
 31)   0.536 us    |          atime_needs_update();
 31)   0.193 us    |          mnt_get_write_access();
 31)   1.121 us    |          generic_update_time();
 31)   0.160 us    |          mnt_put_write_access();
 31)   3.107 us    |        }
 31) + 15.782 us   |      }
 31) + 17.978 us   |    }
 31)               |    kiocb_done() {
 31)   0.117 us    |      io_req_io_end();
 31)   0.600 us    |    }
 31) + 25.544 us   |  }
```

#### 是如何被唤醒的?

```txt
sudo bpftrace -e 'kprobe:io_worker_handle_work { @[kstack(bpftrace)] = count(); } interval:s:1000 { exit(); }'

@[
        io_worker_handle_work+1
        io_wq_worker+222
        ret_from_fork+242
        ret_from_fork_asm+26
]: 83
```

还是记错了，似乎在 io_worker_handle_work 中就是完成所有的工作的?
```txt
 12)               |  io_worker_handle_work() {
 12)   0.053 us    |    _raw_spin_lock();
 12)   0.048 us    |    _raw_spin_unlock();
 12)   0.048 us    |    _raw_spin_unlock();
 12)   0.052 us    |    _raw_spin_lock();
 12)   0.048 us    |    _raw_spin_unlock();
 12)   0.069 us    |    _raw_spin_lock();
 12)   0.048 us    |    _raw_spin_unlock();
 12)               |    io_wq_submit_work() {
 12)               |      io_issue_sqe() {
 12)               |        io_write() {
 12)   0.112 us    |          io_rw_init_file();
 12)   0.170 us    |          rw_verify_area();
 12)   7.905 us    |          ext4_file_write_iter();
 12)   0.202 us    |          kiocb_done();
 12)   9.669 us    |        }
 12)               |        io_req_complete_post() {
 12)   0.617 us    |          __io_req_task_work_add();
 12)   0.799 us    |        }
 12) + 10.760 us   |      }
 12) + 10.899 us   |    }
 12)   0.063 us    |    _raw_spin_lock();
 12)   0.057 us    |    _raw_spin_unlock();
 12)   0.286 us    |    io_wq_free_work();
 12)   0.063 us    |    _raw_spin_lock();
 12)   0.060 us    |    _raw_spin_unlock();
 12)   0.063 us    |    _raw_spin_lock_irq();
 12)   0.058 us    |    _raw_spin_unlock_irq();
 12) + 13.271 us   |  }
```
我靠，这个不是变成了纯串行的了

#### 唤醒之后，还有什么工作要做?
也就是即便是被唤醒，也是有工作做的?

### xfs
#define FILENAME "/home/martins3/data/testfile.txt"

如果是 O_DIRECT ，结果为:
```txt
@[
        __traceiter_io_uring_complete+58
        __io_submit_flush_completions+357
        ctx_flush_and_put+49
        io_handle_tw_list+177
        tctx_task_work_run+81
        tctx_task_work+58
        task_work_run+89
        io_run_task_work+78
        io_cqring_wait+923
        __do_sys_io_uring_enter+321
        do_syscall_64+132
        entry_SYSCALL_64_after_hwframe+118
]: 1
```

看上去，还是在同步等待啊 : IORING_ENTER_GETEVENTS
(亲爹，这样的智商就别来看内核代码了，看看天线宝宝养养脑吧)

显然，是一个提交，然后一个等待
```txt
io_uring_enter(4, 1, 0, 0, NULL, 8)     = 1
io_uring_enter(4, 0, 1, IORING_ENTER_GETEVENTS, NULL, 8) = 0
```

如果不是 O_DIRECT 的，那么:
```txt
@[
        __traceiter_io_uring_complete+58
        __io_submit_flush_completions+357
        io_submit_sqes+332
        __do_sys_io_uring_enter+520
        do_syscall_64+132
        entry_SYSCALL_64_after_hwframe+118
]: 1
```

如果直接命中了 page cache ，那么:
```txt
io_uring_enter(4, 1, 0, 0, NULL, 8)     = 1
```
如果没有命中 page cache ，那么结果为，这个效果和
```txt
io_uring_enter(4, 1, 0, 0, NULL, 8)     = 1
io_uring_enter(4, 0, 1, IORING_ENTER_GETEVENTS, NULL, 8) = 0
```

O_DIRECT 的时候:
```txt
 16)               |    io_read() {
 16)               |      __io_read() {
 16)               |        io_rw_init_file() {
 16)   0.332 us    |          io_file_get_flags();
 16)   1.279 us    |        }
 16)   0.315 us    |        io_file_supports_nowait();
 16)               |        rw_verify_area() {
 16)   0.970 us    |          security_file_permission();
 16)   1.732 us    |        }
 16)               |        xfs_file_read_iter [xfs]() {
 16) + 81.150 us   |          xfs_file_dio_read [xfs]();
 16) + 87.432 us   |        }
 16) + 93.506 us   |      }
 16) + 94.402 us   |    }
 16) ! 115.179 us  |  }
```

不是 O_DIRECT 的时候:
```txt
  8)               |    io_read() {
  8)               |      __io_read() {
  8)               |        io_rw_init_file() {
  8)   0.447 us    |          io_file_get_flags();
  8)   1.868 us    |        }
  8)   0.502 us    |        io_file_supports_nowait();
  8)               |        rw_verify_area() {
  8)   1.304 us    |          security_file_permission();
  8)   2.418 us    |        }
  8)               |        xfs_file_read_iter [xfs]() {
  8) + 42.789 us   |          xfs_file_buffered_read [xfs]();
  8) + 47.868 us   |        }
  8)               |        xfs_file_read_iter [xfs]() {
  8)   2.844 us    |          xfs_file_buffered_read [xfs]();
  8)   3.840 us    |        }
  8) + 60.603 us   |      }
  8)               |      kiocb_done() {
  8)               |        io_req_io_end() {
  8)   0.565 us    |          __fsnotify_parent();
  8)   1.883 us    |        }
  8)   0.506 us    |        io_rw_recycle();
  8)   4.386 us    |      }
  8) + 67.107 us   |    }
  8) + 89.555 us   |  }
```


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
