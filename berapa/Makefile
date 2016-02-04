CC=g++
CFLAGS=-O3 -I../include -std=c++14 -lSDL2 -Wfatal-errors

all: berapa

debug: CFLAGS += -O0 -g
debug: berapa

berapa: berapa.cpp
	$(CC) berapa.cpp -o $@ $(CFLAGS)
clean:
	rm -f berapa berapa.o
