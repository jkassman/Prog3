//This is a server
#include "../common.h"

void serverRequest(int sock) {
  char *queryMessage = "Please enter the name of the file to receive";
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

  unsigned int fileSize;
  FILE *fileToSend;
  if (access(filename, F_OK) < 0) {
    //not a file, send -1;
    fileSize = -1;
    fileSize = htonl(fileSize);
  } else {
    //is a file, discover then send its size
    fileToSend = fopen(filename, "r");
    if (!fileToSend) {
      perror("myftpd: fopen()");
      close(sock);
      exit(6);
    }
    fileSize = getFileSize(fileToSend);
  }  
  //send the 4 byte integer
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
  
  //DEBUG PRINT
  printf("The hash is: ");
  int i;
  for (i = 0; i < 16; i++) {
    printf("%02x.", hash[i]);
  }
  puts("");
  
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
  printf("Received Upload Operation");
  //Receive the two-byte length of the filename
  unsigned short int fileNameLen;
  int status;
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

  char *ackMessage = "Acknowledge Received Filename: Ready to Receive File";
  status = send(sock,ackMessage,strlen(ackMessage),0);
  if (status < 0) {
    perror("myftpd: send() req query");
    close(sock);
    exit(3);
  }

}



int main(int argc, char * argv[]){
  struct sockaddr_in sin;
  char buf[PROG3_BUFF_SIZE];
  char message[PROG3_BUFF_SIZE];
  char *portString;
  socklen_t len;
  int s, new_s, port; // sent;

  //check for correct number of arguments and assign arguments to variables
  if (argc==2){
    portString=argv[1];
    port = atoi(portString);
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
  /*wait for connection, then receive and print text */
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
      else if(strcmp("DEL",buf)==0){
        strcpy(message,"You sent DEL! w00t!\n");
      }
      else if(strcmp("LIS",buf)==0){
        strcpy(message,"You sent LIS! Booyah!\n");
      }
      else if(strcmp("MKD",buf)==0){
        strcpy(message,"You sent MKD! Way to go man!\n");
      }
      else if(strcmp("RMD",buf)==0){
        strcpy(message,"You sent RMD! What a play!\n");
      }
      else if(strcmp("CHD",buf)==0){
        strcpy(message,"You sent CHD! Boomshakalaka!\n");
      }
      else if(strcmp("XIT",buf)==0){
        close(new_s);
        break;
      }
      else{
        strcpy(message,"Send a correct command dumbass\n");
      }
      printf("TCP Server Received:%s\n",buf);
      /*
      if((sent=send(new_s,message,strlen(message),0))==-1){
        perror("Server Send Error!");
        exit(1);
	} */
    }
    printf("Client Quit!\n");
    close(new_s);
  }
  close(s);
}
