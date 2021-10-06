CC=gcc
CXX=g++

CFLAGS = -g -c -Wall -pedantic
CPPFLAGS = -std=c++17

all: test

test: test.o recv_dbg.o
	    $(CC) -o test test.o recv_dbg.o  

clean:
	rm *.o
	rm test

%.o:    %.c
	$(CC) $(CFLAGS) $*.c

%.o:	%.cpp
	$(CXX)  $(CFLAGS) $*.cpp

%.o:	%.hpp
	$(CXX) $(CPPFLAGS) $(CFLAGS) $*.hpp
