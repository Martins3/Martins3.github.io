LIBDIR = /home/maritns3/core/capstone/
INC = /home/maritns3/core/capstone/include
LIBNAME = capstone

all:
	${CC} -c -I$(INC) -DCAPSTONE_HAS_X86 test.c -o test.o
	${CC} test.o -Wall -L$(LIBDIR) -l$(LIBNAME) -o test.out
	${CC} test.o -Wall -l$(LIBNAME) -o sys-test.out
