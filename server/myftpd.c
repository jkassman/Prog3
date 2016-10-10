
/*
  Authors: Jacob Kassman, Lauren Kuta, Matt Paulson
  netIDs: jkassman, lkuta, mpaulson
  Computer Networks: Programming Assignment 3
  myftpd.c
  Creates a TCP server able to receive several commands from a client.
 */

#include "../common.h"

void serverRequest(int sock) {
  char *queryMessage = "Please enter the name of the file to receive:";
  int status;
  status = send(sock,queryMessage,strlen(queryMessage),0);
  if (status < 0) {
    perror("myftpd: send() req query");
    close(sock);
    exit(3);
  }

  //Receive the two-byte length of the filename
  unsigned short int fileNameLen;
  status = recv(sock,(char*) &fileNameLen, 2, 0);
  if (status < 0) {
    perror("myftpd: recv filename length");
    close(sock);
    exit(3);
  }

  fileNameLen = ntohs(fileNameLen);

  //printf("The file name length is %d\n", fileNameLen);

  char filename[fileNameLen];
  //Receive the name of the file:
  status = recv(sock, filename, fileNameLen, 0);
  if (status < 0) {
    perror("myftpd: recv filename");
    close(sock);
    exit(3);
  }
 //printf("Receieved %d bytes\n", status);
 //printf("The name of the file is %s\n", filename);

 //Check to see if the file exists on the server
  unsigned int fileSize;
  FILE *fileToSend;
  if (access(filename, F_OK) < 0) {
    //if it does not exist, send -1 to client
    fileSize = -1;
    fileSize = htonl(fileSize);
  } else {
    //if it does exist, try to open it. Then send size if successful
    fileToSend = fopen(filename, "r");
    if (!fileToSend) {
      perror("myftpd: fopen()");
      close(sock);
      exit(6);
    }
    fileSize = getFileSize(fileToSend);
  }  
  //send the 32-bit file size
  status = send(sock, (char*) &fileSize, 4, 0);
  if (status < 0) {
    perror("myftpd: send file size");
    close(sock);
    exit(3);
  }
  
  //exit the function if the file did not exist
  if (fileSize == -1) return;
  
  //find the MD5 hash of the file:
  unsigned char hash[16];
  hashFile(hash, fileToSend);
  
  /*printf("The hash is: ");
  int i;
  for (i = 0; i < 16; i++) {
    printf("%02x.", hash[i]);
  }
  puts(""); */
  
  //Send the hash
  status = send(sock, hash, 16, 0);
  if (status < 0) {
    perror("myftpd: send MD5 hash");
    close(sock);
    exit(3);
  }

  //Send the file over the socket
  sendFile(sock, fileToSend, fileSize, "myftpd");

  fclose(fileToSend);
}

