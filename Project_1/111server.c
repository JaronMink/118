/*
NAME: Jaron Mink
EMAIL: jaronmink@gmail.com
ID: 904598072
*/
#include <mcrypt.h>
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

static struct option long_options[] = {
  {.name = "port", .has_arg = required_argument, .flag = 0, .val ='p'},
  {.name = "encrypt", .has_arg = required_argument, .flag = 0, .val ='e'},
  {NULL, 0, 0, 0}
};


struct pollfd ufds[2];

int clientFD = -1;
bool shellopt = true;
bool processFinalInput = false;
int pipeFDParent[2], pipeFDChild[2];
pid_t childPID = 0;
bool encryOpt = false;
MCRYPT tdEncry, tdDecry;
  

 
void closeAllPipes();

int connectToClient(int portNum)
{
  struct sockaddr_in serv_addr, cli_addr;
 
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    {
      fprintf(stderr, "Error opening socket!");
      exit(1);
    }

  memset( (char *) &serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portNum);

  if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
      fprintf(stderr, "Error on binding");
      exit(1);
    }

  listen(sockfd, 5);

  int  clilen = sizeof(cli_addr);
  int newsockfd = -1;
  if ((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,(socklen_t *) &clilen)) < 0)
    {
      fprintf(stderr, "Error on accept");
      exit(1);
    }
  //fprintf(stderr, "Connection has been made: %i", newsockfd);
  return newsockfd;
}

