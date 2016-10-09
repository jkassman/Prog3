/*
  Authors: Jacob Kassman, Lauren Kuta, Matt Paulson
  netIDs: jkassman, lkuta, mpaulson
  Computer Networks: Programming Assignment 3
  myftp.c
  Creates a client and allows the client to execute a variety of functions.  
 */

#include "../common.h"

#define PROG3_SEND_OP_ERR "myftp: send opcode"

int clientRequest(int sock)
{
    errorCheckStrSend(sock, "REQ", PROG3_SEND_OP_ERR);

    //Receive query from the server
    char recvQuery[PROG3_BUFF_SIZE];
    unsigned short int fileNameLen;
    unsigned short int fileNameLenToSend;
    char fileName[1000];
    unsigned char hash[16];
    unsigned char recvdHash[16];
    unsigned int fileLenBuffy;
    int bytesRecvd;
    struct timeval tvalBefore, tvalAfter;
    double transferTime, throughput;

    bytesRecvd = errorCheckRecv(sock, recvQuery, sizeof(recvQuery), 
                                "myftp: query recv()");

    //print out received query:
    int i;
    for (i = 0; i < bytesRecvd; i++)
    {
        printf("%c", recvQuery[i]);
    }
    puts(""); 

    //Get file name and length:
    fgets(fileName, 1000, stdin);
    fileNameLen = strlen(fileName);
    fileName[(strlen(fileName)-1)] = '\0';
    fileNameLenToSend = htons(fileNameLen);

    //Sends file name length and file name:
    errorCheckSend(sock, &fileNameLenToSend, 2, 
                   "myftp: REQ: send file name length");
    errorCheckStrSend(sock, fileName, "myftp: send filename");
    
    //Receive 32-bit file size:
    errorCheckRecv(sock, &fileLenBuffy, 4,
                   "myftp: recv() 32-bit file length");

    printf("File length: %i \n", fileLenBuffy);

    //Decode 32-bit value:
    if(fileLenBuffy == -1) {
      printf("File does not exist on server.\n");
      return 0;
    }

    //Receives MD5 Hash:
    errorCheckRecv(sock, hash, 16, "myftp: recv() MD5 hash");
  
    int j;
    printf("Hash value: \n");
    for( j = 0; j < 16; j++) {
      printf("%02x", hash[j]);
    }
    printf("\n");

    //Receieve a file from the server:
    FILE *f = fopen(fileName, "w");
    if(!f){
     printf("Error opening file \n");
     return 0;
    }
    gettimeofday(&tvalBefore, NULL);
    recvFile(sock, f, fileLenBuffy, "myftp");
    gettimeofday(&tvalAfter, NULL);

    //Computes the MD5 hash of recieved file: 
    fclose(f);
    f = fopen(fileName, "r");
    hashFile(recvdHash, f);
    fclose(f);
   
    int k;
    printf("New File Hash Value: ");
    for(k = 0; k < 16; k++) {
      printf("%02x", recvdHash[k]);
    }
    printf("\n");

    //Calculate throughput of request process:
    transferTime = (tvalAfter.tv_sec - tvalBefore.tv_sec) + 1e-6*(tvalAfter.tv_usec - tvalBefore.tv_usec);
    throughput = ((fileLenBuffy/transferTime)/1e6);

    //Compares the two hashes:
    if(!hashCompare(hash, recvdHash)) {
      printf("The hashes match!\n");
      printf("%d bytes transfered in %lf seconds: %lf Megabytes/sec \n", fileLenBuffy, transferTime, throughput);
    }else{
      printf("The hashes do not match...\n");
    }

    return 0;
}

