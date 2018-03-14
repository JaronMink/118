#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <list>
#include "JJP.h"

/****
JJP public
****/
JJP::JJP(int domain, int type, int protocol){
  mSockfd = ::socket(domain, type, protocol);
}

JJP::~JJP() {
  close(mSockfd);
}

int JJP::setsockopt(int level, int optname, const void *optval, socklen_t optlen){
  return ::setsockopt(mSockfd, level, optname, optval, optlen);
}

int JJP::bind(const struct sockaddr *addr, socklen_t addrlen){
  return ::bind(mSockfd, addr, addrlen);
}

int JJP::listen(int backlog){
  return ::listen(mSockfd, backlog);
}

int JJP::accept(struct sockaddr *addr, socklen_t * addrlen){
  int newsockfd = ::accept(mSockfd, addr, addrlen);

  while (1) {
    int n;
    size_t bytesRead = 0;
    uint16_t ackNum, receiveWindow;
    char buffer[1024];

    memset(buffer, 0, 1024);  // reset memory

    //read client's message
    while((n = ::read(newsockfd, buffer, 1023)) == 0)
      bytesRead += n;
    //if (n < 0) error("ERROR reading from socket");

    int isACKorFIN = mReceiver.receive_packet(buffer, bytesRead, ackNum, receiveWindow);
    printf("Receiving packet %d\n", ackNum);

    if (isACKorFIN)
      {}//mSender.notify_ACK(

    size_t available_space = mSender.get_avaliable_space();
    if (available_space > 0) {
      char* packet[1024];
      size_t sequence_num = 0;
      size_t packet_len = mPacker.create_data_packet(packet, 1024, sequence_num);
      mSender.send(*packet, packet_len, sequence_num);
    }
  }
  return newsockfd;
}

int JJP::connect(const struct sockaddr *addr, socklen_t addrlen){
  int retVal = ::connect(mSockfd, addr, addrlen);
  ///
  //todo create new thread that constantly reads and writes from socket and does stuff
  ///
  return retVal;
}

ssize_t JJP::write(const void *buf, size_t nbytes) {
  return mPacker.store((char*)buf, nbytes);
}

ssize_t JJP::read(void *buf, size_t nbytes) {
  return mReceiver.read(buf, nbytes);
}


/****
JJP private
****/

/****
Packer public
****/
JJP::Packer::Packer() {
  bufLen = 0;
}


size_t JJP::Packer::store(const char* str, size_t len) {
  bufLen+=len;
  bufSS.write(str, len);
  return len; //assume we wrote the whole thing
}
//read into buf and return size of packet
size_t JJP::Packer::create_data_packet(char** buf, uint32_t len, uint16_t sequence_number){
  size_t dataLen = len - headerLen;
  if(dataLen <= 0 || dataLen > (1024 - headerLen)) { //if we don't have enough space to put any data, or packet is too big return a nullptr
  return 0;
  }

  char* body = (char*)malloc(sizeof(char)*dataLen);
  bufSS.read(body, dataLen);
  size_t bytesRead = bufSS.gcount();
  bufLen -= bytesRead;
  uint32_t totalPacketSize = bytesRead + headerLen;
  if(bytesRead <= 0) {
    std::cout << "Packer: No bytes to read from packer\n";
    return 0;
  }

  char* header = create_header(totalPacketSize, sequence_number, (uint16_t) 0, (uint16_t) 0, false, false, false);

  char* packet = (char*) malloc(sizeof(char)*totalPacketSize);
  memmove(packet, header, sizeof(char)*12);
  memmove(packet + 12, body, sizeof(char)*bytesRead);
  *buf = packet;
  free(body);
  free(header);
  return totalPacketSize;
}

/****
Packer private
****/
char* JJP::Packer::create_header(uint32_t packet_length, uint16_t sequence_number, uint16_t acknowledgement_num, uint16_t receiver_window, bool isACK, bool isFIN, bool isSYN){ //size_t sequenceNum, bool isACK, size_t ackNum, bool isFIN, size_t rcwn, size_t receiverWindow, size_t totalSize) {
  char* header = (char *) malloc(sizeof(char)*12); //96 bit header

int16_t flags = 0;
  if(isACK){
    flags = flags | (0x1<<15); //flag is 15th bit
  }
  if(isFIN) {
    flags = flags | (0x1<<14); //flag is 14th bit
  }
  if (isSYN) {
    flags = flags | (0x1<<13); //flag is 13th bit
  }
  //add contents to packet

  memmove(header, (char*)&packet_length, 4);
  memmove(header+4,(char*)&sequence_number, 2);
  memmove(header+6,(char*)&acknowledgement_num, 2);
  memmove(header+8,(char*)&receiver_window, 2);
  memmove(header+10,(char*)&flags, 2);

  return header;
}
size_t JJP::Packer::size() {
  return bufLen;
}