void serverUpload(int sock) {
  unsigned short int fileNameLen;
  unsigned char hash[16];
  unsigned char recvdHash[16];
  char throughputMessage[100];
  char failedMessage[100];
  unsigned int fileLenBuffy;
  int status;
  int fileLenRecvd, hashRecvd;
  double transferTime;
  double throughput;
  struct timezone tz;
  struct timeval ts1;
  struct timeval ts2;

  //Receive the two-byte length of the filename
  status = recv(sock,(char*) &fileNameLen, 2, 0);
  if (status < 0) {
    perror("myftpd: recv filename length");
    close(sock);
    exit(3);
  }

  fileNameLen = ntohs(fileNameLen);
  
  //printf("The file name length is %d\n", fileNameLen);

  char filename[fileNameLen];
  //Receive the name of the file:
  status = recv(sock, filename, fileNameLen, 0);
  if (status < 0) {
    perror("myftpd: recv filename");
    close(sock);
    exit(3);
  }
  //printf("Receieved %d bytes\n", status);
  //printf("The name of the file is %s\n", filename);

  char *ackMessage = "Acknowledge Received Filename: Ready to Receive File";
  status = send(sock,ackMessage,strlen(ackMessage),0);
  if (status < 0) {
    perror("myftpd: send() req query");
    close(sock);
    exit(3);
  }

  //Receives 32-bit file length:
  fileLenRecvd = recv(sock, (char *)&fileLenBuffy, 4, 0);
  if(fileLenRecvd < 0) 
  {
     fprintf(stderr, "myftp Error: recv() 32-bit file length: %s\n", strerror(errno));
      close(sock);
      exit(3);
  }

  fileLenBuffy = ntohl(fileLenBuffy);
  //printf("File length: %i \n", fileLenBuffy);

  //get the time when beginning to receive file
  gettimeofday(&ts1,&tz);

  //Receieve a file from the client:
  FILE *f = fopen(filename, "w");
  if(!f){
   printf("Error opening file \n");
   exit(242);
  }
  recvFile(sock, f, fileLenBuffy, "myftpd");

  //get the time after receiving file
  gettimeofday(&ts2,&tz);

  //Computes the MD5 hash of recieved file: 
  fclose(f);
  f = fopen(filename, "r");
  hashFile(recvdHash, f);
  fclose(f);

  //Receives MD5 Hash:
  hashRecvd = recv(sock, hash, 16, 0);
  if(hashRecvd < 0) 
  {
     fprintf(stderr, "myftpd Error: recv() MD5 hash: %s\n", strerror(errno));
      close(sock);
      exit(3);
  }

  //compute throughput
  transferTime = (ts2.tv_sec - ts1.tv_sec) + 1e-6*(ts2.tv_usec - ts1.tv_usec);
  throughput = ((fileLenBuffy/transferTime)/1e6);
  
  /*int j;
  printf("Hash value: \n");
  for( j = 0; j < 16; j++) {
    printf("%02x.", hash[j]);
  }
  printf("\n"); */

  //Compares the two hashes:
  if(!hashCompare(hash, recvdHash)) {
    sprintf(throughputMessage, "%u bytes transferred in %f seconds: %f Megabytes/sec\n", fileLenBuffy,transferTime,throughput);
    //printf("Throughput Message is:%s\n",throughputMessage);
    errorCheckSend(sock, throughputMessage, strlen(throughputMessage), "myftpd");
  }else{
    sprintf(failedMessage, "Transfer Unsuccessful");
    errorCheckSend(sock, failedMessage, strlen(failedMessage), "myftpd");
  }
  return;
}

void serverList(int sock){
  char command[50];
  int fileSize;
  int sizeToSend;

  //create a temp file that contains the directory listing
  strcpy(command, "ls > ./.tempList.txt");
  system(command);

  FILE *f = fopen("./.tempList.txt","r");

  //get the filesize
  fileSize = getFileSize(f);
  sizeToSend = htonl(fileSize);

  //send the filesize to the client
  errorCheckSend(sock, &sizeToSend, 4, "myftpd");

  //send the file
  sendFile(sock, f, fileSize, "myftpd");

  //remove the temp file
  strcpy(command, "rm ./.tempList.txt");
  system(command);

}

void serverDelete(int sock){
  unsigned short int fileNameLen;
  int status;
  char command[50];
  int confirmVal;
  char clientConfirm[1000];

  //Receive the two-byte length of the filename
  status = recv(sock,(char*) &fileNameLen, 2, 0);
  if (status < 0) {
    perror("myftpd: recv filename length");
    close(sock);
    exit(3);
  }

  fileNameLen = ntohs(fileNameLen);
  //DEBUG PRINT
  printf("The file name length is %d\n", fileNameLen);

  char filename[fileNameLen];
  //Receive the name of the file:
  status = recv(sock, filename, fileNameLen, 0);
  if (status < 0) {
    perror("myftpd: recv filename");
    close(sock);
    exit(3);
  }
  printf("Receieved %d bytes\n", status);
  printf("The name of the file is %s\n", filename);

  //if the file exists, send confirm value of 1
  FILE* f = fopen(filename, "w");
  if (f){
    confirmVal = 1;
  }
  else{
    confirmVal = -1; //otherwise sent confirm value of -1
  }
  confirmVal = ntohl(confirmVal);
  errorCheckSend(sock, &confirmVal, sizeof(int), "myftpd");

  if (confirmVal == -1){
    return;
  }

  //receive confirmation of DEL from client
  errorCheckRecv(sock, &clientConfirm, sizeof(clientConfirm), "myftpd");

  if(strcmp("Yes",clientConfirm)==0){ //if DEL confirmed, delete file
    sprintf(command, "rm %s", filename);
    if (system(command) == 0){
      confirmVal = 1; //send back 1 if deletion successful
    }
    else{
      confirmVal = -1;//otherwise send back -1
    }
    confirmVal = htonl(confirmVal);
    errorCheckSend(sock, &confirmVal, sizeof(int), "myftpd");
  }
  else if(strcmp("No",clientConfirm)==0){ //if DEL cancelled, go back to wait for operation state
    return;
  }
}

