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
#include "../common.h"

#define PROG3_BUFFER_SIZE 4096

#define PROG3_SEND_OP_ERR "myftp: send opcode"
#define PROG3_SEND_NUM_ERR "myftp: send number error"
#define PROG3_SEND_STR_ERR "myftp: send string error"
/*
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
*/
int clientRequest(int sock)
{
    errorCheckStrSend(sock, "REQ", PROG3_SEND_OP_ERR);
    //Receive query from the server
    char recvQuery[PROG3_BUFFER_SIZE];
    unsigned short int fileNameLen;
    unsigned short int fileNameLenToSend;
    char fileName[1000];
    unsigned char hash[16];
    unsigned char recvdHash[16];
    unsigned int fileLenBuffy;
    int status;
    int bytesRecvd;
    int fileLenRecvd, hashRecvd; // fileRecvd;
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
    errorCheckStrSend(sock, fileName, "myftp: send filename");
    
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

    //Receieve a file from the serer:
    FILE *f = fopen(fileName, "w");
    if(!f){
     printf("Error opening file \n");
     return 0;
    }
    recvFile(sock, f, fileLenBuffy, "myftp");

    //Computes the MD5 hash of recieved file: 
    fclose(f);
    f = fopen(fileName, "r");
    hashFile(recvdHash, f);
    fclose(f);
   
    int k;
    printf("New File Hash Value: ");
    for(k = 0; k < 16; k++) {
      printf("%02x.", recvdHash[k]);
    }
    printf("\n");

    //Compares the two hashes:
    if(!hashCompare(hash, recvdHash)) {
      printf("The hashes match!\n");
    }else{
      printf("The hashes do not match...\n");
    }

    return 0;
}

int clientUpload(int sock)
{
    char fileName[100];
    unsigned int fileSize;
    FILE *fileToSend;
    short int fileNameSize;

    //Send Op Code to server:
    errorCheckStrSend(sock, "UPL", PROG3_SEND_OP_ERR);

    //Getting the file name from the user:
    printf("Please enter the name of the file you would like to upload: ");
    fgets(fileName, 100, stdin);
    
    //Checking to see if the file exists:
    if (access(fileName, F_OK) < 0) {
      //not a file, send -1;
      fileSize = -1;
      fileSize = htonl(fileSize);
    fileToSend = fopen(filename, "r");
    if (!fileToSend) {
      perror("myftpd: fopen()");
      exit(6);
    }
    fileSize = getFileSize(fileToSend);
    fileNameSize = strlen(fileName);
   
    //Sending over the length of the file name followed by the file name:
    
    errorCheckSend(sock, (char*)&fileNameSize, 2, PROG3_SEND_NUM_ERR);
    errorCheckStrSend(sock, fileName, PROG3_SEND_STR_ERR);

    //Receives acknowledgement from the server:

    //Sends over the file size (32-bit value):
   
    //Sends over actual file to server:

    //Computes MD5 Hash and sends it as 16-byte string:

    //return to "prompt user for operation" state:
  }  
    return 0;
}

int clientDelete(int sock)
{
    errorCheckStrSend(sock, "DEL", PROG3_SEND_OP_ERR);
    return 0;
}

int clientList(int sock)
{
    errorCheckStrSend(sock, "LIS", PROG3_SEND_OP_ERR);
    return 0;
}

int clientMkdir(int sock)
{
    errorCheckStrSend(sock, "MKD", PROG3_SEND_OP_ERR);
    return 0;
}

int clientRmdir(int sock)
{
    errorCheckStrSend(sock, "RMD", PROG3_SEND_OP_ERR);
    return 0;
}

int clientCd(int sock)
{
    errorCheckStrSend(sock, "CHD", PROG3_SEND_OP_ERR);
    return 0;
}

void clientExit(int sock)
{
    errorCheckStrSend(sock, "XIT", PROG3_SEND_OP_ERR);
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
