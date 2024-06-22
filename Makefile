CC = g++
CFLAGS = -Wall -O3 -g3
LIBS = -lv4l2

OBJS = main.o encoder.o

all: dev work

dev: $(OBJS)
	$(CC) $(CFLAGS) -o dev main.cpp encoder.cpp encoder.hpp capture.hpp capture.cpp buffer.hpp utils.cpp utils.hpp $(LIBS) -lpthread

work: $(OBJS)
	$(CC) $(CFLAGS) -o work work.cpp $(LIBS)

clean:
	rm -f main.o encoder.o dev work 
