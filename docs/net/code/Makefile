SRCS = $(wildcard ./*.c)
OUTS = $(SRCS:.c=.out)

# 当前 shell 是 nix 环境，PATH 里的 cc/ld 是 nix 版本：
# nix ld 会给产物注入 nix glibc 的 RUNPATH，导致动态链接的 binary 一运行就
# stack smashing abort。强制使用系统 gcc 并通过 -B/usr/bin 让 collect2
# 优先找到 /usr/bin/ld.bfd。
CC = /usr/bin/gcc
CFLAGS = -Wall -g -B/usr/bin

GLIB_LIB     = $(shell pkg-config --libs gthread-2.0)
GLIB_INCLUDE = $(shell pkg-config --cflags glib-2.0)

all: $(OUTS)

pcap-sniffex.out : pcap-sniffex.c
	$(CC) $(CFLAGS) -lpcap -o $@ $<

%.out:%.o
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(OUTS)
