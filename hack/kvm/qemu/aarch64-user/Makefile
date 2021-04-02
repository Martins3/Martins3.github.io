dyn:
	aarch64-linux-gnu-gcc -o hello64dyn.out hello64.c 
	qemu-aarch64 -L /usr/aarch64-linux-gnu ./hello64dyn.out

static:
	aarch64-linux-gnu-gcc -static -o hello64.out hello64.c
	./hello64.out
