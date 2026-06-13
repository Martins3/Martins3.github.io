CC=gcc
all: mylib program1.c program2.c
	$(CC) -o program1.o -c program1.c
	$(CC) -o program3.o -c program3.c
	$(CC) -o program1.out program1.o  program3.o ./mylib.so
	$(CC) -o program2.out program2.c ./mylib.so


all2: mylib mylib2 program1.c program2.c
	$(CC) -o program1.out program1.c  ./mylib.so ./mylib2.so 
	$(CC) -o program2.out program2.c  ./mylib2.so ./mylib.so 

run: all
	@-./program1.out && ./program2.out

run2: all2
	@-./program1.out && ./program2.out

mylib:lib.c lib.h
	$(CC) -fPIC -shared -o mylib.so lib.c lib3.c

dump: mylib
	# objdump -S --disassemble mylib.so
	objdump -R mylib.so

mylib2:lib2.c lib2.h
	$(CC) -fPIC -shared -o mylib2.so lib2.c



test:lib.h program1.c lib.c
	$(CC) program1.c lib.c
