all:
	rm -f lib.a
	gcc -c simple-foo.c -o a.o
	gcc -c foo.c -o b.o
	gcc -c bar.c -o c.o
	ar rc lib.a b.o a.o c.o
	gcc main.c lib.a
	./a.out
