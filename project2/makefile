# Makefile for gerador and parque - SOPE

all: createdir bin/gerador bin/parque

createdir:
	mkdir -p bin/

bin/gerador: bin/gerador.o bin/utils.o
	gcc bin/gerador.o bin/utils.o -o bin/gerador -Wall

bin/parque: bin/parque.o bin/utils.o
	gcc bin/parque.o bin/utils.o -o bin/parque -Wall

bin/gerador.o: gerador.c utils.h
	gcc -c -Wall -pthread gerador.c -o bin/gerador.o

bin/parque.o: parque.c utils.h
	gcc -c -Wall -pthread parque.c -o bin/parque.o

bin/utils.o: utils.c utils.h
	gcc -c -Wall -pthread utils.c  -o bin/utils.o

trash:
	rm -rf bin/parque.o bin/gerador.o bin/utils.o

clean:
	rm -rf bin
