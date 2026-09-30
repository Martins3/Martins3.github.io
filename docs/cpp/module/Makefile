CXX ?= g++
CXX20 = $(CXX) -std=c++20 -fmodules-ts -Wall -Wextra
CXX23 = $(CXX) -std=c++23 -fmodules-ts -Wall -Wextra

# 定位 libstdc++ 的 C++ 头文件目录，用于编译 C++23 的 std 模块（bits/std.cc）
CXX_INCLUDE_CXX := $(shell echo | $(CXX) -E -x c++ -v - 2>&1 | \
	awk '/#include <...> search starts here:/{f=1;next}/End of search list/{f=0}f' | head -1)
STD_CC := $(CXX_INCLUDE_CXX)bits/std.cc

OUTS := basic-main.out impl-main.out partition-main.out \
        header-unit-main.out gmf-main.out import-std.out

.PHONY: all clean

all: $(OUTS)
	@echo "=== basic ===";        ./basic-main.out
	@echo "=== impl unit ===";    ./impl-main.out
	@echo "=== partition ===";    ./partition-main.out
	@echo "=== header unit ===";  ./header-unit-main.out
	@echo "=== gmf + std ===";    ./gmf-main.out
	@echo "=== import std ===";   ./import-std.out

# ---- basic：接口单元 + 消费者 ----
basic-main.out: basic.cpp basic-main.cpp
	$(CXX20) -c basic.cpp -o basic.o
	$(CXX20) basic.o basic-main.cpp -o $@

# ---- implementation unit：接口只有声明，实现在另一个 TU ----
impl-main.out: impl.cpp impl-impl.cpp impl-main.cpp
	$(CXX20) -c impl.cpp -o impl.o
	$(CXX20) -c impl-impl.cpp -o impl-impl.o
	$(CXX20) impl.o impl-impl.o impl-main.cpp -o $@

# ---- partition：接口单元分片，primary 负责 re-export ----
partition-main.out: partition.cpp partition-circle.cpp partition-square.cpp partition-main.cpp
	$(CXX20) -c partition-circle.cpp -o partition-circle.o
	$(CXX20) -c partition-square.cpp -o partition-square.o
	$(CXX20) -c partition.cpp -o partition.o
	$(CXX20) partition.o partition-circle.o partition-square.o partition-main.cpp -o $@

# ---- header unit：import "header-unit.hpp" ----
header-unit-main.out: header-unit.hpp header-unit-main.cpp
	$(CXX20) -fmodule-header=user -c header-unit.hpp
	$(CXX20) header-unit-main.cpp -o $@

# ---- global module fragment + C++23 import std ----
gmf-main.out: gmf.cpp gmf-main.cpp gcm.cache/std.gcm
	$(CXX23) -c gmf.cpp -o gmf.o
	$(CXX23) std.o gmf.o gmf-main.cpp -o $@

# ---- C++23 import std ----
import-std.out: import-std.cpp gcm.cache/std.gcm
	$(CXX23) std.o import-std.cpp -o $@

# std 模块：编译 libstdc++ 自带的 bits/std.cc，产出 std.o 和 gcm.cache/std.gcm
gcm.cache/std.gcm: $(STD_CC)
	$(CXX23) -c $< -o std.o

clean:
	$(RM) $(OUTS) *.o
	$(RM) -r gcm.cache
