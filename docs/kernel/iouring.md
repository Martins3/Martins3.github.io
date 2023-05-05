# io uring
- https://blog.vmsplice.net/2022/06/comparing-virtio-nvme-and-iouring-queue.html

- 基于 io-uring 的 ubd： https://lwn.net/Articles/900690/

- [迟先生 : io_uring 的接口与实现](https://www.skyzh.dev/posts/articles/2021-06-14-deep-dive-io-uring/)

![](https://kernel.taobao.org/2020/09/IO_uring_Optimization_for_Nginx/3.png)
- [ ] https://github.com/frevib/io_uring-echo-server : 别 手高眼低 了，先学会如何使用吧。

https://despairlabs.com/posts/2021-06-16-io-uring-is-not-an-event-system/

- [A journey to io_uring, AIO and modern storage devices](https://clickhouse.com/blog/en/2021/reading-from-external-memory/)

 - [ ] [^8]
io_uring 有如此出众的性能，主要来源于以下几个方面：
1. 用户态和内核态共享提交队列（submission queue）和完成队列（completion queue）
2. IO 提交和收割可以 offload 给 Kernel，且提交和完成不需要经过系统调用（system call）
3. 支持 Block 层的 Polling 模式
4. 通过提前注册用户态内存地址，减少地址映射的开销
5. 不仅如此，io_uring 还可以完美支持 buffered IO，而 libaio 对于 buffered IO 的支持则一直是被诟病的地方。

- [ ] TODO doc should be read later

[Linux I/O 原理和 Zero-copy 技术全面揭秘](https://zhuanlan.zhihu.com/p/308054212)
- [ ] sendfile, etc


[^10]
read the doc :
aio's limitation :
1. only support async IO for O_DIRECT
> TODO how O_DIRECT works
> 1. checked
> 2. skip page cache
2. some constrans for sync submit
3. api lead noticeable copying

TODO please continue the documentation


- [ ] [liburing](https://github.com/axboe/liburing)

read write 会直到事情做完
epoll : 当有的工作做好之后，通知，其实是一个 thread 可以监控一大群的 socket，任何一个 ready 之后就开始工作

https://thenewstack.io/how-io_uring-and-ebpf-will-revolutionize-programming-in-linux/ : 这个也可以参考参考
https://kernel-recipes.org/en/2019/talks/faster-io-through-io_uring/

[nice blog](https://github.com/shuveb/io_uring-by-example) provide example code about liburing and directly syscall


[Man page]
  - io_uring_setup :

- IORING_SETUP_IOPOLL
- IORING_SETUP_SQPOLL
- IORING_ENTER_GETEVENTS
- IORING_ENTER_SQ_WAKEUP

- https://github.com/tokio-rs/io-uring
- https://lwn.net/Articles/879724/ ：终于，两年之后，开始处理网络的问题了

- https://lwn.net/Articles/903855/ : 据说可以优化 qcow2
  - https://lore.kernel.org/io-uring/20220509092312.254354-1-ming.lei@redhat.com/


- https://unixism.net/2020/04/io-uring-by-example-article-series/
- https://lwn.net/Articles/776703/

[^7]: [io uring and eBPF](https://thenewstack.io/how-io_uring-and-ebpf-will-revolutionize-programming-in-linux/)
[^8]: [zhihu : io_uring introduction](https://zhuanlan.zhihu.com/p/62682475?utm_source=wechat_timeline)
[^10]: [Efficient IO with io_uring](https://kernel.dk/io_uring.pdf)



## 问题
- 为什么 def_blk_fops iopoll 完全没有办法用上啊
  - 似乎 iopoll 的用户似乎只有 iouring 的

- 难道是我的 fio 有问题，也许 fio 使用的 liburing 的动态库有问题?
```c
const struct file_operations def_blk_fops = {
	.open		= blkdev_open,
	.release	= blkdev_close,
	.llseek		= blkdev_llseek,
	.read_iter	= blkdev_read_iter,
	.write_iter	= blkdev_write_iter,
	.iopoll		= iocb_bio_iopoll,
	.mmap		= generic_file_mmap,
	.fsync		= blkdev_fsync,
	.unlocked_ioctl	= blkdev_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= compat_blkdev_ioctl,
#endif
	.splice_read	= generic_file_splice_read,
	.splice_write	= iter_file_splice_write,
	.fallocate	= blkdev_fallocate,
};
```

## Welcome to Lord of the io_uring
https://news.ycombinator.com/item?id=23132549

## [io_uring is not an event system](https://news.ycombinator.com/item?id=27540248)

看了半天，一会搞出什么?  都是类似这种:
> epoll is based on a "readiness" model (i.e. it tells when when you can start I/O). io_uring is based on a "completion" model (i.e. it tells you when I/O is finished).

相比 aio，似乎只是少了一次 getevent 那个系统调用而已。

## 也不知道相对于 epoll 存在什么优势
虽然存在测试
https://github.com/frevib/io_uring-echo-server

我感觉更多的是因为 aio 搞不好就不是真的 aio 吧