void serverMKD(int sock){
  unsigned short int dirNameLen;
  int status;
  char command[50];
  int confirmVal;

  //Receive the two-byte length of the dirname
  status = recv(sock,(char*) &dirNameLen, 2, 0);
  if (status < 0) {
    perror("myftpd: recv dirname length");
    close(sock);
    exit(3);
  }

  dirNameLen = ntohs(dirNameLen);
  //DEBUG PRINT
  printf("The directory name length is %d\n", dirNameLen);

  char dirname[dirNameLen];
  //Receive the name of the directory:
  status = recv(sock, dirname, dirNameLen, 0);
  if (status < 0) {
    perror("myftpd: recv dirname");
    close(sock);
    exit(3);
  }
  printf("Receieved %d bytes\n", status);
  printf("The name of the directory is %s\n", dirname);

  //if the directory exists, send confirm value of -2
  DIR* dir = opendir(dirname);
  if (dir){
    confirmVal = -2;
  }
  else{ //if directory doesn't exist, attempt to create it
    sprintf(command, "mkdir %s", dirname);
    if (system(command) == 0){
      confirmVal = 1; //send confirm value of 1 if successful
    }
    else{
      confirmVal = -1;//send confirm value of -1 if unsuccessful
    }
  }
  confirmVal = ntohl(confirmVal);
  errorCheckSend(sock, &confirmVal, sizeof(int), "myftpd");
}

void serverRMD(int sock){
  unsigned short int dirNameLen;
  int status;
  char command[50];
  int confirmVal;
  char clientConfirm[1000];

  //Receive the two-byte length of the dirname
  status = recv(sock,(char*) &dirNameLen, 2, 0);
  if (status < 0) {
    perror("myftpd: recv dirname length");
    close(sock);
    exit(3);
  }

  dirNameLen = ntohs(dirNameLen);
  //DEBUG PRINT
  printf("The directory name length is %d\n", dirNameLen);

  char dirname[dirNameLen];
  //Receive the name of the directory:
  status = recv(sock, dirname, dirNameLen, 0);
  if (status < 0) {
    perror("myftpd: recv dirname");
    close(sock);
    exit(3);
  }
  printf("Receieved %d bytes\n", status);
  printf("The name of the directory is %s\n", dirname);

  //if the directory exists, send confirm value of 1
  DIR* dir = opendir(dirname);
  if (dir){
    confirmVal = 1;
  }
  else{
    confirmVal = -1; //otherwise sent confirm value of -1
  }
  confirmVal = ntohl(confirmVal);
  errorCheckSend(sock, &confirmVal, sizeof(int), "myftpd");

  if (confirmVal == -1){
    return;
  }

  //receive confirmation of RMD from client
  errorCheckRecv(sock, &clientConfirm, sizeof(clientConfirm), "myftpd");

  if(strcmp("Yes",clientConfirm)==0){ //if RMD confirmed, delete directory
    sprintf(command, "rmdir %s", dirname);
    if (system(command) == 0){
      confirmVal = 1; //send back 1 if deletion successful
    }
    else{
      confirmVal = -1;//otherwise send back -1
    }
    confirmVal = ntohl(confirmVal);
    errorCheckSend(sock, &confirmVal, sizeof(int), "myftpd");
  }
  else if(strcmp("No",clientConfirm)==0){ //if RMD cancelled, go back to wait for operation state
    return;
  }
}

