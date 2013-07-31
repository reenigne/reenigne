CC=cc
CFLAGS=-O2 -I../include -std=c++11

all: 8088
8088: 8088.o
	$(CC) -o 8088 8088.o
8088.o: 8088.cpp
	$(CC) $(CFLAGS) -c 8088.cpp
clean:
	rm -f 8088 8088.o
