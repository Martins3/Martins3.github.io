# 注意: nix 环境的 clang 缺 libclang_rt.tsan.a, 所以这里用 gcc
CC := gcc
TSAN_FLAGS := -fsanitize=thread -O1 -g -Wall -lpthread

.PHONY: all tsan missed asan-tsan deadlock myasan clean

all: tsan missed deadlock

# --- TSan 能测到的: 普通 data race ---
race-counter.tsan.out: race-counter.c
	$(CC) $(TSAN_FLAGS) $< -o $@

tsan: race-counter.tsan.out
	-./race-counter.tsan.out
	./race-counter.tsan.out protect

# --- TSan 测不到的: 内联汇编 / 未执行的路径 ---
missed-asm.tsan.out: missed-asm.c
	$(CC) $(TSAN_FLAGS) $< -o $@

missed-coverage.tsan.out: missed-coverage.c
	$(CC) $(TSAN_FLAGS) $< -o $@

missed: missed-asm.tsan.out missed-coverage.tsan.out
	./missed-asm.tsan.out
	./missed-coverage.tsan.out
	-./missed-coverage.tsan.out racy

# --- ASan 和 TSan 能否同时打开: 不能, 编译器直接报错 ---
asan-tsan:
	@echo "== gcc =="
	! gcc -fsanitize=address,thread -O1 -g race-counter.c -lpthread -o /dev/null
	@echo "== clang =="
	! clang -fsanitize=address,thread -O1 -g race-counter.c -lpthread -o /dev/null

# --- 死锁检测: 必须用 clang runtime, gcc 的 TSan 不支持 detect_deadlocks ---
# nix 的 clang 没把 compiler-rt 挂到默认搜索路径, 这里手动找出来链接.
# 注意必须 --whole-archive: 只按需拉取时 tsan 的 preinit 初始化代码不会
# 被链入, runtime 完全不生效 (实测连 data race 都不报).
CLANG := clang
CLANG_RT ?= $(firstword $(wildcard /nix/store/*compiler-rt-libc-21*/lib/linux/libclang_rt.tsan-x86_64.a))

DEADLOCK_SRCS := deadlock-abba.c deadlock-real.c deadlock-semaphore.c deadlock-condvar.c
DEADLOCK_OUTS := $(DEADLOCK_SRCS:.c=.tsan-clang.out)

%.tsan-clang.out: %.c
	$(CLANG) -fsanitize=thread -O1 -g -Wall -c $< -o $*.o
	$(CLANG) $*.o -Wl,--whole-archive $(CLANG_RT) -Wl,--no-whole-archive -lpthread -ldl -lm -o $@
	rm -f $*.o

# 第一个能报告 lock-order-inversion, 后三个挂死且无报告, 靠 timeout 杀掉
deadlock: $(DEADLOCK_OUTS)
	-TSAN_OPTIONS="detect_deadlocks=1" timeout 10 ./deadlock-abba.tsan-clang.out
	-TSAN_OPTIONS="detect_deadlocks=1" timeout 5 ./deadlock-real.tsan-clang.out
	-TSAN_OPTIONS="detect_deadlocks=1" timeout 5 ./deadlock-semaphore.tsan-clang.out
	-TSAN_OPTIONS="detect_deadlocks=1" timeout 5 ./deadlock-condvar.tsan-clang.out

# --- 自己实现 ASan 的 callback (KASAN 的角色扮演) ---
# -fsanitize=kernel-address: 编译器只插桩, 不链接 runtime, 我们自己在
# myasan.c 里提供 __asan_load/storeN_noabort / __asan_register_globals 和
# shadow memory. instrumentation-with-call-threshold=0 即 KASAN_OUTLINE:
# 所有访问都走 callback; asan-stack=0 避免引入 __asan_stack_malloc 一族.
KASAN_FLAGS := -fsanitize=kernel-address -mllvm -asan-instrumentation-with-call-threshold=0 -mllvm -asan-stack=0 -O1 -g -Wall

myasan-demo.kasan.out: myasan-demo.c myasan.c
	$(CLANG) $(KASAN_FLAGS) -c myasan-demo.c -o myasan-demo.o
	$(CLANG) -O1 -g -Wall -c myasan.c -o myasan.rt.o
	$(CLANG) -rdynamic myasan-demo.o myasan.rt.o -o $@
	rm -f myasan-demo.o myasan.rt.o

myasan: myasan-demo.kasan.out
	./myasan-demo.kasan.out

clean:
	rm -f *.out *.o