//returns 1 if invalid filename exists.
int clientUpload(int sock)
{
    char fileName[100];
    char firstAck[100];
    unsigned char hash[16];
    unsigned int fileSize;
    FILE *fileToSend;
    unsigned int fileSizeToSend;
    short int fileNameSize;
    //int hashMash;
    char throughputMess[100];

    //Getting the file name from the user:
    printf("Please enter the name of the file you would like to upload: ");
    fgets(fileName, 100, stdin);
    printf("File Name: %s \n", fileName);
    fileName[(strlen(fileName)-1)] = '\0';

    //Checking to see if the file exists:
    if (access(fileName, F_OK) < 0) {
      //if file does not exist...
       perror("myftp()");
       return 1;
    } else { 
       fileToSend = fopen(fileName, "r");
      if (!fileToSend) {
        perror("myftp() ");
        close(sock);
        exit(6);
       }
    }

    //Send Op Code to server:
    errorCheckStrSend(sock, "UPL", PROG3_SEND_OP_ERR);
    
    fileSize = getFileSize(fileToSend);
    fileSizeToSend = htonl(getFileSize(fileToSend));
    printf("File Size: %i \n", fileSize);
    fileNameSize = htons(strlen(fileName)+1);
    printf("FileNameSize: %i \n", fileNameSize);
   
    //Sending over the length of the file name followed by the file name:
    
    errorCheckSend(sock, &fileNameSize, 2, "myftp: sending length of file name");
    errorCheckStrSend(sock, fileName, "myftp: sending file name");

    //Receives acknowledgement from the server:

    errorCheckRecv(sock, firstAck, sizeof(firstAck), "myftp: receiving first ack");
    printf("%s \n", firstAck);

    //Sends over the file size (32-bit value):

    errorCheckSend(sock, &fileSizeToSend, 4, "myftp: sending file size");
   
    //Sends over actual file to server:

    sendFile(sock, fileToSend, fileSize, "myftp:");

    //Computes MD5 Hash and sends it as 16-byte string:

    hashFile(hash, fileToSend);
    fclose(fileToSend);

    int k;
    printf("File's Hash Value: ");
    for(k = 0; k < 16; k++) {
      printf("%02x.", hash[k]);
    }
    printf("\n");

   // errorCheckStrSend(sock, (char*)hash, "myftp:");
   errorCheckSend(sock, &hash, 16, "myftp: ");

   //Check to see if the hashes match:
   /* errorCheckRecv(sock, &hashMash, 2, "myftp: ");
   if(hashMash == -1) {
     printf("The hashes do not match! \n");
     return 0;
   }else{
     printf("The hashes matched! \n"); */

     errorCheckRecv(sock, &throughputMess, 100, "myftp: ");
     //throughput = ((fileSize/transferTime)/1e6);
     //transferTime = (fileSize/(throughput*1e6));
     
     printf("%s\n", throughputMess);
  // }
     
   //return to "prompt user for operation" state:
 
    return 0;
}

int clientList(int sock)
{
    //send opcode
    errorCheckStrSend(sock, "LIS", PROG3_SEND_OP_ERR);

    //receive file size
    int fileSize;
    errorCheckRecv(sock, &fileSize, 4, "myftp: LIS: recv() file size");
    fileSize = ntohl(fileSize);
    
    //Make a temporary file:
    char filename[32];
    strcpy(filename, "/tmp/prog3LIS_XXXXXX");
    int tempFd = mkstemp(filename);
    FILE* tempF = fdopen(tempFd, "w");
    
    //actually receive the file
    recvFile(sock, tempF, fileSize, "myftp");
    
    //print out all files in the listings file
    char sysCommand[64];
    fflush(tempF);
    sprintf(sysCommand, "cat %s", filename);
    int status;
    status = system(sysCommand);
    if (status == -1)
    {
        perror("myftp: display LIS file");
        exit(42);
    }

    //Remove the temporary file
    fclose(tempF);
    sprintf(sysCommand, "rm %s", filename);
    status = system(sysCommand);
    if (status == -1)
    {
        perror("myftp: delete LIS file");
        exit(42);
    }
    
    return 0;
}

/* The beginning of RMD, DEL, and MKD is the same, so it is encapsulated here */
//Operation is the code (e.g. MKD)
//userMessage is how to prompt the user.
void startClientDir(int sock, char *operation, char *userMessage)
{
    errorCheckStrSend(sock, operation, PROG3_SEND_OP_ERR);
    unsigned short int dirNameLen;
    unsigned short int dirNameLenToSend;
    char dirname[1000];
    //Grab user input
    printf("%s", userMessage);
    dirNameLen = getNameFromUser(dirname, 1000);
    
    //send the file name length
    dirNameLenToSend = htons(dirNameLen);
    char errorStr[80];
    sprintf(errorStr, "myftp: %s: send() file name length", operation);
    errorCheckSend(sock, &dirNameLenToSend, 2, errorStr);

    //send the file name:
    sprintf(errorStr, "myftp: %s: send() filename", operation);
    errorCheckStrSend(sock, dirname, errorStr);
}

int clientMkdir(int sock)
{
    startClientDir(sock, "MKD", "Please enter the name of the directory to make:\n");
    //get status of directory make
    int dirStatus;
    errorCheckRecv(sock, &dirStatus, 4, "myftp: MKD: recv() directory status");
    dirStatus = ntohl(dirStatus);

    //print out status of directory make
    switch (dirStatus)
    {
    case 1:
        puts("The directory was successfully made.");
        break;
    case -2:
        puts("The directory already exists on server.");
        break;
    case -1:
        puts("Error in making directory.");
        break;
    default:
        fprintf(stderr, "Wait, it shouldn't be possible to get here.\n");
    }
    
    return 0;
}

