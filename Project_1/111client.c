/*
NAME: Jaron Mink
EMAIL: jaronmink@gmail.com
ID: 904598072
*/

#include <fcntl.h>
#include <termios.h> //tcgetattr()
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h> 
#include <unistd.h> //POSIX Functions
#include <stdlib.h> //malloc and free
#include <getopt.h> //used to for getopt_long
#include <stdbool.h>
#include <signal.h> //to send seg fault
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h> //for strerror
#include <poll.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <mcrypt.h>

struct pollfd ufds[2];

struct termios savedTerminalAtt;
bool shellopt = false;
bool processFinalInput = false;
pid_t childPID = 0;
int serverFD = -1;
int logFD = -1;
char *logFileName = NULL;
bool logOpt = false;
//char *keyFileName = NULL;
bool encryOpt = false;
MCRYPT tdEncry, tdDecry; 

static struct option long_options[] = {
  {.name = "port", .has_arg = required_argument, .flag = 0, .val ='p'},
  {.name = "log", .has_arg = required_argument, .flag = 0, .val ='l'},
  {.name = "encrypt", .has_arg = required_argument, .flag = 0, .val ='e'},
  {NULL, 0, 0, 0}
};

void deinitEncryption();

void restoreTerminalState()
{
  close(serverFD);
  if(logOpt)
    close(logFD);
  if(encryOpt)
    deinitEncryption();
  tcsetattr(STDIN_FILENO, TCSANOW, &savedTerminalAtt);
}


void setUpTerminal() {
  
  struct termios newTermAtt;
  if(!isatty(STDIN_FILENO))
  {
    fprintf(stderr, "not referring to terminal! Error!\n");
    exit (EXIT_FAILURE);
  }
    //save terminal settings so we can reset right before exiting
    tcgetattr(STDIN_FILENO, &savedTerminalAtt);
    atexit(restoreTerminalState);
    
    //change terminal settings as specified
    tcgetattr(STDIN_FILENO, &newTermAtt);
    newTermAtt.c_iflag = ISTRIP;
    newTermAtt.c_oflag = 0;
    newTermAtt.c_lflag = 0;    

    //set the attributes
    tcsetattr(STDIN_FILENO, TCSANOW, &newTermAtt); 
}

void writeToLogFile(char *buf, long len)
{
  if(logFD == -1)
    {
      mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO;//S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
	
      if ((logFD = open(logFileName, O_RDWR|O_CREAT|O_TRUNC, mode)) < 0)//creat(fileName, mode)) < 0)
	{
	  char* errorMsg = strerror(errno);
	  fprintf(stderr, "error creating tmp file:%s", errorMsg);
	  exit(1);
    	}
    }
  write(logFD, buf, len);
}


void writeHeaderToLogFile(bool sentData, int bytesTransferred) {
  char *header1 = NULL;

  if(sentData)
    {
      header1 = "SENT ";
    }
  else
    {
      header1 = "RECIEVED ";
    }

  char *sizeStr = malloc(sizeof(char)*32);
  sprintf(sizeStr, "%d", bytesTransferred);
  char header3[] = " bytes: ";

  char *header = malloc(sizeof(char)*(strlen(header1) + strlen(sizeStr) + strlen(header3)));
  header[0] = 0;
  strcat(header, header1);
  strcat(header, sizeStr);
  strcat(header, header3);

  writeToLogFile(header, strlen(header));
}


void readInKey(char* keyFileName, char* *keyPtr, int *keySize )
{
  int keyFD;
  if((keyFD = open(keyFileName, O_RDONLY)) < 0)
    {
      char *errorMsg = strerror(errno);
      fprintf(stderr, "Error opening key has occurred: %s", errorMsg);
      exit(1);
    }

  struct stat st;
  if(fstat(keyFD, &st) < 0) {
    char *errorMsg = strerror(errno);
    fprintf(stderr, "Error getting key stat has occurred: %s", errorMsg);
    exit(1);
  }
  int keyLen = st.st_size;
  char *keyStr = (char *) malloc(sizeof(char) * keyLen); 

  int bytesTotal = 0;
  int bytesRead = -1;
  while((bytesRead = read(keyFD, (keyStr + bytesTotal), keyLen)) > 0)
    {
      bytesTotal += bytesRead;
    }
  if(bytesRead < 0) {
      char *errorMsg = strerror(errno);
      fprintf(stderr, "Error reading key has occurred: %s", errorMsg);
      exit(1);
    }

  *keyPtr = keyStr;
  *keySize = keyLen;
  return;
}

//initalize and open modules for encryption and decryption