void closeAllPipes()
{
  close(pipeFDParent[0]);
  close(pipeFDParent[1]);
  close(pipeFDChild[0]);
  close(pipeFDChild[1]);
   
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
void initEncryption(char* key, int keySize) {
    
  if((tdEncry = mcrypt_module_open("twofish", NULL, "cfb", NULL)) == MCRYPT_FAILED)
  {
    fprintf(stderr, "Error! Opening of encryption module failed\n");
    exit(1);
  }

  char *IV1  = malloc(mcrypt_enc_get_iv_size(tdEncry));
   int i;
   for(i = 0; i < mcrypt_enc_get_iv_size(tdEncry); i++)
     IV1[i] = 'i';
   
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
  if(mcrypt_generic(tdEncry, buf, bufLen)<0) {
    fprintf(stderr, "Error encrypting!");
    exit(1);
  }
}

void decrypt(char *buf, int bufLen)
{
  //char str1[2] = { buf[0], '\0' };
  //fprintf(stderr, "before:%s\n", str1);
  if(mdecrypt_generic(tdDecry, buf, bufLen) < 0){
    fprintf(stderr, "Error encrypting!");
    exit(1);
  }
  //char str2[2] = { buf[0], '\0' };
  //fprintf(stderr, "after:%s\n", str2);
}


void readFromWriteTo(long readFD, long writeFD, long bufLen, bool fromShell) {
  char* buf = malloc( sizeof(char) * bufLen); //create a buffer to store 4096 bytes of data
  int bytesRead = 0;
  bytesRead = read(readFD, buf, bufLen);
  //fprintf(stderr, "Reading %d bytes!\n", bytesRead);
  //if no more input is read and we hit ^D then exit
  if(bytesRead == 0)
    {
      if(readFD == pipeFDParent[0])// wait till shell ends then exit
	 {
	  exit(0);
	 }
      //fprintf(stderr, "EOF recieved!");
      //if not the shell, then close pipe to shell so it will exit as well
      //close pipe to shell
      close(pipeFDChild[1]);
      //make sure to process all data in transit for shell
      processFinalInput = true;
    }
 
  
  if(bytesRead < 0)
    {
      char* errorMsg = strerror(errno);
      fprintf(stderr, "Error reading input: %s", errorMsg);
      exit(1);
    }
      //write char per char
  int i= -1;
  for(i = 0; i < bytesRead; i++)
    {
      char currentChar = buf[i];

      if(!fromShell && encryOpt)
	{
	  ///char str1[2] = { currentChar, '\0' };
	  //fprintf(stderr, "before:%s\n", str1);
	  decrypt(&currentChar, 1);
	  //char str2[2] = { currentChar, '\0' };
	  //fprintf(stderr, "after:%s\n", str2);       
	}
      
      //check for ^D
      if(currentChar == 0x4)
	{
	  char replacementText[2] = {'^', 'D'};
	  //close pipe to shell
	  close(pipeFDChild[1]);
	  //make sure to process all data in transit for shell
	  processFinalInput = true;
	  if(fromShell)
	    write(writeFD, replacementText, 2);
	}
      else if (currentChar == 0x3)
	{
	  kill(childPID, SIGINT);
	  if(fromShell){
	    char replacementText[2] = { '^', 'C'};
	    
	    write(writeFD, replacementText, 2);
	    //processFinalInput = true;
	  } 
	}
      
      //if <cr> or <lf> print out <cr><lf>
      /*else if(currentChar == '\r' || currentChar == '\n')
	{
	  char replacementText[2] = {'\r', '\n'};
	  
	  if (!fromShell) { //if shell then put <lf>
	    write(pipeFDChild[1], &(replacementText[1]), 1);
	  }
	  else {
	    if(encryOpt){
	       encrypt(&currentChar, 1);
	    }
	       write(writeFD, &replacementText, 2);
	  }
	  }*/
      else //just write single char
	{  
	  //fprintf(stderr, "blah\r\n");
	  if (!fromShell) { //if shell, write an <lf> to it
	    write(pipeFDChild[1], &currentChar, 1);
	    //		fprintf(stderr, "writing to pipe yo");
	  } else {
	    if(encryOpt)
	      {
		encrypt(&currentChar, 1);
	      }
	    write(writeFD, &currentChar, 1);	
	  }
	}
    }
  
  free(buf);
}

void sigPipeHandler(int sigNum)
{
  if(sigNum == SIGPIPE)
    {
      //fprintf(stderr, "Error! Write attempted to a pipe whos read end has been closed!"); 
      exit(0);
    }
}

void createPipe(int myPipe[2]) {
  if(pipe(myPipe) < 0)
    {
      char* errorMsg = strerror(errno);
      fprintf(stderr, "error creating pipes:%s", errorMsg);
      exit(1);
    }
}

void setUpChildIO()
{
  //child pipe will read from their own and write to the others
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  dup2(pipeFDChild[0], 0);
  dup2(pipeFDParent[1], 1);
  dup2(pipeFDParent[1], 2);
  close(pipeFDChild[1]);
  close(pipeFDChild[0]);
  close(pipeFDParent[0]);
  close(pipeFDParent[1]);
}

void openShell()
{
  setUpChildIO();
  
  char * args[] = {
    "/bin/bash", NULL
  };
  if(execvp("/bin/bash", args) == -1)
    {
      char* errorMsg = strerror(errno);
      fprintf(stderr, "Error, cannot run executable \"/bin/bash\":%s", errorMsg );
      exit(1);
    }
}

void setUpPolls(int socketFD)
{
  ufds[0].fd = pipeFDParent[0];
  ufds[1].fd = socketFD;

  ufds[0].events = POLLIN | POLLHUP;
  ufds[1].events = POLLIN | POLLHUP;
}

void harvestAndReport();

void finish()
{
  closeAllPipes();
  close(clientFD);
  if(encryOpt)
    deinitEncryption();
  harvestAndReport();
}

void harvestAndReport()
{
  int status = 0;
  waitpid(childPID, &status, 0);
  fprintf(stderr, "SHELL EXIT SIGNAL=%d, STATUS=%d", WIFSIGNALED(status), WEXITSTATUS(status));
}

int main (int argc, char* argv[])
{
  atexit(finish);
  int portNum =-1;//= getArguement(argc, argv); //redo this!
  char *key = NULL;
  int keyLen = 0;
  
  shellopt = true; //not an option anymore, want to eventually remove          
  char optionChar;
  while( (optionChar = getopt_long(argc, argv, "", long_options, NULL)) != -1) { //if there are still options to parse
    switch(optionChar)
      {
      case'p':
	portNum = atoi(optarg);
	break;
	
      case 'e':
        readInKey(optarg, &key, &keyLen);
	//fprintf(stderr, "%s %d\n", key, keyLen);
	initEncryption(key, keyLen);
        encryOpt = true;
        break;
      case '?':
	fprintf(stderr, "Invalid option attempted!\nAvaliable options:\n\t--port\'arguement\'\n\t--encrypt=\'arg\'\n");    //todo, add a correct usage line????
	exit(1);
	break;	
      }
  }
  

  if(portNum == -1)
    {
      fprintf(stderr, "--port=\'arguement\' option is required!\n");
      exit(1);
    }

  clientFD = connectToClient(portNum);


  //each respectively reads from their own pipe[0] and writes to their other pipe [1]
  createPipe(pipeFDParent);
  createPipe(pipeFDChild);
 
  childPID =  fork();
  if(childPID == 0)
    openShell();

  //close unused pipes
  close(pipeFDChild[0]);
  close(pipeFDParent[1]);

  //add sig handler
  if(SIG_ERR == signal(SIGPIPE, sigPipeHandler))
    {
      fprintf(stderr, "Error, cannot register SIGPIPE handler");
    }
 
  setUpPolls(clientFD);
  
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
	  int i;
	  for(i = 0; i <2; i++)
	    {
	      bool fromShell = false;
	      if(processFinalInput && ufds[i].fd != pipeFDParent[0])
		continue;
	      
	      if(i==0)
		{
		  fromShell = true;
		}
	      if (ufds[i].revents & POLLIN || ufds[i].revents & POLLHUP)
		{
		  readFromWriteTo(ufds[i].fd, clientFD, BUFF_LEN, fromShell);
		}
	      else if (processFinalInput) //if we finished, exit
		{
		  fprintf(stderr, "exiting in main");
		  exit(0);
		}
	    }
	}
    }

  
}



