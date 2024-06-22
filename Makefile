CC = g++
CFLAGS = -Wall -O3 -g3
LIBS = -lv4l2

OBJS = main.o encoder.o
DIR_LIB=-L/home/amither/Documents/v4l2-test/library/build
DIR_INC=

all: dev

dev: $(OBJS)
	$(CC) $(CFLAGS) -o dev main.cpp encoder.cpp encoder.hpp capture.hpp capture.cpp buffer.hpp utils.cpp utils.hpp -I/home/amither/Documents/v4l2-test/library/include $(LIBS) -lpthread $(DIR_LIB)

# work: $(OBJS)
# 	$(CC) $(CFLAGS) -o work work.cpp $(LIBS)

clean:
	rm -f main.o encoder.o dev 
