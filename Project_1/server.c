
/* A simple server in the internet domain using TCP
   The port number is passed as an argument
   This version runs forever, forking off a separate
   process for each connection
*/
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <sys/stat.h>    // structures for stat
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>  /* signal name macros, and the kill() prototype */
#include <fcntl.h>
#include <string.h>

void error(char *msg)
{
  perror(msg);
  exit(1);
}

void readFileContent(int fileFD, char** content, int* contentLen) {
  struct stat st;
  if(fstat(fileFD, &st) < 0) {
    error("ERROT: cannot read requested files stats");
  }
  
  int fileLen = st.st_size; //byte size of file                                                                                                                
  char* fileStr = (char*) malloc(sizeof(char) * fileLen);
  
  int bytesTotal = 0;
  int bytesRead;
  while((bytesRead = read(fileFD, (fileStr + bytesTotal), fileLen)) > 0) {
    bytesTotal += bytesRead;
  }
  if(bytesRead < 0) {
    error("ERROR: cannot read from specified file");
  }

  //return contents and length                                                                                                                                  
  *content = fileStr;
  *contentLen = fileLen;
  return;
}


void sendFileContent(int sockFD, int fileFD) {
  char* fileContent = NULL;
  int fileLen = -1;
  readFileContent(fileFD, &fileContent, &fileLen);
  write(sockFD, fileContent, fileLen); //might have to be put into while loop for large files                                             
  //writeFileContent(fileContent, fileLen);                                                                                               
}

void send404Error(int socketFD) {
  char* error404Msg = "Error 404, resource not found";
  write(socketFD, error404Msg, strlen(error404Msg)); 
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);  // create socket
    if (sockfd < 0)
        error("ERROR opening socket");
    memset((char *) &serv_addr, 0, sizeof(serv_addr));   // reset memory

    // fill in address info
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd, 5);  // 5 simultaneous connection at most

    //accept connections
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

    if (newsockfd < 0)
     error("ERROR on accept");

    int n;
    char buffer[256];

    memset(buffer, 0, 256);  // reset memory

    //read client's message
    n = read(newsockfd, buffer, 255);
    if (n < 0) error("ERROR reading from socket");
    printf("Here is the message: %s\n", buffer);
    
    char * requestedFile = "test.txt";
    int requestedFD;
    if((requestedFD = open(requestedFile, O_RDONLY)) < 0) {
      send404Error(newsockfd);
    } else {
      sendFileContent(newsockfd, requestedFD);
    }
    
    
    
    //reply to client
    n = write(newsockfd, "I got your message", 18);
    if (n < 0) error("ERROR writing to socket");

    close(newsockfd);  // close connection
    close(sockfd);

    return 0;
}
