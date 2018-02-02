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
#include <ctype.h>

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
  while((bytesRead = read(fileFD, (fileStr + bytesTotal), fileLen - bytesRead)) > 0) {
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


void sendFileContent(int sockFD, int fileFD, char* httpResponse) {
  char* fileContent = NULL;
  int fileLen = -1;
  readFileContent(fileFD, &fileContent, &fileLen);
  char content_len_str[50];
  sprintf(content_len_str, "Content-Length: %i\n", fileLen);
  strcat(httpResponse, content_len_str);
  strcat(httpResponse, "\n");
  write(sockFD, httpResponse, strlen(httpResponse));
  write(sockFD, fileContent, fileLen);
}

void send404Error(int socketFD, char* httpResponse) {
  char* error404Msg = "Error 404, resource not found";
  char content_len_str[100];
  sprintf(content_len_str, "Content-Length: %zd\n\n", strlen(error404Msg));
  write(socketFD, httpResponse, strlen(httpResponse));
  write(socketFD, content_len_str, strlen(content_len_str));
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

    //while(1) {
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

    //accept connections and process them
    while(1){
      newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

      if (newsockfd < 0)
	error("ERROR on accept");
      
      int n;
      char buffer[512];
      
      memset(buffer, 0, 512);  // reset memory
      
      //read client's message
      n = read(newsockfd, buffer, 511);
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
	  }
	}
      char file_path_spaces[256] = {0};
      for (int i = 0; i < strlen(file_path); i++) {
	char three_chars[256];
	strcpy(three_chars, file_path+i);
	three_chars[3] = '\0';
	if (strcmp(three_chars, "%20") == 0) {
	  i += 2;
	  strcat(file_path_spaces, " ");
	}
      else {
      	char non_space_char[2];
      	sprintf(non_space_char, "%c", file_path[i]);
      	strcat(file_path_spaces, non_space_char);
      }
      }
      
      char* file_ext = NULL;
      char* file_index = file_path_spaces+1;
      for (int i = 0; i < strlen(file_index); i++)
	{
      	if (file_index[i] == '.')
      	  {
      	    file_ext = file_index+i+1;
      	    break;
      	  }
	}
      for (int i = 0; file_index != NULL && i < strlen(file_ext); i++)
	file_ext[i] = tolower(file_ext[i]);
      
      char response[1024];
      char date_text[512];
      time_t now = time(0);
      struct tm tm = *gmtime(&now);
      strftime(date_text, sizeof(date_text), "%a, %d %b %Y %H:%M:%S %Z", &tm);
      struct stat attr;
      char last_modified[512];
      stat(file_index, &attr);
      strftime(last_modified, sizeof(last_modified), "%a, %d %b %Y %H:%M:%S %Z", gmtime(&(attr.st_mtime)));
      char format_str[512] = "HTTP/1.0 200 OK\n";
      strcat(format_str, "Date: %s\n");
      strcat(format_str, "Server: localhost\n");
      strcat(format_str, "MIME-version: 1.0\n");
      strcat(format_str, "Last-Modified: %s\n");
      char* mime_type;
      if (file_ext != NULL) {
	if (strcmp(file_ext, "html") == 0)
	  mime_type = "text/html";
	else if (strcmp(file_ext, "htm") == 0)
	  mime_type = "text/htm";
	else if (strcmp(file_ext, "jpg") == 0 || strcmp(file_ext, "jpeg") == 0)
	  mime_type = "image/jpeg";
	else if (strcmp(file_ext, "png") == 0)
	  mime_type = "image/png";
	else if (strcmp(file_ext, "gif") == 0)
          mime_type = "image/gif";
	else if (strcmp(file_ext, "txt") == 0)
	  mime_type = "text/txt";
      }
      else
	mime_type = "text/html";
      
      char content_type_str[50];
      sprintf(content_type_str, "Content-Type: %s\n", mime_type);
      strcat(format_str, content_type_str);
      strcat(format_str, "Expires: 0\n");
      sprintf(response, format_str, date_text, last_modified, attr.st_size);
      
      char err_format_str[512] = "HTTP/1.0 404 Not Found\n";
      strcat(err_format_str, "Date: %s\n");
      strcat(err_format_str, "Server: localhost\n");
      strcat(err_format_str, "MIME-version: 1.0\n");
      strcat(err_format_str, "Last-Modified: %s\n");
      strcat(err_format_str, "Content-Type: text/html\n");
      strcat(err_format_str, "Expires: 0\n");
      char err_response[1024];
      sprintf(err_response, err_format_str, date_text, date_text);
      
      fprintf(stderr, "%s", file_index);
      int requestedFD;
      if((requestedFD = open(file_index, O_RDONLY)) < 0) {
	send404Error(newsockfd, err_response);
      } else {
	sendFileContent(newsockfd, requestedFD, response);
      }
      
      //reply to client
      if (n < 0) error("ERROR writing to socket");
      
      close(newsockfd);  // close client connection
    }
    
    close(sockfd);
    return 0;
}
