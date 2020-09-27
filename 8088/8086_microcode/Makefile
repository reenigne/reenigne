#!/usr/bin/make -f
EXEC = 8086_microcode.o

%.o: %.cpp

	g++ -std=c++0x -o $@ $<

test: $(EXEC)
	./$(EXEC) | diff -u0 --strip-trailing-cr 8086_microcode/microcode_8086.txt - 

clean:
	-rm $(EXEC)

all: test

.PHONY: test clean all
