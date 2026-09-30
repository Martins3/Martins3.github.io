SRC := actress.cpp
OUT := actress.out

CXX := g++
CXXFLAGS := -Wall -std=c++20 -g

all: $(OUT)
	./$(OUT)

$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

# clang -cc1 直接进入 clang 前端, 不经过 driver, 因此没有链接步骤。
# actress.cpp 刻意不含头文件, 所以这里不需要 -I 指定标准库路径。
dump: $(SRC)
	@echo "== clang -cc1 -fdump-record-layouts =="
	clang -cc1 -fdump-record-layouts $<
	@echo
	@echo "== g++ -fdump-lang-class =="
	$(CXX) $(CXXFLAGS) -fdump-lang-class -c $< -o /dev/null
	@echo "g++ 的 dump 写入 $<.001l.class"

clean:
	rm -f $(OUT) *.o *.class
