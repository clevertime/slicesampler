# Remove -D__MACOSX_CORE__ if you're not on OS X
CC=gcc -D__MACOSX_CORE__ -Wno-deprecated
FLAGS= -std=c99 -Wall
LIBS=-framework OpenGL -framework GLUT -lportaudio -lsndfile 
OBJS=slicesampler.o 

EXE=slicesampler

SRCS = slicesampler.c fft.c

all: $(EXE)

$(EXE): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LIBS)

clean:
	rm -f *~ core $(EXE) *.o
	rm -rf $(EXE).dSYM 
