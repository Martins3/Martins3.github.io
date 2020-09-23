CFLAGS =  -Wall


	
# 重写隐式规则
%.out : %.c
	$(CC) $(CFLAGS) $< -o $@  $(LDLIBS)

A_EXE = a
B_EXE = b

# EXE = $($(A_EXE) $(B_EXE):out)
EXE = $(addsuffix .out,$(A_EXE) $(B_EXE))


and:
	echo and

x:
	echo "x "

all : ${EXE}
	echo $(EXE)
	echo $(A_EXE)


clean : 
	rm -f ${EXE} *.o

${EXE} :		# True as a rough approximation

