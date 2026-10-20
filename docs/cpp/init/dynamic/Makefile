CXX ?= g++
CXXFLAGS ?= -std=c++20 -O0 -g -Wall -Wextra -Wpedantic

.DEFAULT_GOAL := build
.DELETE_ON_ERROR:
.PHONY: all build run run-order run-scanf clean help

DEMOS := dynamic-ab.out dynamic-ba.out dynamic-scanf.out
ORDER_SOURCES := dynamic-order-a.cpp dynamic-order-b.cpp dynamic-order-main.cpp

all: build
build: $(DEMOS)

# 显式保留源文件的顺序，便于对比链接顺序对初始化的影响。
dynamic-ab.out: $(ORDER_SOURCES) dynamic-order.hpp Makefile
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $(ORDER_SOURCES) $(LDLIBS) -o $@

dynamic-ba.out: $(ORDER_SOURCES) dynamic-order.hpp Makefile
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) dynamic-order-b.cpp dynamic-order-a.cpp dynamic-order-main.cpp $(LDLIBS) -o $@

dynamic-scanf.out: dynamic-scanf.cpp Makefile
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $< $(LDLIBS) -o $@

# 自动运行全部示例；scanf 的输入固定为 7，不需要等待键盘输入。
run: run-order dynamic-scanf.out
	printf '7\n' | ./dynamic-scanf.out

# 放在同一个 recipe 中，make -j 时两份输出也不会交错。
run-order: dynamic-ab.out dynamic-ba.out
	./dynamic-ab.out
	./dynamic-ba.out

# 手动输入一个整数，观察全局变量动态初始化中的 scanf。
run-scanf: dynamic-scanf.out
	./dynamic-scanf.out

clean:
	$(RM) $(DEMOS)

help:
	@printf '%s\n' \
		'make           编译全部 demo（C++20）' \
		'make run       运行全部 demo，自动向 scanf 输入 7' \
		'make run-order 对比 a→b、b→a 两种链接顺序' \
		'make run-scanf 运行 scanf 示例，手动输入整数' \
		'make clean     删除本 Makefile 生成的 .out 文件'
