CC := gcc
CFLAGS := -O2 -pthread -Wall -Wextra

TARGETS := store_buffering.out message_passing.out release_acquire.out control_dependency.out

.PHONY: all clean test

all: $(TARGETS)

store_buffering.out: store_buffering.c
	$(CC) $(CFLAGS) -o $@ $<

message_passing.out: message_passing.c
	$(CC) $(CFLAGS) -o $@ $<

release_acquire.out: release_acquire.c
	$(CC) $(CFLAGS) -o $@ $<

control_dependency.out: control_dependency.c
	$(CC) $(CFLAGS) -o $@ $<

test: all
	@echo "=== Store Buffering Test ==="
	./store_buffering.out 10000000
	@echo ""
	@echo "=== Message Passing Test ==="
	./message_passing.out 1000000
	@echo ""
	@echo "=== Release-Acquire Chain Test ==="
	./release_acquire.out 1000000
	@echo ""
	@echo "=== Control Dependency Test ==="
	./control_dependency.out 1000000

clean:
	rm -f $(TARGETS)
