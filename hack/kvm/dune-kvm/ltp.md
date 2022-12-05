# ltp

大致阅读了一下 ltp 的所用的项目，几乎所有的测试项目都是在 testcases 下，将 testcases/kernel/syscall 主要的测试内容，其余的测试项目主要是 shell 或者测试内核的功能选项的，比较难以进行和 dune 的正确性关联不大。

主要有下面几种
1. 标准 Makefile : 从 .c 直接编译为 exe 文件，修改 .o 规则，通过检测是否链接 -lltp， 
2. 独立 main / 独立 Makefile : 首先编译为 .o 然后编译为 exe ，这种生成两个 .o, 在编译为 exe 的时候判断是否依赖于
3. nfwn


- [ ] swapon 需要单独进入编译，否则会出现搞笑的错误


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

## 需要增加的 patch
generate_runtest.sh
