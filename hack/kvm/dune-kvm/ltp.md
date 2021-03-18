# ltp

大致阅读了一下 ltp 的所用的项目，几乎所有的测试项目都是在 testcases 下，将 testcases/kernel/syscall 主要的测试内容，其余的测试项目主要是 shell 或者测试内核的功能选项的，比较难以进行和 dune 的正确性关联不大。

主要有下面几种
1. 标准 Makefile : 从 .c 直接编译为 exe 文件，修改 .o 规则，通过检测是否链接 -lltp， 
2. 独立 main / 独立 Makefile : 首先编译为 .o 然后编译为 exe ，这种生成两个 .o, 在编译为 exe 的时候判断是否依赖于
3. nfwn

- [ ] 现在的这种高发会进入两次 dune 吗 ?

swapon 需要单独进入编译，否则会出现搞笑的错误

```
➜  syscalls git:(master) ag MAKE_TARGETS

migrate_pages/Makefile
8:MAKE_TARGETS          := $(patsubst $(abs_srcdir)/%.c,%,$(wildcard $(abs_srcdir)/*[0-9].c))
9:$(MAKE_TARGETS): %: migrate_pages_common.o

swapoff/Makefile
9:$(MAKE_TARGETS): %: ../swapon/libswapon.o

nftw/Makefile
15:MAKE_TARGETS         := nftw01 nftw6401 dune_nftw01 dune_nftw6401

delete_module/Makefile
19:MAKE_TARGETS         := delete_module01 delete_module02 delete_module03 \

bpf/Makefile
8:FILTER_OUT_MAKE_TARGETS       := bpf_common
13:$(MAKE_TARGETS): %: bpf_common.o

move_pages/Makefile
10:MAKE_TARGETS         := $(patsubst $(abs_srcdir)/%.c,%,$(wildcard $(abs_srcdir)/*[0-9].c))
12:$(MAKE_TARGETS): %: move_pages_support.o


memfd_create/Makefile
8:FILTER_OUT_MAKE_TARGETS         := memfd_create_common
12:$(MAKE_TARGETS): %: memfd_create_common.o


init_module/Makefile
16:MAKE_TARGETS         := init_module01 init_module02 init_module.ko


swapon/Makefile
13:FILTER_OUT_MAKE_TARGETS         := libswapon
17:$(MAKE_TARGETS): %: libswapon.o

sethostname/Makefile
20:MAKE_TARGETS := sethostname01 sethostname02 sethostname03

finit_module/Makefile
16:MAKE_TARGETS         := finit_module01 finit_module02 finit_module.ko
➜  syscalls git:(master)
```

在一些文件夹下，没有任何的 .c
1. ipc : 更加深层次的
2. paging : 就是空的
3. sethostname : 一些诡异的技术


糟糕，原来是需要 pan-ltp 驱动的
```
+ /opt/ltp/bin/ltp-pan -e -S -a 406798 -n 406798 -p -f /tmp/ltp-F64qpmJ1zw/alltests -l /opt/ltp/results/LTP_RUN_ON-2021_03_15-00h_34m_23s.log -C /opt/ltp/output/LTP_RUN
_ON-2021_03_15-00h_34m_23s.failed -T /opt/ltp/output/LTP_RUN_ON-2021_03_15-00h_34m_23s.tconf
```
其中，alltests 是装配的内容吧 !

## install
```
define generate_install_rule

INSTALL_FILES		+= $$(abspath $$(DESTDIR)/$(3)/$(1))

$$(abspath $$(DESTDIR)/$(3)/$(1)): \
    $$(abspath $$(dir $$(DESTDIR)/$(3)/$(1)))
	install -m $$(INSTALL_MODE) $(shell test -d "$(2)/$(1)" && echo "-d") $(PARAM) "$(2)/$(1)" $$@
	$(shell test -d "$(2)/$(1)" && echo "install -m "'$$(INSTALL_MODE) $(PARAM)' "$(2)/$(1)/*" -t '$$@')
endef

$(foreach install_target,$(INSTALL_TARGETS),$(eval $(call generate_install_rule,$(install_target),$(abs_srcdir),$(INSTALL_DIR))))
$(foreach make_target,$(MAKE_TARGETS),$(eval $(call generate_install_rule,$(make_target),$(abs_builddir),$(INSTALL_DIR))))
```






## 读读文档
Various callbacks can be set by the test writer, including
+test.test_all+, which we have set to +run()+. The test harness will execute
this callback in a separate process (using +fork()+), forcibly terminating it
if it does not return after +test.timeout+ seconds.

./runltp -f syscalls

/home/maritns3/core/loongson-dune/ltp_dir/ltp/lib/README.md :

    library process
    +----------------------------+
    | main                       |
    |  tst_run_tcases            |
    |   do_setup                 |
    |   for_each_variant         |
    |    for_each_filesystem     |   test process
    |     fork_testrun ------------->+--------------------------------------------+
    |      waitpid               |   | testrun                                    |
    |                            |   |  do_test_setup                             |
    |                            |   |   tst_test->setup                          |
    |                            |   |  run_tests                                 |
    |                            |   |   tst_test->test(i) or tst_test->test_all  |
    |                            |   |  do_test_cleanup                           |
    |                            |   |   tst_test->cleanup                        |
    |                            |   |  exit(0)                                   |
    |   do_exit                  |   +--------------------------------------------+
    |    do_cleanup              |
    |     exit(ret)              |
    +----------------------------+

> ### Test library and exec()
> 
> The piece of mapped memory to store the results to is not preserved over
> exec(2), hence to use the test library from a binary started by an exec() it
> has to be remaped. In this case the process must to call tst\_reinit() before
> calling any other library functions. In order to make this happen the program
> environment carries LTP\_IPC\_PATH variable with a path to the backing file on
> tmpfs. This also allows us to use the test library from shell testcases.


## 分析一下代码
- fork_testrun
  * run_tcases_per_fs
  * tst_run_tcases
  - testrun : 首先会调用一下 fork 维持生活
    - do_test_setup
    - run_tests
    - do_test_cleanup

所以，在 testrun 的位置添加一个 config 判断，然后生成两个链接库，对于每一个 syscall 生成两个二进制文件来执行。

- [ ] 但是，是存在那种可以执行所有 syscall 测试的脚本，修改一下这个脚本。

## Makefile 的依赖路径


- abort:Makefile
  - include/mk/testcases.mk : 定义了生成 libltp 的方法
    - include/mk/env_pre.mk : 各种全局变量的定义
    - include/mk/functions.mk : 定义两个函数
      - generate_install_rule
      - get_make_dirs
  - include/mk/generic_leaf_target.mk
     - include/mk/generic_leaf_target.inc
     - include/mk/env_post.mk : 定义各种各种正在测试的变量相关的数据

/home/maritns3/core/loongson-dune/ltp_dir/ltp/include/mk/lib.mk 定义 libltp 被 ar 的位置

正确的方法是，将含有 DUNE flag 的文件和不含有 dune 的文件当做一个文件，分别处理到 tst_test ltp 和各个普通的文件

- [ ] 也许并不是所有的文件都和 ltp 一样简单的

- [ ]  /home/maritns3/core/loongson-dune/ltp_dir/ltp/include/mk/generic_leaf_target.inc 的

## 结论

lib.mk 是提供给不同的

## 使用 UnixBench 测试性能
