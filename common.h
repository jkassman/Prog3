#ifndef PROG3_COMMON_H
#define PROG3_COMMON_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <mhash.h>

#define PROG3_BUFF_SIZE 4096

int errorCheckRecv(int sock, void *data, size_t data_len, const char *errorMsg);
int errorCheckSend(int sock, void *data, size_t data_len, const char *errorMsg);
int errorCheckStrSend(int sock, char *stringToSend, const char *errorMsg);
int stringToInt(int *destInt,char *toConvert, int minVal, int maxVal);
void hashFile(unsigned char hash[], FILE* fileToHash);
int hashCompare(unsigned char *hash1, unsigned char *hash2);
void sendFile(int sock, FILE* file, unsigned int fileSize, 
              const char *programName);
void recvFile(int sock, FILE *f, unsigned int fileSize,
              const char *programName);
unsigned int getFileSize(FILE *f);

#endif
