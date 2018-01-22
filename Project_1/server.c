
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
#include <time.h>

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
	
    char file_path[256];

    // tokenize to get every spaced word
    char *saveptr_line, *saveptr_word;
    char *line, *word;
    char* request_lines = buffer;
    char line_tok[2] = "\n";
    line_tok[1] = 0;
    char word_tok[2] = " ";
    word_tok[1] = 0;

    for (int i = 0; ; i++, request_lines = NULL)
    {
      line = strtok_r(request_lines, line_tok, &saveptr_line);
      if (line == NULL)
	break;
      //printf("%s\n\n", line);
      for (int j = 0; ; j++, line = NULL) {
	word = strtok_r(line, word_tok, &saveptr_word);
	if (word == NULL)
	  break;

	if (i == 0) // header line
	{
	  if (j == 0){
	    if (strcmp("GET", word) == 0)
	      {
	    
	      }
	    else
	      {
	    
	      }
	  }
	  if (j == 1){
	    strcpy(file_path, word);
	  }
	  if (j == 2){
	    if (strcmp ("HTTP/1.0", word) != 0)
	      {	
		//error("ERROR only supports HTTP 1.0.");
		//close(newsockfd);
		//close(sockfd);
	      }
	  }
	}
	//printf("%s ", word);
      }
      printf("\n");
    }
    
    char response[1024];
    char date_text[512];
    time_t now = time(0);
    struct tm tm = *gmtime(&now);
    strftime(date_text, sizeof(date_text), "%a, %d %b %Y %H:%M:%S %Z", &tm);
    struct stat attr;
    char last_modified[512];
    stat(file_path, &attr);
    strftime(last_modified, sizeof(last_modified), "%a, %d %b %Y %H:%M:%S%Z", gmtime(&(attr.st_ctime)));struct stat attr;
    char last_modified[512];
    char format_str = "HTTP/1.0 200 OK\nDate: %s\nServer: localhost\nMIME-version: 1.0\nLast-Modified: %s", date_text, last_modified);
    sprintf(response, format_str, date_text);

    printf("%s", response);
    
    int requestedFD;
    if((requestedFD = open("test.txt", O_RDONLY)) < 0) {
      send404Error(newsockfd);
    } else {
      sendFileContent(newsockfd, requestedFD);
    }
    
    //reply to client
    if (n < 0) error("ERROR writing to socket");

    close(newsockfd);  // close connection
    close(sockfd);

    return 0;
}
