/*
  Authors: Jacob Kassman, Lauren Kuta, Matt Paulson
  netIDs: jkassman, lkuta, mpaulson
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>
#include <netdb.h>

#define PROG3_BUFFER_SIZE 4096

/* stringToInt()
 Convert the string to a long in base 10 using strtol,
 If out of bounds, return an error: 1 if too low, 2 if too high.
 If any unrecognized characters, return 3.
 if successful, return 0.
*/
int stringToInt(int *destInt,char *toConvert, int minVal, int maxVal)
{
    char *errorChecker = NULL;
    long long veryLong = strtol(toConvert, &errorChecker, 10);
    if (errorChecker != NULL && *errorChecker != '\0')
    { //unrecognized character
        return 3;
    }
    if (veryLong > maxVal)
    { 
        return 2;
    }
    else if (veryLong < minVal)
    {
        return 1;
    }
    *destInt = (int) veryLong;
    return 0;
}

void sendOpCode(const char *opcode, int sock)
{
    //Send opcode to the server
    int status;
    status = send(sock, opcode, strlen(opcode) + 1, 0);
    if (status < 0)
    {
        fprintf(stderr, "myftp error sending opcode: %s\n", strerror(errno));
        close(sock);
        exit(3);
    }
}

int clientRequest(int sock)
{
    sendOpCode("REQ", sock);
    //Receive query from the server
    char recvQuery[PROG3_BUFFER_SIZE];
    unsigned short int fileNameLen;
    unsigned short int fileNameLenToSend;
    char fileName[1000];
    unsigned char hash[16];
    unsigned int fileLenBuffy;
    int status;
    int bytesRecvd;
    int fileLenRecvd, hashRecvd, fileRecvd;
    bytesRecvd = recv(sock, recvQuery, sizeof(recvQuery), 0);
    if (bytesRecvd < 0)
    {
        fprintf(stderr, "myftp Error: recv(): %s\n", strerror(errno));
        close(sock);
        exit(3);
    }

    //print out received query:
    int i;
    for (i = 0; i < bytesRecvd; i++)
    {
        printf("%c", recvQuery[i]);
    }
    puts(""); 

    //Sends file name and length:
    fgets(fileName, 1000, stdin);
    fileNameLen = strlen(fileName);
    fileName[(strlen(fileName)-1)] = '\0';
    fileNameLenToSend = htons(fileNameLen);

    status = send(sock, (char *)&fileNameLenToSend,2, 0);
    if (status < 0)
    {
        fprintf(stderr, "myftp error sending file name length for REQ: %s\n", strerror(errno));
        close(sock);
        exit(3);
    }
    sendOpCode(fileName, sock);
    
    //Receives 32-bit file length:
    fileLenRecvd = recv(sock, (char *)&fileLenBuffy, 4, 0);
    if(fileLenRecvd < 0) 
    {
       fprintf(stderr, "myftp Error: recv() 32-bit file length: %s\n", strerror(errno));
        close(sock);
        exit(3);
    }

    printf("File length: %i \n", fileLenBuffy);

    //Decode 32-bit value:
    if(fileLenBuffy == -1) {
      printf("File does not exist on server.\n");
      return 0;
    }

    //Receives MD5 Hash:
    hashRecvd = recv(sock, hash, 16, 0);
    if(hashRecvd < 0) 
    {
       fprintf(stderr, "myftp Error: recv() MD5 hash: %s\n", strerror(errno));
        close(sock);
        exit(3);
    }
  
    int j;
    printf("Hash value: \n");
    for( j = 0; j < 16; j++) {
      printf("%02x.", hash[j]);
    }
    printf("\n");

    //Reads x number of bytes from server:
    char file[fileLenBuffy];
    int counter = 0;
    FILE *f = fopen(fileName, "w");
    if(!f){
     printf("Error opening file \n");
     return 0;
    }
    while(counter < fileLenBuffy){
      fileRecvd = recv(sock, file, fileLenBuffy, 0);
      if(fileRecvd < 0) 
      {
       fprintf(stderr, "myftp Error: recv() file from server: %s\n", strerror(errno));
        close(sock);
        exit(3);
      } 
      fwrite(file, 1, fileRecvd, f);  //write into file f and save
      counter = counter + fileRecvd;
    }
    fclose(f);

    //Computes the MD5 hash of recieved file: 

    return 0;
}

