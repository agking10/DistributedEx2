CC=gcc
CXX=g++

CFLAGS = -g -c -Wall -pedantic
CPPFLAGS = -std=c++17

MCAST_OBJ = mcast.o machine.o recv_dbg.o messages.o
START_MCAST_OBJ = start_mcast.o recv_dbg.o messages.o

all: test start_mcast mcast

test: test.o recv_dbg.o
	    $(CC) -o test test.o recv_dbg.o

start_mcast: $(START_MCAST_OBJ)
		$(CXX) -o start_mcast $(START_MCAST_OBJ)

mcast: $(MCAST_OBJ)
		$(CXX) -o mcast $(MCAST_OBJ)

clean:
	rm *.o
	rm test
	rm mcast
	rm start_mcast

%.o:    %.c
	$(CC) $(CFLAGS) $*.c

%.o:	%.cpp
	$(CXX)  $(CFLAGS) $*.cpp

%.o:	%.hpp
	$(CXX) $(CPPFLAGS) $(CFLAGS) $*.hpp
