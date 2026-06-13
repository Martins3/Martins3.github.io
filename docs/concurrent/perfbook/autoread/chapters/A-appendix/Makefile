# Makefile for perfbook Appendix A demos
# Binary suffix .out for automatic gitignore compatibility

CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -pthread
TARGETS = after_demo.out after_demo_locked.out rcu_demo.out mb_demo.out

.PHONY: all clean test

all: $(TARGETS)

after_demo.out: after_demo.c
	$(CC) $(CFLAGS) -o $@ $<

after_demo_locked.out: after_demo_locked.c
	$(CC) $(CFLAGS) -o $@ $<

rcu_demo.out: rcu_demo.c
	$(CC) $(CFLAGS) -o $@ $<

mb_demo.out: mb_demo.c
	$(CC) $(CFLAGS) -o $@ $<

test: all
	@echo "=== Running after_demo.out ==="
	./after_demo.out || true
	@echo ""
	@echo "=== Running after_demo_locked.out ==="
	./after_demo_locked.out || true
	@echo ""
	@echo "=== Running rcu_demo.out ==="
	./rcu_demo.out
	@echo ""
	@echo "=== Running mb_demo.out ==="
	./mb_demo.out 100000

clean:
	rm -f $(TARGETS)
