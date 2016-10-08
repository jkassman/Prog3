#  Authors: Jacob Kassman, Lauren Kuta, Matt Paulson
#  netIDs: jkassman, lkuta, mpaulson
#  Computer Networks: Programming Assignment 3
#  Makefile 
#  Makefile for our client code.

myftp: myftp.c common.o
	gcc -Wall myftp.c common.o -o myftp -lmhash
	
common.o: ../common.c ../common.h
	gcc -c ../common.c ../common.h -Wall -lmhash
clean:
	rm myftp common.o
