CXX ?= g++
CXXFLAGS := -std=c++17 -O0 -g -Wall -Wextra -Wpedantic
DEMOS := ownership.out unique-list.out shared-weak.out

.PHONY: all run
all: $(DEMOS)

%.out: %.cpp Makefile
	$(CXX) $(CXXFLAGS) $< -o $@

run: all
	./ownership.out
	./unique-list.out
	./shared-weak.out
