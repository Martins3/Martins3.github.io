# ltp

- [x] top_srcdir ??

Makefile
```
top_srcdir		?= ../../../..

include $(top_srcdir)/include/mk/testcases.mk

include $(top_srcdir)/include/mk/generic_leaf_target.mk
```

include/mk/generic_leaf_target.mk
```
include $(top_srcdir)/include/mk/env_post.mk
include $(top_srcdir)/include/mk/generic_leaf_target.inc
```

include/mk/testcases.mk
```
include $(top_srcdir)/include/mk/env_pre.mk
include $(top_srcdir)/include/mk/functions.mk

APICMDS_DIR	:= $(abs_top_builddir)/tools/apicmds

LIBLTP_DIR	:= $(abs_top_builddir)/lib

LIBLTP		:= $(LIBLTP_DIR)/libltp.a

$(APICMDS_DIR)/tst_kvercmp: $(APICMDS_DIR)
	$(MAKE) -C "$^" -f "$(abs_top_srcdir)/tools/apicmds/Makefile" all

$(LIBLTP): $(LIBLTP_DIR)
	$(MAKE) -C "$^" -f "$(abs_top_srcdir)/lib/Makefile" all

MAKE_DEPS	:= $(LIBLTP)

INSTALL_DIR	:= testcases/bin

LDLIBS		+= -lltp

ifdef LTPLIBS

LTPLIBS_DIRS = $(addprefix $(abs_top_builddir)/libs/lib, $(LTPLIBS))
LTPLIBS_FILES = $(addsuffix .a, $(addprefix $(abs_top_builddir)/libs/, $(foreach LIB,$(LTPLIBS),lib$(LIB)/lib$(LIB))))

MAKE_DEPS += $(LTPLIBS_FILES)

.PHONY: $(LTPLIBS_FILES)

$(LTPLIBS_FILES): $(LTPLIBS_DIRS)

$(LTPLIBS_FILES): %:
ifdef VERBOSE
	$(MAKE) -C "$(dir $@)" -f "$(subst $(abs_top_builddir),$(abs_top_srcdir),$(dir $@))/Makefile" all
else
	@echo "BUILD $(notdir $@)"
	@$(MAKE) --no-print-directory -C "$(dir $@)" -f "$(subst $(abs_top_builddir),$(abs_top_srcdir),$(dir $@))/Makefile" all
endif

LDFLAGS += $(addprefix -L$(top_builddir)/libs/lib, $(LTPLIBS))

endif

$(LTPLIBS_DIRS) $(APICMDS_DIR) $(LIBLTP_DIR): %:
	mkdir -p "$@"
```

## 读读文档

Various callbacks can be set by the test writer, including
+test.test_all+, which we have set to +run()+. The test harness will execute
this callback in a separate process (using +fork()+), forcibly terminating it
if it does not return after +test.timeout+ seconds.

./runltp -f syscalls

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
