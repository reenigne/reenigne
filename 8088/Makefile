CC=g++
CFLAGS=-O2 -I../include -std=c++11

debug: CFLAGS += -O0 -g
debug: 8088

all: 8088
8088: 8088.cpp
	$(CC) $(CFLAGS) 8088.cpp -o $@
clean:
	rm -f 8088 8088.o
