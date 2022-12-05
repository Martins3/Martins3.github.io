CC= gcc
AR=ar
SRCS = malloc.c entry.c printf.c string.c io.c
OBJ = ${SRCS:.c=.o} 

CRT=crt.a
CFLAGS= -g -fno-builtin -nostdlib -fno-stack-protector

${CRT}: ${OBJ}
	${AR} -rs $@ $^

%.o: %.c
	${CC} ${CFLAGS} -c $< -o $@

depend:
	makedepend -- $(CFLAGS) -- $(SRCS) test.c


clean:
	rm -f ${CRT} test.out $(OBJ) test.o

test:test.out
	./test.out

test.out: test.o crt.a 
	ld -static -e mini_crt_entry $^ -o test.out


# DO NOT DELETE

malloc.o: minicrt.h
entry.o: minicrt.h
printf.o: minicrt.h
string.o: minicrt.h
io.o: minicrt.h
test.o: minicrt.h
