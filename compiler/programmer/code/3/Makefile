section:sp.o
	objdump -h sp.o

allSection:sp.o
	readelf -S sp.o

codeSec:sp.o
	objdump -x -s -d sp.o

exeSection:sp.out
	objdump -h sp.out

strtab:sp.o
	objdump  sp.o
	
	
sp.o:simpleSection.c
	gcc -c simpleSection.c -o sp.o

sp.out:simpleSection.c
	gcc simpleSection.c -o sp.out
