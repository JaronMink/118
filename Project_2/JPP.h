#ifndef JPP_H
#define JPP_H
/**
JPP socket
used in same way as UDP or TCP socket
 **/
#include<stdio.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<string.h>
#include<sstream>

using namespace std;

class JPP {
 public:
  JPP(int domain, int type, int protocol); //like socket defintion
  int setsockopt(int level, int optname, const void *optval, socklen_t optlen);
  int bind(const struct sockaddr *addr, socklen_t addrlen);
  int listen(int backlog);
  int accept(struct sockaddr *addr, socklen_t * addrlen);
  int connect(const struct sockaddr *addr, socklen_t addrlen);

  ssize_t write(const void *buf, size_t nbytes);
  ssize_t read(void *buf, size_t nbytes);
  
 private:
  /*
  class JPP_Packet {
  public:
    JPP_Packet(char* packetDataBuf, size_t len);
    size();
    isACK();
    getPacketNum();
    
    }*/ //don't know if we need this.
  /**
   Used to store data and form packets to be sent when requested by the sender
   **/
  class Packer {
  public:
    //store data to be send
    ssize_t store(const char* str, size_t len);
    //create a packet of total size len
    char* create_packet(size_t len);
  
  private:
    //given x bytes of data, add a header to it
    char* setup_packet(const char* str, size_t len);
    sstream bufSS;
    size_t bufLen;
    size_t headerLen = sizeof(char)*12; //96 bit header
  };
  /**
  Used to send and retransmit data if necessary. Will receive packets from Packer and put into queue to send. Will keep account of packets until ACK has been received for the specific packet.   
   **/
  class Sender {
  public:
    Sender();
    size_t get_avaliable_space();
    size_t send(char* packet, size_t packet_len);
  
  private:
    int send_packet(char* packet, size_t packet_len);
    char* updated_buf_ptr();
    
    char m_buf[5120];
    char* buf;
    char* ack; 
    char* next; 
    int cwnd;
    int rwnd;
  };
  
  /**
   Used to turn packets back into bytes. Returns if packet is valid   
   **/
 
  class Receiver{
  public:
    Receiver();
    bool receive_packet(); //bool returns if valid
    size_t read(void *buf, size_t nbytes); //read from stored data
    size_t get_avaliable_space();
  private:
    
  }
  
  Packer mPacker;
  Sender mSender;
  Receiver mReceiver;
  int mSockfd;
}
#endif


/**
Max packet size = 1024 bytes
window size = 5120 bytes
max sequence num = 30720
retransmission time = 500ms

HEADER SPECS -96 bits total (12 bytes)

DataLen(32bits),
seqNum(16bits), ackNum(16bits)
rcwn(16bits),ACK(1bit),fin(1bit),Syn(1bit),unused(13bits)
**/