/*

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

struct pollfd ufds[2];

struct termios savedTerminalAtt;
bool shellopt = false;
bool processFinalInput = false;
int pipeFDParent[2], pipeFDChild[2];
pid_t childPID = 0;
static struct option long_options[] = {
    {.name = "shell", .has_arg = no_argument, .flag = 0, .val ='s'}
};

  

 
void closeAllPipes()
{
  close(pipeFDParent[0]);
  close(pipeFDParent[1]);
  close(pipeFDChild[0]);
  close(pipeFDChild[1]);
   
}

void restoreTerminalState()
{
  //  fprintf(stderr, "restored!!!!");
  closeAllPipes();

  int status = 0;
  waitpid(childPID, &status, 0);
  //int exitVal = WEXITSTATUS(status);
  //int exitSig = 0x007f & exitVal;
  //  int exitStatus = 0xff00 & exitVal;
  fprintf(stderr, "SHELL EXIT SIGNAL=%d, STATUS=%d", WIFSIGNALED(status), WEXITSTATUS(status));
  
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

  
void readFromWriteTo(long readFD, long writeFD, long bufLen, bool fromShell) {
 char* buf = malloc( sizeof(char) * bufLen); //create a buffer to store 4096 bytes of data
  int bytesRead = 0;
  bytesRead = read(readFD, buf, bufLen);

  //if no more input is read and we hit ^D then exit
  if(bytesRead == 0 && readFD == pipeFDParent[0] && processFinalInput)
    {    
      exit(0);
    }
  
  if(bytesRead < 0)
    {
      char* errorMsg = strerror(errno);
      fprintf(stderr, "Error reading input: %s", errorMsg);
      exit(1);
    }
  //if(bytesRead > 0 && fromShell)
  //  fprintf(stderr, "bytesRead:%i", bytesRead); 
  //check for <cr> and <lf> ----> <cr><lf>
      //write char per char
      for(int i = 0; i < bytesRead; i++)
	{
	  char currentChar = buf[i];

	  //check for ^D
	  if(currentChar == 0x4)
	    {
	      char replacementText[2] = {'^', 'D'};
	      write(writeFD, replacementText, 2);
	      if(shellopt)
		{
		  //close pipe to shell
		  close(pipeFDChild[1]);
		  //make sure to process all data in transit for shell
		  processFinalInput = true;
		
		}
	      else
		exit(0);
	    }
	  else if (currentChar == 0x3 && shellopt)
	    {
	      kill(childPID, SIGINT);
	      char replacementText[2] = { '^', 'C'};
	      write(writeFD, replacementText, 2);
	    }
	  
	  //if <cr> or <lf> print out <cr><lf>
	  else if(currentChar == '\r' || currentChar == '\n')
	    {
	      char replacementText[2] = {'\r', '\n'};
	      
	      if (shellopt && !fromShell) { //if shell then put <lf>
		  write(pipeFDChild[1], &(replacementText[1]), 1);
		}
	        write(writeFD, &replacementText, 2);
	      
	    }
	  else //just write single char
	    {  
	      //fprintf(stderr, "blah\r\n");
	      if (shellopt && !fromShell) { //if shell, write an <lf> to it
		write(pipeFDChild[1], &currentChar, 1);
		//		fprintf(stderr, "writing to pipe yo");
	      }
	      write(writeFD, &currentChar, 1);
	    }
    }

  free(buf);
}

void sigPipeHandler(int sigNum)
{
  if(sigNum == SIGPIPE)
    {
      fprintf(stderr, "Error! Write attempted to a pipe whos read end has been closed!"); 
      exit(1);
    }
}

void createPipe(int myPipe[2]) {
  if(pipe(myPipe) < 0)
    {
      char* errorMsg = strerror(errno);
      fprintf(stderr, "error creating pipes:%s", errorMsg);
      exit(1);
    }
}

void setUpChildIO()
{
  //child pipe will read from their own and write to the others
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  dup2(pipeFDChild[0], 0);
  dup2(pipeFDParent[1], 1);
  dup2(pipeFDParent[1], 2);
  close(pipeFDChild[1]);
  close(pipeFDChild[0]);
  close(pipeFDParent[0]);
  close(pipeFDParent[1]);
}

void openShell()
{
  setUpChildIO();
  
  char * args[] = {
    "/bin/bash", NULL
  };
  if(execvp("/bin/bash", args) == -1)
    {
      char* errorMsg = strerror(errno);
      fprintf(stderr, "Error, cannot run executable \"/bin/bash\":%s", errorMsg );
      exit(1);
    }
}

void setUpPolls()
{
  ufds[0].fd = pipeFDParent[0];
  ufds[1].fd = STDIN_FILENO;

  ufds[0].events = POLLIN; //| POLLRDBAND;
  ufds[1].events = POLLIN; //| POLLRDBAND;
}
  
int main(int argc,char* argv[])
{
  shellopt = false;
  char optionChar;
    while( (optionChar = getopt_long(argc, argv, "", long_options, NULL)) != -1) { //if there are still options to parse
    switch(optionChar)
      {
      case's':
	shellopt = true;
	break;
      case '?':
	fprintf(stderr, "Invalid option attempted!\nAvaliable options:\n\t--input=\'arguement\'\n\t--output=\'arguement\'\n\t--segfault\n\t--catch\n");    //todo, add a correct usage line????
	exit(1);
	break;	
      }
    }

    setUpTerminal();

  //each respectively reads from their own pipe[0] and writes to their other pipe [1]
  createPipe(pipeFDParent);
  createPipe(pipeFDChild);
 
  if(shellopt)
    {
      childPID =  fork();
      if(childPID == 0)
	openShell();
    }
  close(pipeFDChild[0]);
  close(pipeFDParent[1]);
  if(SIG_ERR == signal(SIGPIPE, sigPipeHandler))
    {
      fprintf(stderr, "Error, cannot register SIGPIPE handler");
    }
 
  setUpPolls();
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
	    { if(processFinalInput && ufds[i].fd != pipeFDParent[0])
		continue;
	      
	      if (ufds[i].revents & POLLIN)
		{
		  bool fromShell = false;
		  if(i==0)
		    {
		      //char* buf = malloc(sizeof(char)*256);
		      //int bytes;
		      //if(bytes = read(ufds[0].fd, buf, 256))
		      //write(STDOUT_FILENO, buf, bytes);
		      fromShell = true;
		    }
		    readFromWriteTo(ufds[i].fd, STDOUT_FILENO, BUFF_LEN, fromShell);
		}
	      else if (pipeFDParent[0] && processFinalInput) //if we finished, exit
		{
		  exit(0);
		}
	    }
	}
    }
}
*/
