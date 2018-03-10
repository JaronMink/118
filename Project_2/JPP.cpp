#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include "JPP.h"
using namespace std;

/****
JPP public
****/
JPP::JPP(int domain, int type, int protocol){
  mSockfd = socket(domain, type, protocol);
}

int JPP::setsockopt(int level, int optname, const void *optval, socklen_t optlen){
  return setsockopt(mSockfd, level, optval, optlen);
}

int JPP::bind(const struct sockaddr *addr, socklen_t addrelen){
  return bind(mSockfd, addr, addrlen);
}

int JPP::listen(int backlog){
  return listen(mSockfd, backlog);
}

int JPP::accept(struct sockaddr *addr, socklen_t * addrlen){
  int retVal = accept(mSockfd, addr, addrlen);
  ///
  //todo create new thread that constantly reads and writes from socket and does stuff
  ///
  return retVal
}

int JPP::connect(const struct sockaddr *addr, socklen_t addrlen){
  int retVal = connect(mSockfd, addr, addrlen);
  ///
  //todo create new thread that constantly reads and writes from socket and does stuff
  /// 
  return retVal;
}

ssize_t JPP::write(const void *buf, size_t nbytes) {
  return mPacker.store(str, len);
}

ssize_t JPP::read(void *buf, size_t nbytes) {
  return mReceiver.read(buf, nbytes);
} 


/****
JPP private
****/

/****
Packer public 
****/
void Packer::store(const char* str, size_t len) {
  bufLen+=len; 
  bufSS.write(str, len);
}
//read into buf and return size of packet
size_t Packer::create_packet(char* buf, size_t len){
  size_t dataLen = len - headerLen;
  if(dataLen <= 0 || dataLen > 1024) { //if we don't have enough space to put any data, or packet is too big return a nullptr
    return NULL;
  }

  char* body = malloc(sizeof(char)*dataLen);
  bufSS.read(body, dataLen);
  size_t bytesRead = bufSS.gcount();
  if(bytesRead <= 0) {
    cerr << "Packer: No bytes to read from packer\n";
    return NULL;
  }

  char* header = create_header()
  
}

/****
Packer private
****/
char* Packer::createHeader(int32_t packet_length, int16_t sequence_number, int16_t acknowledgement_num, int16_t receiver_window, bool isACK, bool isFIN, bool isSYN){ //size_t sequenceNum, bool isACK, size_t ackNum, bool isFIN, size_t rcwn, size_t receiverWindow, size_t totalSize) {
  char* header = malloc(sizeof(char)*12); //96 bit header
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
  header[0] = packet_length;
  header[4] = sequence_number;
  header[6] = acknowledgement_num;
  header[8] = receiver_window;
  header[10] = flags;

  return header;
}
size_t Packer::size() {
  return bufLen;
}
char* Packer::setup_packet(const char* str, size_t len){

}

/****
Sender public
****/
size_t Sender::get_avaliable_space(){

}
size_t Sender::send(char* packet, size_t packet_len);

/****
Sender private
****/
size_t Sender::send_packet(char* packet, size_t packet_len){

}

char* Sender::updated_buf_ptr(){

}

/****
Receiver public
 ****/
Receiver::Receiver() {

}

bool Receiver::receive_packet() {

}

size_t Receiver::read(void *buf, size_t nbytes){

}

size_t Receiver::get_avaliable_space(){

}

/****
Receiver private
 ****/
