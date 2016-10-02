//This is a server
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

int main(int argc, char * argv[]){
  struct sockaddr_in sin;
  char buf[4096];
  char *portString;
  socklen_t len;
  int s, new_s, port, sent;

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
    perror("simplex-talk:socket");
    exit(1);
  }
  //bind socket to specified address
  if((bind(s,(struct sockaddr *)&sin,sizeof(sin)))<0){
    perror("simplex-talk: bind");
    exit(1);
  }
  //listen to the socket
  if((listen(s,0))<0){
    perror("simplex-talk:listen");
    exit(1);
  }
  printf("Hello, and Welcome to the Server of the 21st Century!\n");
  /*wait for connection, then receive and print text */
  while(1){
    if((new_s = accept(s,(struct sockaddr *)&sin,&len))<0){
      perror("simplex-talk:accept");
      exit(1);
    }
    while(1){
      if((len=recv(new_s,buf,sizeof(buf),0))==-1){
        perror("Server Received Error!");
        exit(1);
      }
      if(len==0) break;
      printf("TCP Server Received:%s\n",buf);
      if((sent=send(new_s,buf,len,0))==-1){
        perror("Server Send Error!");
        exit(1);
      }
    }
    printf("Client finishes, close the connection!\n");
    close(new_s);
  }
}
