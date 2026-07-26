SRCS = $(wildcard ./*.S)
OUTS = $(SRCS:.S=.out)

$(info $(SRCS))

CFLAGS = -Wall -g -O0

all: $(OUTS)

lib.o:lib.c
	$(CC) $(CFLAGS) -c -o $@ $<

# 关于 noexecstack
# https://stackoverflow.com/questions/73435637/how-can-i-fix-usr-bin-ld-warning-trap-o-missing-note-gnu-stack-section-imp
# 不明觉厉啊
#
# https://stackoverflow.com/questions/19364969/compilation-fails-with-relocation-r-x86-64-32-against-rodata-str1-8-can-not
# 为什么 x86 就需要添加上 pie 啊
%.out:%.S lib.o
	$(CC) $(CFLAGS) -z noexecstack -no-pie -o $@ $^

clean:
	rm -f $(OUTS)

dis:
	gdb -quiet test.out -ex "disass add" -ex "q"
	gdb -quiet test.out -ex "disass non_leaf_add" -ex "q"
