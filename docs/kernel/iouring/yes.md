# iouring 杂记
## ioring 函数 `__cold`

例如:
```c
static __cold void io_tctx_exit_cb(struct callback_head *cb)
```

```c
/*
 *   gcc: https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#index-cold-function-attribute
 *   gcc: https://gcc.gnu.org/onlinedocs/gcc/Label-Attributes.html#index-cold-label-attribute
 *
 * When -falign-functions=N is in use, we must avoid the cold attribute as
 * contemporary versions of GCC drop the alignment for cold functions. Worse,
 * GCC can implicitly mark callees of cold functions as cold themselves, so
 * it's not sufficient to add __function_aligned here as that will not ensure
 * that callees are correctly aligned.
 *
 * See:
 *
 *   https://lore.kernel.org/lkml/Y77%2FqVgvaJidFpYt@FVFF77S0Q05N
 *   https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88345#c9
 */
#if !defined(CONFIG_CC_IS_GCC) || (CONFIG_FUNCTION_ALIGNMENT == 0)
#define __cold				__attribute__((__cold__))
#else
#define __cold
#endif
```

文档： https://gcc.gnu.org/onlinedocs/gcc/Label-Attributes.html

表示该函数执行概率很低。

- io_wq_submit_work
  - io_arm_poll_handler
     - `__io_arm_poll_handler`
- io_poll_check_events

## io uring 可以使用的位置?
- aio
- network
  - https://developer.aliyun.com/article/834974
- epoll
- syscall
- dpdk / spdk

写一个综叙来对比他们。

- 相对于 aio 的改进是什么?


## show_fdinfo

```txt
[root@nixos:/proc/118496/fdinfo]# cat 4
pos:    0
flags:  02000002
mnt_id: 14
ino:    240059
SqMask: 0x7f
SqHead: 3777186
SqTail: 3777186
CachedSqHead:   3777186
CqMask: 0x7f
CqHead: 3777060
CqTail: 3777091
CachedCqTail:   3777091
SQEs:   0
CQEs:   31
   36: user_data:0, res:4096, flag:0
   37: user_data:0, res:4096, flag:0
   38: user_data:0, res:4096, flag:0
   39: user_data:0, res:4096, flag:0
   40: user_data:0, res:4096, flag:0
   41: user_data:0, res:4096, flag:0
   42: user_data:0, res:4096, flag:0
   43: user_data:0, res:4096, flag:0
   44: user_data:0, res:4096, flag:0
   45: user_data:0, res:4096, flag:0
   46: user_data:0, res:4096, flag:0
   47: user_data:0, res:4096, flag:0
   48: user_data:0, res:4096, flag:0
   49: user_data:0, res:4096, flag:0
   50: user_data:0, res:4096, flag:0
   51: user_data:0, res:4096, flag:0
   52: user_data:0, res:4096, flag:0
   53: user_data:0, res:4096, flag:0
   54: user_data:0, res:4096, flag:0
   55: user_data:0, res:4096, flag:0
   56: user_data:0, res:4096, flag:0
   57: user_data:0, res:4096, flag:0
   58: user_data:0, res:4096, flag:0
   59: user_data:0, res:4096, flag:0
   60: user_data:0, res:4096, flag:0
   61: user_data:0, res:4096, flag:0
   62: user_data:0, res:4096, flag:0
   63: user_data:0, res:4096, flag:0
   64: user_data:0, res:4096, flag:0
   65: user_data:0, res:4096, flag:0
   66: user_data:0, res:4096, flag:0
SqThread:       -1
SqThreadCpu:    -1
UserFiles:      1
UserBufs:       128
PollList:
CqOverflowList:
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
