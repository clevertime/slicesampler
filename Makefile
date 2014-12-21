# Remove -D__MACOSX_CORE__ if you're not on OS X
CC=gcc -D__MACOSX_CORE__ -Wno-deprecated
FLAGS= -std=c99 -Wall
LIBS=
OBJS=

EXE=

SRCS = 

all: $(EXE)

$(EXE): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LIBS)

clean:
	rm -f *~ core $(EXE) *.o
	rm -rf $(EXE).dSYM 
