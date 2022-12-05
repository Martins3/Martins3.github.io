CC=gcc

link:a.o b.o
	gcc a.o b.o -o ab.out

mergeSection:
	objdump -h a.o
	objdump -h b.o
	objdump -h ab.out

common:b.o
	readelf -s b.o



clean:
	/bin/rm -f a.o b.o ab.out


.o:.c
	${CC} -c $< -o $@

