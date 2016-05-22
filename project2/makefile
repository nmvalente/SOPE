# Makefile for gerador and parque - SOPE

all: createdir bin/gerador bin/parque

createdir:
	mkdir -p bin/

bin/gerador: gerador.c
	gcc -Wall -o bin/gerador gerador.c

bin/parque: parque.c
	gcc -Wall -o bin/parque parque.c

trash:
	rm -rf *.o *.d

clean:
	rm -rf *.o *.d bin/gerador bin/parque
