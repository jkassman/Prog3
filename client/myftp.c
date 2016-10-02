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
    
    int bytesSent;
    //char buffy[PROG3_BUFFER_SIZE];
    char *buffy = "The right sequence of bytes";
    bytesSent = send(sock, buffy, strlen(buffy) + 1, 0);
    if (bytesSent < 0)
    {
        fprintf(stderr, "myftp Error: send(): %s\n", strerror(errno));
        close(sock);
        exit(3);
    }

    char recvBuffy[PROG3_BUFFER_SIZE];
    int bytesRecvd;
    bytesRecvd = recv(sock, recvBuffy, sizeof(recvBuffy), 0);
    if (bytesRecvd < 0)
    {
        fprintf(stderr, "myftp Error: recv(): %s\n", strerror(errno));
        close(sock);
        exit(3);
    }

    printf("Got %d bytes\n", bytesRecvd);
    int i;
    for (i = 0; i < bytesRecvd; i++)
    {
        printf("%c", recvBuffy[i]);
    }
    puts("");

    close(sock);
    exit(0);
}
