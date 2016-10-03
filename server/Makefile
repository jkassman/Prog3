myftpd: myftpd.c common.o
	gcc -Wall myftpd.c common.o -o myftpd -lmhash

common.o: ../common.c ../common.h
	gcc -c ../common.c -Wall -lmhash

clean:
	rm myftpd common.o