int clientRmdir(int sock)
{
    startClientDir(sock, "RMD",
                   "Please enter the name of the directory to remove:\n");

    int dirStatus;
    //receive directory status from server
    errorCheckRecv(sock, &dirStatus, 4, 
                   "myftp: RMD: recv() dirStatus");
    dirStatus = ntohl(dirStatus);
    //DEBUG PRINT
    printf("dirStatus is %d\n", dirStatus);
    if (dirStatus > 0)
    {
        printf("Are you sure you want to remove the directory? (Yes/No): ");
        char answer[16];
        int wrong = 0;
        do 
        {
            if (wrong)
            {
                printf("Please enter either Yes or No: ");
            }
            fgets(answer, 12, stdin);
            answer[strlen(answer)-1] = '\0'; //chop off the newline
            int i;
            for (i = 0; i < strlen(answer); i++)
            {
                answer[i] = tolower(answer[i]);
            }
            wrong = 1;
        } while (strcmp(answer, "yes") && strcmp(answer, "no"));
        if (!strcmp(answer, "no"))
        {
            errorCheckStrSend(sock, "No", "myftp: RMD: send() No");
            puts("Delete abandoned by the user!");
        }
        if (!strcmp(answer, "yes"))
        {
            errorCheckStrSend(sock, "Yes", "myftp: RMD: send() Yes");
            //check if successfuly deleted:
            int deleteStatus;
            errorCheckRecv(sock, &deleteStatus, 4, 
                           "myftp: RMD: recv() delete status");
            deleteStatus = ntohl(deleteStatus);
            if (deleteStatus > 0)
            {
                puts("Directory Deleted.");
            }
            else
            {
                puts("Failed to delete directory.");
            }
        }      
    }
    else
    { 
        puts("The directory does not exist on server.");
    }
    return 0;
}

int clientDelete(int sock)
{
    startClientDir(sock, "DEL",
                   "Please enter the name of the file to delete:\n");

    int fileStatus;
    //receive file status from server. (Whether or not it exists)
    errorCheckRecv(sock, &fileStatus, 4, 
                   "myftp: DEL: recv() fileStatus");
    fileStatus = ntohl(fileStatus);
    //DEBUG PRINT
    printf("fileStatus is %d\n", fileStatus);
    if (fileStatus > 0)
    {
        printf("Are you sure you want to delete the file? (Yes/No): ");
        char answer[16];
        int wrong = 0;
        do 
        {
            if (wrong)
            {
                printf("Please enter either Yes or No: ");
            }
            fgets(answer, 12, stdin);
            answer[strlen(answer)-1] = '\0'; //chop off the newline
            int i;
            for (i = 0; i < strlen(answer); i++)
            {
                answer[i] = tolower(answer[i]);
            }
            wrong = 1;
        } while (strcmp(answer, "yes") && strcmp(answer, "no"));
        if (!strcmp(answer, "no"))
        {
            errorCheckStrSend(sock, "No", "myftp: DEL: send() No");
            puts("Delete abandoned by the user!");
        }
        else if (!strcmp(answer, "yes"))
        {
            errorCheckStrSend(sock, "Yes", "myftp: DEL: send() Yes");
            //check if successfuly deleted:
            int deleteStatus;
            errorCheckRecv(sock, &deleteStatus, 4, 
                           "myftp: DEL: recv() delete status");
            deleteStatus = ntohl(deleteStatus);
            if (deleteStatus > 0)
            {
                puts("File Deleted.");
            }
            else
            {
                puts("Failed to delete the file.");
            }
        }      
    }
    else
    { 
        puts("The file does not exist on server.");
    }
    return 0;
}

int clientCd(int sock)
{
    errorCheckStrSend(sock, "CHD", PROG3_SEND_OP_ERR);

    char changeDir[100];
    short int cdNameSize;

    //Get the name of the directory to change to:
    printf("Please list the name of the directory that you would like to change to: ");
    fgets(changeDir, 100, stdin);
    printf("File Name: %s", changeDir);
    changeDir[(strlen(changeDir)-1)] = '\0';

    //Send the length of the directory name and the directory name:
    cdNameSize = htons(strlen(changeDir)+1);
    
    errorCheckSend(sock, &cdNameSize, 2, "myftp: ");
    errorCheckStrSend(sock, changeDir, "myftp: ");
   
    //Receive and process confirm from server:
    int dirStatus;
    errorCheckRecv(sock, &dirStatus, 4, "myftp: MKD: recv() directory status");
    dirStatus = ntohl(dirStatus);

    //print out status of directory make
    switch (dirStatus) {
      case -2:
        puts("The directory does not exist on this server.");
        return 0;
        break;

      case -1:
        puts("Error in changing directory.");
        return 0;
        break;

      case 1:
        puts("Changed current directory.");
        return 0;
        break;
  
      default:
        fprintf(stderr, "Wait, it shouldn't be possible to get here.\n");
    }
  
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
    printf("\nWelcome to THE super awesome Project 3: Client Side Edition! :D \n");
    while (!loopExit)
    {
        printf("\nWhat would you like to do? Type in the 3 letter code of your choice.\n \n");
        printf("REQ: Request a file to download from the server.\n");
        printf("UPL: Upload a file to the server.\n");
        printf("DEL: Delete a file from the server.\n");
        printf("LIS: List all files and directories at your current position.\n");
        printf("MKD: Make a directory on the server.\n");
        printf("RMD: Remove a directory from the server.\n");
        printf("CHD: Change your current directory on the server.\n");
        printf("XIT: Exit the program.\n");

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
