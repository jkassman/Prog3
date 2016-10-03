myftp: myftp.c common.o
	gcc -Wall myftp.c common.o -o myftp
	
common.o: ../common.c ../common.h
	gcc -c ../common.c ../common.h -Wall -lmhash
clean:
	rm myftp common.o
