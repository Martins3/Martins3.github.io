.DEFAULT_GOAL := compare

CXX ?= g++
CXXFLAGS := -std=c++17 -O1 -Wall -Wextra -Wpedantic

SOURCE := rvo-nrvo.cpp
DEFAULT_OUT := rvo-nrvo.out
NO_ELIDE_OUT := rvo-nrvo-no-elide.out
FORWARD_SOURCE := forward.cpp
FORWARD_OUT := forward.out

.PHONY: build clean compare run-default run-forward run-no-elide

build: $(DEFAULT_OUT) $(NO_ELIDE_OUT)

$(DEFAULT_OUT): $(SOURCE) Makefile
	$(CXX) $(CXXFLAGS) $< -o $@

$(NO_ELIDE_OUT): $(SOURCE) Makefile
	$(CXX) $(CXXFLAGS) -fno-elide-constructors $< -o $@

$(FORWARD_OUT): $(FORWARD_SOURCE) Makefile
	$(CXX) $(CXXFLAGS) $< -o $@

run-default: $(DEFAULT_OUT)
	./$(DEFAULT_OUT)

run-no-elide: $(NO_ELIDE_OUT)
	./$(NO_ELIDE_OUT)

run-forward: $(FORWARD_OUT)
	./$(FORWARD_OUT)

compare: build
	@echo "===== C++17 default: optional NRVO enabled ====="
	./$(DEFAULT_OUT)
	@echo
	@echo "===== C++17 -fno-elide-constructors ====="
	./$(NO_ELIDE_OUT)

clean:
	$(RM) $(DEFAULT_OUT) $(NO_ELIDE_OUT) $(FORWARD_OUT)
