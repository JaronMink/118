#ifndef JJP_H
#define JJP_H
/**
JJP socket
used in same way as UDP or TCP socket
 **/
#include<stdio.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<string.h>
#include<sstream>

class JJP {
 public:
  JJP(int domain, int type, int protocol); //like socket defintion
  int setsockopt(int level, int optname, const void *optval, socklen_t optlen);
  int bind(const struct sockaddr *addr, socklen_t addrlen);
  int listen(int backlog);
  int accept(struct sockaddr *addr, socklen_t * addrlen);
  int connect(const struct sockaddr *addr, socklen_t addrlen);

  ssize_t write(const void *buf, size_t nbytes);
  ssize_t read(void *buf, size_t nbytes);
  
 private:
  /*
  class JJP_Packet {
  public:
    JJP_Packet(char* packetDataBuf, size_t len);
    size();
    isACK();
    getPacketNum();
    
    }*/ //don't know if we need this.
  /**
   Used to store data and form packets to be sent when requested by the sender
   **/
  class Packer {
  public:
    Packer();
    //store data to be send
    size_t store(const char* str, size_t len);
    //create a packet of total size len, sets a pointer buf to packet and returns size of it
    size_t create_data_packet(char* buf, uint32_t len, uint16_t sequence_number);
  
  private:
    //given x bytes of data, add a header to it
    char* create_header(uint32_t packet_length, uint16_t sequence_number, uint16_t acknowledgement_num, uint16_t receiver_window, bool isACK, bool isFIN, bool isSYN);
    size_t size();
    std::stringstream bufSS;
    size_t bufLen;
    static const size_t headerLen = sizeof(char)*12; //96 bit header
  };
  /**
  Used to send and retransmit data if necessary. Will receive packets from Packer and put into queue to send. Will keep account of packets until ACK has been received for the specific packet.   
   **/
  class Sender {
  public:
    //Sender();
    size_t get_avaliable_space();
    size_t send(char* packet, size_t packet_len);
  
  private:
    size_t send_packet(char* packet, size_t packet_len);
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
    
  };
  
  Packer mPacker;
  Sender mSender;
  Receiver mReceiver;
  int mSockfd;
};
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
