
#include <stdio.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include "JJP.h"

/****
JJP public
****/
JJP::JJP(int domain, int type, int protocol){
  mSockfd = ::socket(domain, type, protocol);
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
  int retVal = ::accept(mSockfd, addr, addrlen);
  ///
  //todo create new thread that constantly reads and writes from socket and does stuff
  ///
  return retVal;
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
size_t Packer::create_data_packet(char** buf, uint32_t len, uint16_t sequence_number){
  size_t dataLen = len - headerLen;
  if(dataLen <= 0 || dataLen > 1024) { //if we don't have enough space to put any data, or pa\
cket is too big return a nullptr                                                              
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

  char* header = create_header(totalPacketSize, sequence_number, (uint16_t) 0, (uint16_t) 0, \
			       false, false, false);

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
    flags = flags & (0x1<<15); //flag is 15th bit
  }
  if(isFIN) {
    flags = flags &(0x1<<14); //flag is 14th bit
  }
  if (isSYN) {
    flags = flags &(0x1<<13); //flag is 13th bit
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

}
size_t JJP::Sender::send(char* packet, size_t packet_len) {

}

/****
Sender private
****/
size_t JJP::Sender::send_packet(char* packet, size_t packet_len){

}

char* JJP::Sender::updated_buf_ptr(){

}

/****
Receiver public
 ****/
JJP::Receiver::Receiver() {

}

bool JJP::Receiver::receive_packet() {

}

size_t JJP::Receiver::read(void *buf, size_t nbytes){

}

size_t JJP::Receiver::get_avaliable_space(){

}

/****
Receiver private
 ****/
