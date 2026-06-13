.PHONY: all run1 run2
all: Program1 Program2

Lib.so: Lib.c
	gcc -fPIC -shared -o Lib.so Lib.c

Program1: Program1.c Lib.so
	gcc -o Program1 Program1.c ./Lib.so

Program2: Program2.c Lib.so
	gcc -o Program2 Program2.c ./Lib.so

run1:
	./Program1

run2:
	./Program2




