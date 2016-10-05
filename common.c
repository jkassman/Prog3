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

void sendFile(int sock, FILE* file, unsigned int fileSize, 
              const char *programName)
{
    char sendFileBuffer[PROG3_BUFF_SIZE];
    size_t bytesRead = 0;
    size_t bytesThisTime = 0;
    int status;

    while (bytesRead < fileSize) 
    {
        //Read up to the maximum buffer size from the file
        bytesThisTime = fread(sendFileBuffer, 1, PROG3_BUFF_SIZE, file);
        if (bytesThisTime == 0) 
        {
            fprintf(stderr, "%s Error: read file: %s\n", 
                    programName, strerror(errno));
            close(sock);
            exit(6);
        }
        bytesRead += bytesThisTime;

        //DEBUG PRINT
        printf("Read in %d bytes this time. bytesRead is %d and fileSize is %d\n", (int) bytesThisTime, (int) bytesRead, fileSize);

        //Send out the bytes read in this iteration to the client
        status = send(sock, sendFileBuffer, bytesThisTime, 0);
        if (status < 0) 
        {
            fprintf(stderr, "%s Error: send file: %s\n", 
                    programName, strerror(errno));
            close(sock);
            exit(5);
        }
    }
    //DEBUG PRINT
    puts("Finished sending the file!");
}


//returns -1 if failed to open the file to write.
int recvFile(int sock, FILE *f, unsigned int fileSize, 
              const char *programName)
{
    //Reads x number of bytes from server:
    char file[PROG3_BUFF_SIZE];
    int counter = 0;
    int fileRecvd;

    if(!f)
    {
        printf("Error opening file \n");
        return -1;
    }
    while(counter < fileSize){
        fileRecvd = recv(sock, file, PROG3_BUFF_SIZE, 0);
        if(fileRecvd < 0) 
        {
            fprintf(stderr, "%s Error: recv() file from server: %s\n", 
                    programName, strerror(errno));
            close(sock);
            exit(3);
        } 
        fwrite(file, 1, fileRecvd, f);  //write into file f and save
        counter = counter + fileRecvd;
    }
    fclose(f);
    return 0;
}

unsigned int getFileSize(FILE *f)
{
    unsigned int toReturn;
    fseek(f, 0, SEEK_END);
    toReturn = (unsigned int) ftell(f);
    rewind(f);
    
    return toReturn;
}
