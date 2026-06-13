# Makefile
目前最好的教程是 [Learn Makefiles : With the tastiest examples](https://makefiletutorial.com/#getting-started)
- 首先一个重点讲解各个案例
- 然后提供一个模板，这个模板应该可以应付大多数个人项目。

如果是在有什么不懂的可以再去阅读[官方文档](https://www.gnu.org/software/make/manual/html_node/index.html)

[跟我一起写 Makefile](https://github.com/seisman/how-to-write-makefile) 也是一个不错的中文参考。

## 一些技巧
- [利用 git 和 makefile 来构建测试](https://chrismorgan.info/blog/make-and-git-diff-test-harness/)

## 遇到的过问题

### 使用数组
```mk
KENREL_MODULES= a b c
$(info $(KENREL_MODULES))
all:
	@for module in $(KENREL_MODULES); do \
		echo "$$module"; \
		done;
```

### test if variable not empty

https://stackoverflow.com/questions/38801796/how-to-conditionally-set-makefile-variable-to-something-if-it-is-empty

```mk
ifeq ($(TEST),)
TEST := $(something else)
endif
```

## 检测工具
-  https://github.com/mrtazz/checkmake

## generate file if doesn't exists
https://stackoverflow.com/questions/12605051/how-to-check-if-a-directory-doesnt-exist-in-make-and-create-it

```txt
object/%.o: code/%.cc | object
    compile $< somehow...

object:
    mkdir -p $@
```

## 资源
https://news.ycombinator.com/item?id=42945146

## 记录一个经典案例

为了让可以自动把目录下的所有的 .c 文件都添加为 .o ，然后放过来:
```mk
# https://stackoverflow.com/questions/6145041/makef ile-filter-out-strings-containing-a-character
FILTER_OUT = $(foreach v,$(2),$(if $(findstring $(1),$(v)),,$(v)))

# 每一个命令都是 stackoverflow 得到的，makefile 太难了
SRCS = $(wildcard $(PWD)/*.c)
OBJS=$(notdir $(SRCS:.c=.o))
# 为了将中间产物 martins3.mod.c 过滤掉，不然会同时构建两个产物的
GENE_OBJS := $(call FILTER_OUT,mod.o, $(OBJS))

uname_p := $(shell uname -m)
SRCS := $(patsubst %.c,%.o,$(wildcard $(PWD)/arch/$(uname_p)/*.c)) \
	$(patsubst %.S,%.o,$(wildcard $(PWD)/arch/$(uname_p)/*.S))
ARCH_OBJS := $(addprefix arch/$(uname_p)/, $(notdir $(SRCS)))

# TODO 有办法合并这些内容吗？
SRCS = $(wildcard $(PWD)/concurrent/*.c)
CONCURRENT_OBJS := $(addprefix concurrent/, $(notdir $(SRCS:.c=.o)))

SRCS = $(wildcard $(PWD)/mm/*.c)
MM_OBJS := $(addprefix mm/, $(notdir $(SRCS:.c=.o)))

SRCS = $(wildcard $(PWD)/sched/*.c)
SCHED_OBJS := $(addprefix sched/, $(notdir $(SRCS:.c=.o)))

SRCS = $(wildcard $(PWD)/ds/*.c)
DS_OBJS := $(addprefix ds/, $(notdir $(SRCS:.c=.o)))

SRCS = $(wildcard $(PWD)/time/*.c)
TIME_OBJS := $(addprefix time/, $(notdir $(SRCS:.c=.o)))

ALL_OBJS := $(GENE_OBJS) $(ARCH_OBJS) $(CONCURRENT_OBJS) $(MM_OBJS) $(SCHED_OBJS) $(DS_OBJS) $(TIME_OBJS)
martins3-m := $(call FILTER_OUT,-user, $(ALL_OBJS))
```

## just 只是 改进一下 makefie 吗?
https://github.com/casey/just

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
