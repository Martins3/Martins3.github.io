CXX ?= g++
CXXFLAGS ?= -std=c++17 -O0 -g -Wall -Wextra -Wpedantic

.DEFAULT_GOAL := build
.PHONY: build run clean

build: basic.out adl.out counter.out

basic.out: basic.cpp Makefile
	$(CXX) $(CXXFLAGS) $< -o $@

adl.out: adl.cpp Makefile
	$(CXX) $(CXXFLAGS) $< -o $@

counter.out: counter.cpp counter-main.cpp counter.hpp Makefile
	$(CXX) $(CXXFLAGS) counter.cpp counter-main.cpp -o $@

run: build
	./basic.out
	./adl.out
	./counter.out

clean:
	$(RM) basic.out adl.out counter.out
