uname_m := $(shell uname -m)

SRCS := $(wildcard ./**/*.cpp) $(wildcard ./*.cpp)
OUTS := $(SRCS:.cpp=.out)

all: $(OUTS)

ifeq ($(uname_m), aarch64)
ARCH_FLAGS := -march=armv8.3-a
endif

CPP_FLAGS=-Wall -std=c++20 -O3 -g
# CPP_FLAGS+=-fno-elide-constructors

stacktrace.out:stacktrace.cpp
	clang++ -rdynamic $(CPP_FLAGS) $(ARCH_FLAGS) -o $@ $^

%.out:%.cpp
	clang++ $(CPP_FLAGS) $(ARCH_FLAGS) -o $@ $^
clean:
	rm -f $(OUTS)
