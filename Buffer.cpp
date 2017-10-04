#include <string.h>
#include "Buffer.h"


unsigned char * Buffer::getPayload() {

  return data;
}


bool Buffer::hasBytes(unsigned int bytes) {

  if (len < (bytes - 1)) {
    return false;
  }

  return true;
}


unsigned char Buffer::getByte(unsigned int pos) {

  if (pos >= dlen) {
    return 0;
  }

  return data[pos];
}


void Buffer::addBytes(const unsigned char * bytes, int bLen) {

  if (bLen < 0 || len + bLen < 0) {
    return;
  }

  memcpy(data + len, bytes, bLen);
  len += bLen;
}


void Buffer::reset() {

  len = 0;
}


Buffer::Buffer() {

  len = 0;
  dlen = 256;
}


Buffer::~Buffer() {

}

