#ifndef JJP_H
#define JJP_H
/**
JJP socket
used in same way as UDP or TCP socket
 **/
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <list>
#include <sstream>
#include <queue>
#include <time.h>

class JJP {
 public:
  JJP(int domain, int type, int protocol); //like socket defintion
  ~JJP();
  int setsockopt(int level, int optname, const void *optval, socklen_t optlen);
  int bind(const struct sockaddr *addr, socklen_t addrlen);
  int listen(int backlog);
  void processing_thread(int newsockfd);
  int accept(struct sockaddr *addr, socklen_t * addrlen);
  int connect(const struct sockaddr *addr, socklen_t addrlen);

  ssize_t write(const void *buf, size_t nbytes);
  ssize_t read(void *buf, size_t nbytes);

 private:
  /**
     thread psuedo code:
        read_from_UDP_and_translate_to_packet
	process_packet
	if_ack
	  notify_sender

	get_avaliable_space
	create_packet_with_len
	send_packet
   **/


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
    size_t create_data_packet(char** buf, uint32_t len, uint16_t sequence_number);
    size_t create_FIN(char** packet, uint16_t seq_num);
    size_t create_ACK(char** packet, uint16_t seq_num);
    size_t create_SYN(char** packet, uint16_t seq_num);
    size_t create_update(char** packet, uint16_t seq_num);
    void set_rwnd(size_t new_rwnd) {rwnd=new_rwnd;}
  private:
    //given x bytes of data, add a header to it
    char* create_header(uint32_t packet_length, uint16_t sequence_number, uint16_t acknowledgement_num, uint16_t receiver_window, bool isACK, bool isFIN, bool isSYN);
    size_t size();
    size_t rwnd;
    std::stringstream bufSS;
    size_t bufLen;
    static const size_t headerLen = sizeof(char)*12; //96 bit header
  };
  /**
  Used to send and retransmit data if necessary. Will receive packets from Packer and put into queue to send. Will keep account of packets until ACK has been received for the specific packet.
   **/
  class Sender {
  public:
    Sender();
    size_t get_avaliable_space();
    size_t send(char* packet, size_t packet_len, uint16_t seq_num);
    size_t send_update(char * packet, size_t packet_len); //just to send updates when rwnd is 0
    void resend_expired_packets();
    void set_sockfd(int sockfd) {mSockfd = sockfd;}
    void update_cwnd(size_t new_wnd) {cwnd = new_wnd;}
    void update_rwnd(size_t new_wnd) {rwnd = new_wnd;}
    void notify_ACK(uint16_t seq_num);
  private:
    // size_t send_packet(char* packet, size_t packet_len);
    size_t max_buf_size();

    class PacketObj {
    public:
      PacketObj(char* pack, size_t pack_len, uint16_t seq_num) {
	sequence_num = seq_num;
	packet = pack;
	packet_len = pack_len;
	isAcked = false;
	time(&sent_time);
      }

      time_t sent_time;
      uint16_t sequence_num;
      char* packet;
      size_t packet_len;
      bool isAcked;
    };

    std::list<PacketObj> packet_buffer;
    //size_t max_size;
    int mSockfd;
    bool packet_has_timed_out(PacketObj packet_obj) {
      time_t now = time(0);
      if(difftime(now, packet_obj.sent_time)*100 > 500) {
	return true;
      }
      return false;
    }

    const double timeout_ms = 500;
    size_t next_byte;
    //char m_buf[5120];
    //char* BUF; // ACK + min(rwnd, cwnd)
    //char* ACK; //last Acked byte
    //char* NEXT; //last sent byte
    size_t cwnd; //5120 bytes usually
    size_t rwnd; //0-5120 bytes
  };

  /**
   Used to turn packets back into bytes. Returns if packet is valid
   **/

   class Receiver{
   public:
     Receiver(); //init packet num
     // returns -1 if invalid data, 0 if valid data, 1 if ACK
     int receive_packet(char* packet, size_t packet_len, uint16_t &acknowledgement_num, uint16_t &receiver_window);
     //if data, send ACK (telegraph to JJP that we received data, ie return true)
     //if ACK, notify sender that packet has been successfully acked
     //if data
     //put into temporary buffer (update avaliable space)

     size_t read(void *buf, size_t nbytes); //read from stored data
     //read from sstring, either up to nbytes or x bytes. return x bytes.
     size_t get_avaliable_space();
     //(5120) total space in bufer - sum of packet size in temp storage
     void set_sockfd(int sockfd) {mSockfd = sockfd;}

   private:
     //update_temporary_storage //transfer valid data from temp storage to sstream
     struct packetPair
     {
         uint16_t seq_num;
         char* packet;
         size_t packet_len;

         packetPair(uint16_t givenSeqNum, char* givenPacket, size_t givenPacketLen)  {
           seq_num = givenSeqNum;
           packet = givenPacket;
           packet_len = givenPacketLen;
         }

         bool operator<(const struct packetPair& other) const
         {  return seq_num < other.seq_num; }
     };

     //next expected packet, init to 0
     int mSockfd;
     uint16_t expected_packet_num;
     std::stringstream bufSS;
     std::priority_queue<packetPair> storage_queue;
     size_t used_space;
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
