all:hello

hello: hello.o
	ld -o hello hello.o

hello.o:hello.asm
	nasm -f elf64 -o hello.o hello.asm

run:hello
	./hello

obj:hello
	objdump -D hello
