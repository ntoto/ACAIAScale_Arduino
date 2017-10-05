#ifndef BUFFER_H
#define BUFFER_H

class Buffer {

  unsigned char data[40];
  int len;
  int dlen;
  
public:
  Buffer();
  ~Buffer();
  unsigned char * getPayload();
  int getLen();
  int getFreeLen();
  unsigned char getByte(unsigned int pos);
  bool hasBytes(unsigned int bytes);
  void addBytes(const unsigned char * bytes, int bLen);
  void addByteCount(int bLen);
  void removeBytes(int bLen);
  void reset();
};

#endif