int clientUpload(int sock)
{
    sendOpCode("UPL", sock);
    return 0;
}

int clientDelete(int sock)
{
    sendOpCode("DEL", sock);
    return 0;
}

int clientList(int sock)
{
    sendOpCode("LIS", sock);
    return 0;
}

int clientMkdir(int sock)
{
    sendOpCode("MKD", sock);
    return 0;
}

int clientRmdir(int sock)
{
    sendOpCode("RMD", sock);
    return 0;
}

int clientCd(int sock)
{
    sendOpCode("CHD", sock);
    return 0;
}

void clientExit(int sock)
{
    sendOpCode("XIT", sock);
    printf("The session has been closed\n");
}
 
int main(int argc, char **argv)
{
    int status;
    if (argc != 3)
    {
        fprintf(stderr, "Wrong number of arguments.\n"
                "Proper usage: myftp Server_Name Port\n");
        exit(1);
    }
    
    //grab arguments
    char *hostname = argv[1];
    char *portNumberStr = argv[2];
    int portNumber;

    //Parse the port
    status = stringToInt(&portNumber, portNumberStr, 0, INT_MAX);
    if (status == 1)
    {
        fprintf(stderr, "Error: Negative port value.\n");
    }
    else if (status == 2)
    {
        fprintf(stderr, "Error: Port value out of integer range!\n");
    }
    else if (status == 3)
    {
        fprintf(stderr, "Error: Unrecognized characters in port (%s)\n", 
                portNumberStr);
    }
    if (status) exit(1);

    //create a socket
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        fprintf(stderr, "myftp Error: socket(): %s\n", strerror(errno));
        exit(2);
    }
    
    //Create the TCP Server's address structure
    //Get Ip Address from name
    struct hostent *serverIp;
    serverIp = gethostbyname(hostname);
    if (!serverIp)
    {
        fprintf(stderr, "myftp Error: Unknown Host %s\n", hostname);
        close(sock);
        exit(2);
    }

    //Complete Address Structure
    struct sockaddr_in sinbad;
    bzero(&sinbad, sizeof(sinbad));
    sinbad.sin_family = AF_INET;
    bcopy(serverIp->h_addr, (char *) &sinbad.sin_addr, serverIp->h_length);
    sinbad.sin_port = htons(portNumber);


    //connect to the TCP Server
    status = connect(sock, (struct sockaddr*) &sinbad, sizeof(sinbad));
    if (status < 0)
    {
        fprintf(stderr, "myftp Error: connect(): %s\n", strerror(errno));
        close(sock);
        exit(2);
    }   

    //Get input from user
    char operation[16];
    int loopExit = 0;
    while (!loopExit)
    {
        fgets(operation, 16, stdin);
        //Decide which operation to do
        if (!strcmp(operation, "REQ\n"))
        {
            clientRequest(sock);
        }
        else if (!strcmp(operation, "UPL\n"))
        {
            clientUpload(sock);
        }
        else if (!strcmp(operation, "DEL\n"))
        {
            clientDelete(sock);
        }
        else if (!strcmp(operation, "LIS\n"))
        {
            clientList(sock);
        }
        else if (!strcmp(operation, "MKD\n"))
        {
            clientMkdir(sock);
        }
        else if (!strcmp(operation, "RMD\n"))
        {
            clientRmdir(sock);
        }
        else if (!strcmp(operation, "CHD\n"))
        {
            clientCd(sock);
        }
        else if (!strcmp(operation, "XIT\n"))
        {
            clientExit(sock);
            loopExit = 1;
        }
        else
        {
            fprintf(stderr, "Unrecognized operation code: %s\n", operation);
        }
    }
    
    close(sock);
    exit(0);
}
