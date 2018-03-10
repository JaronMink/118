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
  
}
char* Packer::create_packet(size_t len){

}

/****
Packer private
****/

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