//initalize and open modules for encryption and decryption                                                                                                    
void initEncryption(char* key, int keySize) {

  if((tdEncry = mcrypt_module_open("twofish", NULL, "cfb", NULL)) == MCRYPT_FAILED)
  {
    fprintf(stderr, "Error! Opening of encryption module failed\n");
    exit(1);
  }

   char *IV1 = malloc(mcrypt_enc_get_iv_size(tdEncry));
   int i;
   for(i = 0; i < mcrypt_enc_get_iv_size(tdEncry); i++)
     IV1[i] = 'i';//rand();

  if(keySize > mcrypt_enc_get_key_size(tdEncry))
  {
    fprintf(stderr, "Error, key size is too large to use. Max size is \'%d\'", mcrypt_enc_get_key_size(tdEncry));
  }

  //int blah;                                                                                                                                                 

  if(mcrypt_generic_init(tdEncry, key, keySize, IV1) < 0)
    {
      fprintf(stderr, "Error initializing encryption buffers!");
      exit(1);
    }

  //  fprintf(stderr, "hello\n");                                                                                                                             

  if((tdDecry = mcrypt_module_open("twofish", NULL, "cfb", NULL)) == MCRYPT_FAILED)
  {
    fprintf(stderr, "Error! Opening of encryption module failed\n");
    exit(1);
  }

  if(keySize > mcrypt_enc_get_key_size(tdDecry))
  {
    fprintf(stderr, "Error, key size is too large to use. Max size is \'%d\'",mcrypt_enc_get_key_size(tdDecry));
  }
  /*
  char *IV2 = malloc(mcrypt_enc_get_iv_size(tdDecry));
  int k;
  for(k = 0; k < mcrypt_enc_get_iv_size(tdDecry); k++)
    IV2[i] = rand();
  */
  
  if(mcrypt_generic_init(tdDecry, key, keySize, IV1) < 0)
    {
      fprintf(stderr, "Error initializing encryption buffers!");
      exit(1);
    }
}

void deinitEncryption()
{
  mcrypt_generic_deinit(tdEncry);
  mcrypt_module_close(tdEncry);

  mcrypt_generic_deinit(tdDecry);
  mcrypt_module_close(tdDecry);
}

void encrypt(char *buf, int bufLen)
{
  mcrypt_generic(tdEncry, buf, bufLen);
}

void decrypt(char *buf, int bufLen)
{
  mdecrypt_generic(tdDecry, buf, bufLen);
}
  
void readFromWriteTo(long readFD, long mainFD, long shellFD, long bufLen, bool dontWriteToShell) {
  char* buf = malloc( sizeof(char) * bufLen); 
  int bytesRead = 0;
  bytesRead = read(readFD, buf, bufLen);
  
  if(bytesRead < 0)
    {
      char* errorMsg = strerror(errno);
      fprintf(stderr, "Error reading input: %s", errorMsg);
      exit(1);
    }
    
  if (bytesRead == 0 && readFD == ufds[0].fd)
    {
      exit(0);
    }
  //if(bytesRead > 0 && fromShell)
  //  fprintf(stderr, "bytesRead:%i", bytesRead); 
  //check for <cr> and <lf> ----> <cr><lf>
      //write char per char
  if(logFileName != NULL)
    {
      writeHeaderToLogFile((!dontWriteToShell), bytesRead);
    }
  
  for(int i = 0; i < bytesRead; i++)
    {
      //bool newlineRecieved = false;
      char currentChar = buf[i];
      if(dontWriteToShell)
	{
	  if(logFileName)
	    writeToLogFile(&currentChar, 1);
	  if(encryOpt)
	    decrypt(&currentChar, 1);
	}
      
      //check for ^D
      if(currentChar == 0x4)
	{
	  char replacementText[2] = {'^', 'D'};
	  write(mainFD, replacementText, 2);
	}
      else if (currentChar == 0x3)
	{
	  char replacementText[2] = { '^', 'C'};
	  write(mainFD, replacementText, 2);
	 
	}
      else if((currentChar == '\r' || currentChar == '\n')) //&& !dontWriteToShell) //double spaces are here!
	{
	  char replacementText[2] = {'\r', '\n'};
	  write(mainFD, &replacementText, 2);
	  currentChar = '\n';
	  //newlineRecieved = true;
	}
      else //just write single char
	{
	  write(mainFD, &currentChar, 1);
	}
      
      if (!dontWriteToShell)
	{ //if shell, write an <lf> to it
	  if(encryOpt)
	    {
	      //char str1[2] = { currentChar, '\0' };
	      //fprintf(stderr, "before:%s\n", str1);
	      encrypt(&currentChar, 1);
	      //char str2[2] = { currentChar, '\0' };
	      //fprintf(stderr, "after:%s\n", str2);
	    }
	  
	  write(shellFD, &currentChar, 1);
	  if(logFileName){
	   writeToLogFile(&currentChar, 1);
	  }
	}
    }

  //if(logFileName && dontWriteToShell)
  writeToLogFile("\n", 1);
  free(buf);
}

 void setUpPolls(int fd1, int fd2)
{
  ufds[0].fd = fd1;
  ufds[1].fd = fd2;

  ufds[0].events = POLLIN | POLLHUP; //| POLLRDBAND;
  ufds[1].events = POLLIN | POLLHUP; //| POLLRDBAND;
}

