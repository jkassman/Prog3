#  Authors: Jacob Kassman, Lauren Kuta, Matt Paulson
#  netIDs: jkassman, lkuta, mpaulson
#  Computer Networks: Programming Assignment 3
#  Makefile 
#  Makefile for our server code.

myftpd: myftpd.c common.o
	gcc -Wall myftpd.c common.o -o myftpd -lmhash

common.o: ../common.c ../common.h
	gcc -c ../common.c ../common.h -Wall -lmhash

clean:
	rm myftpd common.o
