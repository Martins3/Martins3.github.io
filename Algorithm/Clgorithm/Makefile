CFLAGS=-Wall -std=c99
CC=clang

JUNK :=  $(shell find . -type f -name '*.o')
EXE :=  $(shell find . -type f -name '*.out')

# 窒息的操作, 这他妈和直接写有什么区别
algorithm.out:
	$(CC) -o BinHeap.o -c heap/BinHeap.c $(CFLAGS)
	$(CC) -o main.o -c main.c $(CFLAGS)
	$(CC) -o $@ main.o BinHeap.o $(CFLAGS)

run:
	./algorithm.out

.PHONY: clean
clean:
	rm -f $(JUNK)
	rm -f $(EXE)