//much of the code was taken from http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/socket.html
//if successful, returns the connection fd, else it will exit the program
int connectToServer(char* serverName ,int portNumber)
{
  struct sockaddr_in serv_addr;
  struct hostent *server;
  int sockfd = -1;
  
  //char serverName[] = "lnxsrv09.seas.ucla.edu";
  if((server = gethostbyname(serverName)) == NULL)
    {
      fprintf(stderr, "Host for given name cannot be found!");
      exit(1);
    }

  
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
      fprintf(stderr, "Error opening socket!");
      exit(1);
    }

  memset((char *)&serv_addr, 0, sizeof(serv_addr)); //set all bytes to 0
  serv_addr.sin_family = AF_INET; // using internet address domain
  memmove ((char *) &serv_addr.sin_addr.s_addr,(char *)server->h_addr, server->h_length); //copy server address

  serv_addr.sin_port = htons(portNumber);

  if(connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
      fprintf(stderr, "Error connecting to host.\n");
      exit(1);
    }
  
  return sockfd; //return file descriptor to write and read to
}

  
int main(int argc, char* argv[])
{
  int portNum = 0;
  logFileName = NULL;
  char *key = NULL;
  int keyLen = 0;
  bool portOpt = false;
  shellopt = true; //not an option anymore, want to eventually remove
  char optionChar;
    while( (optionChar = getopt_long(argc, argv, "", long_options, NULL)) != -1) { //if there are still options to parse
    switch(optionChar)
      {
      case'p':
	portNum = atoi(optarg);
	portOpt = true;
	break;
      case 'l':
	logOpt = true;
	//fprintf(stderr, "blah");
	logFileName = optarg;
	//fprintf(stderr, "blah");
	break;
      case 'e':
	readInKey(optarg, &key, &keyLen);
	initEncryption(key, keyLen);
	//fprintf(stderr, "%s %d\n", key, keyLen); 
	encryOpt = true;
	break;
      case '?':
	fprintf(stderr, "Invalid option attempted!\nAvaliable options:\n\t--port=\'arguement\'\n\t--encrypt=\'arg\'\n\t--log=\'arg\'\n"); 
	exit(1);
	break;	
      }
    }
    
    //make sure portOpt is required!
    if(!portOpt)
      {
	fprintf(stderr, "Error, --port=\'arguement\' option is required!\n");
	exit(1);
      }
    
  
    //put into noncanonical, no echo mode
    setUpTerminal();

    serverFD = connectToServer("localhost", portNum);
    setUpPolls(serverFD, STDIN_FILENO);
    const int BUFF_LEN = 256;
    while(1)
      {
	int pollRet =  poll(ufds, 2, 0);
	if(pollRet < 0)
	  {
	    char* errorMsg = strerror(errno);
	    fprintf(stderr, "Error polling: %s", errorMsg);
	    exit(1);
	  }
	
	if(pollRet > 0)
	  {
	    for(int i = 0; i <2; i++)
	      {
		//if(processFinalInput && ufds[i].fd != pipeFDParent[0])
		// continue;
		
		if (ufds[i].revents & POLLIN || ufds[i].revents & POLLHUP)
		  {
		    bool fromServer = false;
		    if(i==0)
		      {
			//char* buf = malloc(sizeof(char)*256);
			//int bytes;
			//if(bytes = read(ufds[0].fd, buf, 256))
			//write(STDOUT_FILENO, buf, bytes);
			fromServer = true;
		      }
		    readFromWriteTo(ufds[i].fd, STDOUT_FILENO, serverFD, BUFF_LEN, fromServer);
		  }
		//else if (pipeFDParent[0] && processFinalInput) //if we finished, exit
		  //{
		// exit(0);
		// }
	      }
	  }
      }
}
