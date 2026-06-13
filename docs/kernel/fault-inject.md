# 错误注入

## 可以注入错误的点
总体来说，注入错误的本质是来自于硬件故障，而非内核本身有问题:
- 文件系统
- block 层
- 内存
- scheduler
- 网络
- PCIe
- usb

### nvme 似乎自带了一个体系?

```txt
➜  debug find /sys/kernel/debug -name fault_inject
/sys/kernel/debug/nvme1n1/fault_inject
/sys/kernel/debug/nvme0n1/fault_inject
/sys/kernel/debug/nvme1/fault_inject
/sys/kernel/debug/nvme0/fault_inject
```
但是其中的内容看上去差不多，这是因为都是使用相同的框架。

其注入错误的位置为: http://127.0.0.1:3434/fault-injection/nvme-fault-injection.html

### 网络

tc qdisc add dev lo root netem delay 100ms
tc qdisc change dev lo root netem delay 1000ms
tc qdisc change dev lo root netem loss 0.1%

### block

在 /sys/block/sdb 下可以看到 io-timeout-fail 和 make-it-fail

#### timeout 注入


主线:
```sh
pushd /sys/kernel/debug/fail_io_timeout
echo 0 >interval
echo 100 >times
echo -1 >times
echo 100 >probability
echo 1 > verbose

echo 1 >/sys/block/vda/io-timeout-fail
```

老版本
```sh
pushd /sys/kernel/debug/fail_io_timeout
echo 0 >interval
echo 200 >times
echo 100 >probability
echo 1 > verbose

echo 1 >/sys/block/sda/io-timeout-fail
```


### [ ] notifier

### make-it-fail

这里来控制具体的 device 应该失败，但是失败的形式是放到
/sys/kernel/debug/fail_make_request 中

```txt
pushd /sys/kernel/debug/fail_make_request
echo 0 >interval
echo -1 >times
echo 100 >probability
popd

echo 1 >/sys/block/sda/make-it-fail
```


### PCIe

```txt
config PCIEAER
	bool "PCI Express Advanced Error Reporting support"
	depends on PCIEPORTBUS
	select RAS
	help
	  This enables PCI Express Root Port Advanced Error Reporting
	  (AER) driver support. Error reporting messages sent to Root
	  Port will be handled by PCI Express AER driver.

config PCIEAER_INJECT
	tristate "PCI Express error injection support"
	depends on PCIEAER
	select GENERIC_IRQ_INJECTION
	help
	  This enables PCI Express Root Port Advanced Error Reporting
	  (AER) software error injector.

	  Debugging AER code is quite difficult because it is hard
	  to trigger various real hardware errors. Software-based
	  error injection can fake almost all kinds of errors with the
	  help of a user space helper tool aer-inject, which can be
	  gotten from:
	     https://github.com/intel/aer-inject.git
```

## 统一注入框架
```plain
CONFIG_GENERIC_IRQ_INJECTION=y
CONFIG_HAVE_FUNCTION_ERROR_INJECTION=y
# CONFIG_BLK_DEV_NULL_BLK_FAULT_INJECTION is not set
# CONFIG_EDAC_AMD64_ERROR_INJECTION is not set
# CONFIG_NFSD_FAULT_INJECTION is not set
# CONFIG_NOTIFIER_ERROR_INJECTION is not set
CONFIG_FUNCTION_ERROR_INJECTION=y
CONFIG_FAULT_INJECTION=y
CONFIG_FAULT_INJECTION_DEBUG_FS=y
CONFIG_FAIL_MAKE_REQUEST=y
```
lib/fault-inject.c
lib/fault-inject-usercopy.c

## 想法
1. 调查下 qemu 存在错误注入框架吗? (存储，内存，网络延迟)

QEMU 应该是不存在的，这是一个可以提的内容，
如果不行，也可以思考下。
或者吧这个 patch 提交上去，让大家看看。



## 注入框架基本原则
- `fault_create_debugfs_attr` 来创建的

make-it-fail 和 /sys/kernel/debug/fail_make_request
io-timeout-fail 和 /sys/kernel/debug/fail_io_timeout
似乎其他的注入是没有这种对应的关系的，可以调查下


## 基本的错误注入框架
```txt
➜  ~ cd /sys/kernel/debug/fail_
fail_function/      fail_io_timeout/    fail_page_alloc/    fail_usercopy/
fail_futex/         fail_make_request/  failslab/
```
这里负责具体的参数调整


## fault-injection
- https://www.kernel.org/doc/html/latest/fault-injection/index.html

- https://lxadm.com/using-fault-injection/ : 直接 echo /sys/block/sdb/sdb1/make-it-fail
- linux/tools/testing/fault-injection/failcmd.sh

关注一下从这里开始的:
/home/martins3/core/linux/lib/error-inject.c

这个应该是后来没有合并进去:
https://github.com/ionos-enterprise/fault-injection

/sys/kernel/debug/fail_function/inject 中可以插入的位置，除去 syscall 之后，似乎就很少了
```txt
__filemap_add_folio     ERRNO
should_failslab ERRNO
should_fail_alloc_page  TRUE
should_fail_bio ERRNO
```

```txt
519:ALLOW_ERROR_INJECTION(should_fail_bio, ERRNO);
```


- 似乎一共出现上述模块中放上一些 hook 而已。
- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - __x64_sys_ppoll
        - __se_sys_ppoll
          - __do_sys_ppoll
            - poll_select_finish
              - put_timespec64
                - copy_to_user
                  - _copy_to_user
                    - should_fail_usercopy
                      - should_fail
                        - should_fail_ex

## BPF
- https://chaos-mesh.org/docs/simulate-kernel-chaos-on-kubernetes/
  - https://github.com/chaos-mesh/bpfki
    - 无法编译

这里说的限制应该是不存在的吧:
```txt
 long bpf_override_return(struct pt_regs *regs, u64 rc)
  Description
    Used for error injection, this helper uses kprobes to override
    the return value of the probed function, and to set it to *rc*.
    The first argument is the context *regs* on which the kprobe
    works.

    This helper works by setting the PC (program counter)
    to an override function which is run in place of the original
    probed function. This means the probed function is not run at
    all. The replacement function just returns with the required
    value.

    This helper has security implications, and thus is subject to
    restrictions. It is only available if the kernel was compiled
    with the **CONFIG_BPF_KPROBE_OVERRIDE** configuration
    option, and in this case it only works on functions tagged with
    **ALLOW_ERROR_INJECTION** in the kernel code.

    Also, the helper is only available for the architectures having
    the CONFIG_FUNCTION_ERROR_INJECTION option. As of this writing,
    x86 architecture is the only one to support this feature.
  Return
    0
```
https://github.com/iovisor/bpftrace/blob/master/docs/reference_guide.md

## 用户态

### ndbkit

https://news.ycombinator.com/item?id=35899527
> nbdkit memory 10G --filter=error error-rate=1%

https://www.libguestfs.org/nbdkit-error-filter.1.html

https://www.kernel.org/doc/html/latest/admin-guide/device-mapper/dm-flakey.html

### fuse 注入
https://medium.com/@siddontang/use-fuse-to-inject-failure-to-i-o-deb5f2e7800a

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
