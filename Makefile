# Makefile for rmdup and lsdir - SOPE

all: createdir bin/lsdir bin/rmdup

createdir: 
	mkdir -p bin/
	
bin/lsdir: lsdir.c
	gcc -Wall -o bin/lsdir lsdir.c

bin/rmdup: rmdup.c
	gcc -Wall -o bin/rmdup rmdup.c

trash:
	rm -rf *.o *.d

clean:
	rm -rf *.o *.d bin/lsdir bin/rmdup
