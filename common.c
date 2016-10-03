#include "common.h"

void errorCheckRecv(int sock, void *data, size_t data_len, const char *errorMsg) 
{
    int status;
    status = recv(sock, (char*) data, data_len, 0);
    if (status < 0) 
    {
        perror(errorMsg);
        close(sock);
        exit(8);
    }
}

void errorCheckSend(int sock, void *data, size_t data_len, const char *errorMsg)
{
    int status;
    status = send(sock, (char*) data, data_len, 0);
    if (status < 0)
    {
        perror(errorMsg);
        close(sock);
        exit(9);
    }
}

void errorCheckStrSend(int sock, char *stringToSend, const char *errorMsg)
{
    int status;
    status = send(sock, stringToSend, strlen(stringToSend) + 1, 0);
    if (status < 0)
    {
        perror(errorMsg);
        close(sock);
        exit(10);
    }
}

void hashFile(unsigned char hash[], FILE* fileToHash)
{
    rewind(fileToHash);
    //find the MD5 hash of the file:
    MHASH td;
    unsigned char buffer;
    td = mhash_init(MHASH_MD5);
    if (td == MHASH_FAILED) 
    {
        perror("myftpd: mhash_init()");
        exit(4);
    }
  
    while (fread(&buffer, 1, 1, fileToHash) == 1) 
    {
        mhash(td, &buffer, 1);
    }
  
    //finalize hash generation
    mhash_deinit(td, hash);
    rewind(fileToHash);
}
