CC = gcc
FLAGS = -g -Wall
OBJ = helper.o

psort : psort.o ${OBJ}
	${CC} ${FLAGS} -o $@ $< ${OBJ}

all: psort mkwords showwords

mkwords : mkwords.o ${OBJ}
	${CC} ${FLAGS} -o $@ $< ${OBJ} -lm

showwords : showwords.o ${OBJ}
	${CC} ${FLAGS} -o $@ $< ${OBJ}

# How to create .o files (which are dependencies in the previous 
# rule). The variable $< has the value of the .c file that it found.  

%.o : %.c helper.h
	${CC} ${FLAGS} -c $<

# Keep the directory clean.
clean :
	-rm *.o
