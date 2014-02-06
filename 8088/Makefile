CC=g++
CFLAGS=-O3 -I../include -std=c++11 -lSDL2 -fpermissive

all: 8088

debug: CFLAGS += -O0 -g
debug: 8088

8088: 8088.cpp
	$(CC) 8088.cpp -o $@ $(CFLAGS)
clean:
	rm -f 8088 8088.o
