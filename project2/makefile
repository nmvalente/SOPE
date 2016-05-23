# Makefile for gerador and parque - SOPE

all: createdir bin/gerador bin/parque

createdir:
	mkdir -p bin/

bin/gerador: bin/gerador.o bin/utils.o
	gcc bin/gerador.o bin/utils.o -o bin/gerador -Wall -g -pthread

bin/parque: bin/parque.o bin/utils.o
	gcc bin/parque.o bin/utils.o -o bin/parque -Wall -g -pthread

bin/gerador.o: gerador.c utils.h
	gcc -c -Wall -g gerador.c -o bin/gerador.o -pthread

bin/parque.o: parque.c utils.h
	gcc -c -Wall -g parque.c -o bin/parque.o -pthread

bin/utils.o: utils.c utils.h
	gcc -c -Wall -g utils.c  -o bin/utils.o -pthread

trash:
	rm -rf bin/parque.o bin/gerador.o bin/utils.o

clean:
	rm -rf bin
