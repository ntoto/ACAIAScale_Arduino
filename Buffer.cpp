#include <string.h>
#include "Buffer.h"

#include <Arduino.h>

unsigned char * Buffer::getPayload() {

  return data;
}



int Buffer::getLen() {

  return len;
}



int Buffer::getFreeLen() {

  return dlen - len;
}


bool Buffer::hasBytes(unsigned int bytes) {

  if (len < bytes) {
    return false;
  }

  return true;
}


unsigned char Buffer::getByte(unsigned int pos) {

  if (pos >= len) {
    return 0;
  }

  return data[pos];
}


void Buffer::addBytes(const unsigned char * bytes, int bLen) {

  if ((bLen < 0) || ((len + bLen) < 0) || ((len + bLen) > dlen)) {
    return;
  }

  memcpy(data + len, bytes, bLen);
  len += bLen;
}



void Buffer::addByteCount(int bLen) {

   len += bLen;
}

void Buffer::removeBytes(int bLen) {

  if (bLen <= 0) {
    return;
  }

  if (bLen >= len) {
    len = 0;
    return;
  }

  for (int i = 0; i < len - bLen; i++) {
    data[i] = data[bLen + i];
  }

  len = len - bLen;
}


void Buffer::reset() {

  len = 0;
}


Buffer::Buffer() {

  len = 0;
  dlen = sizeof(data);
}


Buffer::~Buffer() {

}


