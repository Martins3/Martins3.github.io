CXX = /usr/bin/g++
# Keep the system compiler/linker together even inside this repository's Nix shell.
CXXFLAGS ?= -B/usr/bin/ -O3 -g -std=c++20 -march=native -Wall -Wextra -Wpedantic

.PHONY: all smoke stream interleave
all: microbench.out stream.out interleave.out
stream: stream.out
interleave: interleave.out

microbench.out: microbench.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ -pthread

stream.out: stream.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ -pthread

interleave.out: interleave.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ -pthread

smoke: stream.out microbench.out
	./microbench.out --quick --samples 1