/****
Sender public
****/
size_t JJP::Sender::get_avaliable_space(){
  if(max_buf_size() < next_byte) {
    std::cerr << "error, next_byte greater than max_buf_size\n";
    exit(1);
  }
  return (max_buf_size() - next_byte);
}

/*

 */
size_t JJP::Packer::create_FIN(char** packet, uint16_t seq_num) {
  *packet = create_header(sizeof(char)*12, seq_num, (uint16_t) 0, rwnd, false, true, false);
  return (sizeof(char)*12);
}
size_t JJP::Packer::create_ACK(char** packet, uint16_t seq_num) {
  *packet = create_header(sizeof(char)*12, seq_num, (uint16_t) 0, rwnd, true, false, false);
  return (sizeof(char)*12);
}
size_t JJP::Packer::create_SYN(char** packet, uint16_t seq_num) {
  *packet = create_header(sizeof(char)*12, seq_num, (uint16_t) 0, rwnd, false, false, true);
  return (sizeof(char)*12);
}

size_t JJP::Packer::create_update(char** packet, uint16_t seq_num) {
  *packet = create_header(sizeof(char)*12, seq_num, (uint16_t) 0, rwnd, false, false, false);
  return (sizeof(char)*12);
}

//go through all packets and resend expired ones
void JJP::Sender::resend_expired_packets() {
  for(std::list<PacketObj>::iterator it = packet_buffer.begin(); it != packet_buffer.end(); it++) {
    if(packet_has_timed_out(*it)) {
      send(it->packet, it->packet_len, it->sequence_num );
    }
  }
}


//set isACK for PacketObj that matches and move up window as much as we can
void JJP::Sender::notify_ACK(uint16_t seq_num) {

}



JJP::Sender::Sender() {
  next_byte = 0;
  cwnd = 5120;
  rwnd = 5120;

  mSockfd = -1;
}

size_t JJP::Sender::send(char* packet, size_t packet_len, uint16_t seq_num){
  if(((long)get_avaliable_space() - (long) packet_len) < 0) { //if we don't have enough space to hold packet, do nothing
    return 0;
  }
  //write to socket
  ::write(mSockfd, packet, packet_len);
  //store in object
  PacketObj new_packet_object(packet, packet_len, seq_num);
  packet_buffer.push_back(new_packet_object);
  next_byte += packet_len;


  //set up alarm for res
  return packet_len;

}
/***
Sender Private
 ***/

size_t JJP::Sender::max_buf_size(){
  size_t minThreshold = 5120;
  if(cwnd < rwnd) {
    minThreshold = cwnd;
  } else {
    minThreshold = rwnd;
  }
  return minThreshold;
}

/****
Receiver public
 ****/
JJP::Receiver::Receiver() {
  expected_packet_num = 0;
  used_space = 0;
  mSockfd = -1;
}

int JJP::Receiver::receive_packet(char* packet, size_t packet_len, uint16_t &acknowledgement_num, uint16_t &receiver_window) {
  //if data, send ACK (telegraph to JJP that we received data, ie return true)
  //if ACK, notify sender that packet has been successfully acked
  //if data
  //put into temporary buffer (update avaliable space)
  uint32_t packet_length;
  uint16_t sequence_number;
  bool isACK, isFIN, isSYN;
  int16_t flags = 0;

  char* header = packet;

  memmove((char*)&packet_length, header, 4);
  memmove((char*)&sequence_number, header+4, 2);
  memmove((char*)&acknowledgement_num, header+6, 2);
  memmove((char*)&receiver_window, header+8, 2);
  memmove((char*)&flags, header+10, 2);

  isACK = flags & (0x1<<15);
  isFIN = flags & (0x1<<14);
  isSYN = flags & (0x1<<13);

  int ret_flag = 0;

  if (isACK || isFIN)
    ret_flag = 1;

  packetPair pPair(sequence_number, packet, packet_len);

  if (get_avaliable_space() > 0)
  {
    storage_queue.push(pPair);
    used_space += packet_len;
  }

  while (!storage_queue.empty() && storage_queue.top().seq_num == expected_packet_num) {
    packetPair currPair = storage_queue.top();
    storage_queue.pop();
    used_space -= currPair.packet_len;
    bufSS.write(currPair.packet, currPair.packet_len);
    expected_packet_num = (expected_packet_num + currPair.packet_len) % 30720;
  }

  return ret_flag;
}

size_t JJP::Receiver::read(void *buf, size_t nbytes){
  bufSS.read((char*)buf, nbytes);
  return bufSS.gcount();
}

size_t JJP::Receiver::get_avaliable_space(){
  return 5120 - used_space;
}

/****
Receiver private
 ****/
