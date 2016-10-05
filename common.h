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

void errorCheckRecv(int sock, void *data, size_t data_len, const char *errorMsg);
void errorCheckSend(int sock, void *data, size_t data_len, const char *errorMsg);
void errorCheckStrSend(int sock, char *stringToSend, const char *errorMsg);
void hashFile(unsigned char hash[], FILE* fileToHash);
void sendFile(int sock, FILE* file, unsigned int fileSize, 
              const char *programName);
int recvFile(int sock, FILE *f, unsigned int fileSize,
              const char *programName);
unsigned int getFileSize(FILE *f);

#endif
