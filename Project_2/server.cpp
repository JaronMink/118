using namespace std;
class Packer {
  public:
    void store(const char* str, size_t len) {

    }

    vector<char> create_packet(size_t size) {

    }
  private:
    vector<char> packet_setup(const char* str, size_t len) {

    }

  vector<char> m_buf;
};

class Sender {
  public:
    int get_available_space() {

    }
    void send() {

    }

  private:
    void send_packet() {

    }

    void set_buf_ptr() {
      buf = (min(cwnd + ack, rwnd + ack) % 5120 ) + m_buf;
    }


  char m_buf[5120];
  char* buf = m_buf + 5120;
  char* ack = m_buf;
  char* next = m_buf;
  int cwnd = 5120;
  int rwnd = 5120;
};

class JJP {
  int write(const char* str, size_t len) {

  }

};

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno, recvlen;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    //while(1) {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);  // create socket
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
      recvlen = recvfrom(sockfd, (struct sockaddr *) &cli_addr, &clilen);

      if (newsockfd < 0)
	      error("ERROR on accept");

      int n;
      char buffer[512];

      memset(buffer, 0, 512);  // reset memory

      //read client's message
      while((n = read(newsockfd, buffer, 511)) == 0);
      if (n < 0) error("ERROR reading from socket");
      printf("Received Message:\n%s\n", buffer);

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
