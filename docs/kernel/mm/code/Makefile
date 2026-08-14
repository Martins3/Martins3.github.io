FILTER_OUT = $(foreach v,$(2),$(if $(findstring $(1),$(v)),,$(v)))
ALL_SRCS := $(wildcard ./*.c)
# lib.c 是公共库(由 rmap.out 单独链接);
# output.c / pageflags-expand.c 是 page-flags 宏展开的产物, 不是独立程序
SRCS := $(call FILTER_OUT,pageflags-expand,$(call FILTER_OUT,output,$(call FILTER_OUT,lib,$(ALL_SRCS))))

# numa.c 需要 numactl-devel(numah + libnuma); 未安装时自动跳过
HAVE_NUMA := $(shell printf '#include <numa.h>\nint main(void){return 0;}\n' | $(CC) -x c - -lnuma -o /dev/null 2>/dev/null && echo y)
ifeq ($(HAVE_NUMA),y)
OUTS := $(SRCS:.c=.out)
else
OUTS := $(filter-out ./numa.out,$(SRCS:.c=.out))
endif

$(info $(OUTS))

CFLAGS = -Wall -g
# -laio

all: $(OUTS)

%.out:%.o
	$(CC) $(CFLAGS) -o $@ $<

rmap.out:rmap.o lib.c
	$(CC) $(CFLAGS) -o $@ $^

madvise-locked-page.out : madvise-locked-page.c
	$(CC) $(CFLAGS) -laio -o $@ $^

numa.out: numa.o
	$(CC) -lnuma -o $@ $^

clean:
	rm -f $(OUTS)