void serverCHD(int sock){
  unsigned short int dirNameLen;
  int status;
  //char command[50];
  int confirmVal;

  //Receive the two-byte length of the dirname
  status = recv(sock,(char*) &dirNameLen, 2, 0);
  if (status < 0) {
    perror("myftpd: recv dirname length");
    close(sock);
    exit(3);
  }

  dirNameLen = ntohs(dirNameLen);
  //DEBUG PRINT
  printf("The directory name length is %d\n", dirNameLen);

  char dirname[dirNameLen];
  //Receive the name of the directory:
  status = recv(sock, dirname, dirNameLen, 0);
  if (status < 0) {
    perror("myftpd: recv dirname");
    close(sock);
    exit(3);
  }
  printf("Receieved %d bytes\n", status);
  printf("The name of the directory is %s\n", dirname);

  //if the directory exists, send confirm value of 1
  DIR* dir = opendir(dirname);
  if (dir){
    //sprintf(command, "cd %s", dirname);
    if (chdir(dirname) == 0){
      confirmVal = 1; //send confirm value of 1 if successful
    }
    else{
      confirmVal = -1;//send confirm value of -1 if unsuccessful
    }
  }
  else{
    confirmVal = -2; //if directory doesn't exist, send confirm value of -2
  }
  confirmVal = htonl(confirmVal);
  errorCheckSend(sock, &confirmVal, sizeof(int), "myftpd");
}

int main(int argc, char * argv[]){
  struct sockaddr_in sin;
  char buf[PROG3_BUFF_SIZE];
  char message[PROG3_BUFF_SIZE];
  socklen_t len;
  int s, new_s, port;

  //check for correct number of arguments and assign arguments to variables
  if (argc==2){
      int convertStatus = stringToInt(&port, argv[1], 0, INT_MAX);
      switch (convertStatus) {
      case 1:
          fprintf(stderr, "Error: Negative port value.\n");
          exit(1);
      case 2:
          fprintf(stderr, "Error: Port value out of integer range!\n");
          exit(1);
      case 3:
          fprintf(stderr, "Error: Unrecognized characters in port (%s)\n",
                  argv[1]);
          exit(1);
      default:
          //Do nothing, successfully parsed the port
          break;
      }
  }
  else{
    fprintf(stderr, "usage: myftpd [Port]\n");
    exit(1);
  }

  /*build address data structure */
  bzero((char*)&sin,sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(port);

  /*setup passive open*/
  if((s=socket(PF_INET,SOCK_STREAM,0))<0){
    perror("myftpd:socket");
    exit(1);
  }
  //bind socket to specified address
  if((bind(s,(struct sockaddr *)&sin,sizeof(sin)))<0){
    perror("myftpd: bind");
    exit(1);
  }
  //listen to the socket
  if((listen(s,0))<0){
    perror("myftpd:listen");
    exit(1);
  }
  printf("Hello, and Welcome to the Server of the 21st Century!\n");
  /*wait for connection */
  while(1){
    if((new_s = accept(s,(struct sockaddr *)&sin,&len))<0){
      perror("myftpd:accept");
      exit(1);
    }
    while(1){
      if((len=recv(new_s,buf,sizeof(buf),0))==-1){
        perror("Server Received Error!");
        exit(1);
      }
      if(len==0) break; //client ^C
      if(strcmp("REQ",buf)==0){
	serverRequest(new_s);
      }
      else if(strcmp("UPL",buf)==0){
        serverUpload(new_s);
      }
      else if(strcmp("LIS",buf)==0){
        serverList(new_s);
      }
      else if(strcmp("DEL",buf)==0){
        serverDelete(new_s);
      }
      else if(strcmp("MKD",buf)==0){
        serverMKD(new_s);
      }
      else if(strcmp("RMD",buf)==0){
        serverRMD(new_s);
      }
      else if(strcmp("CHD",buf)==0){
        serverCHD(new_s);
      }
      else if(strcmp("XIT",buf)==0){
        close(new_s);
        break;
      }
      /*else{
        strcpy(message,"Send a correct command\n");
      } */
      printf("TCP Server Received:%s\n",buf); 
    }
    printf("Client Quit!\n");
    close(new_s);
  }
  close(s);
}
