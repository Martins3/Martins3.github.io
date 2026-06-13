CC := gcc
CXX := clang++
uname_m := $(shell uname -m)

CFLAGS := -Wall -g
CXXFLAGS := -Wall -O3 -g

ifeq ($(uname_m), aarch64)
ARCH_FLAGS := -march=armv8.3-a
endif

ifeq ($(uname_m), x86_64)
ARCH_FLAGS := -z noexecstack -no-pie
endif

# Source files and output targets
ROOT_SRCS := $(wildcard ./*.c)
ROOT_OUTS := $(ROOT_SRCS:.c=.out)

PTHREAD_SRCS := $(wildcard ./pthread/*.c)
PTHREAD_OUTS := $(PTHREAD_SRCS:.c=.out)

AO_SRCS := $(wildcard ./access-once/*.c)
AO_OUTS := $(AO_SRCS:.c=.out)

CPPSTD_SRCS := $(wildcard ./cpp-std/*.cpp ./cpp-std/*/*.cpp ./cpp-std/*/*/*.cpp)
CPPSTD_OUTS := $(CPPSTD_SRCS:.cpp=.out)

ARCH_SRCS := $(wildcard ./arch/$(uname_m)/*.S)
ARCH_OUTS := arch/main.out

TOOLS_SRCS := $(wildcard ./tools/*.c)
TOOLS_OUTS := $(TOOLS_SRCS:.c=.out)

CCIA_SRCS := $(wildcard ./ccia/*.cpp)
CCIA_OUTS := $(CCIA_SRCS:.cpp=.out)

SL_SRCS := $(wildcard ./split-lock/*.cpp)
SL_OUTS := $(SL_SRCS:.cpp=.out)

ALL_OUTS := $(ROOT_OUTS) $(PTHREAD_OUTS) $(AO_OUTS) $(CPPSTD_OUTS) $(ARCH_OUTS) $(TOOLS_OUTS) $(CCIA_OUTS) $(SL_OUTS)

.PHONY: all clean drd hel tsan dis arch-dis

all: $(ALL_OUTS)

# Root directory C files
%.out: %.c
	$(CC) $(CFLAGS) -o $@ $<

# pthread C files
pthread/%.out: pthread/%.c
	$(CC) $(CFLAGS) -lpthread -o $@ $<

# access-once C files
access-once/%.out: access-once/%.c
	$(CC) $(CFLAGS) -lpthread -o $@ $<

# cpp-std C++ files
cpp-std/%.out: cpp-std/%.cpp
	$(CXX) -Wall -std=c++20 -O3 -g $(ARCH_FLAGS) -o $@ $^

# tools C files
tools/%.out: tools/%.c
	$(CC) $(CFLAGS) -lpthread -o $@ $<

# ccia C++ files
ccia/%.out: ccia/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ -latomic

# split-lock C++ files
split-lock/%.out: split-lock/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

# arch special rules
arch/main.o: arch/main.c
	$(CC) -Wall -g -O3 -c -o $@ $<

arch/main.out: arch/main.o $(ARCH_SRCS)
	$(CC) $(ARCH_FLAGS) -o $@ $^

# Override generic rules for specific targets
pthread/elimination-backoff-stack.out: pthread/elimination-backoff-stack.c
	$(CC) -o $@ $< -lpthread -O2 -Wall -g -std=c11

pthread/michael-scott-queue.out: pthread/michael-scott-queue.c
	$(CC) -o $@ $< -lpthread -O2 -Wall -g -std=c11

# Special targets
drd: tools/drd.out
	valgrind --tool=drd ./tools/drd.out protect

hel: tools/drd.out
	valgrind --tool=helgrind ./tools/drd.out

tsan: tools/drd.out
	clang -fsanitize=thread -O3 -g tools/drd.c -lpthread -o tools/drd.out
	./tools/drd.out

dis: cpp-std/memory-model/disassembly/mm-dis.out
	gdb -quiet cpp-std/memory-model/disassembly/mm-dis.out -ex "disass test_cmp_exchange" -ex "q"

arch-dis: arch/main.out
	gdb -quiet arch/main.out -ex "disass main" -ex "q"

clean:
	rm -f $(ALL_OUTS) arch/main.o cpp-std/*.out
