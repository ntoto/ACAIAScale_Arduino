#ifndef DEVICE_H_
#define DEVICE_H_

#include "Buffer.h"

class Device {
  
public:
  const char *service = "1820";
  const char *characteristic = "2a80";

  virtual bool isNewConnection() = 0;
  virtual bool isConnected() = 0;
  virtual void connect() = 0;
  virtual void disconnect() = 0;
  virtual void init() = 0;
  virtual void removeBytes(int bLen) = 0;
  virtual unsigned char getByte(unsigned int pos) = 0;
  virtual unsigned char * getPayload() = 0;
  virtual bool hasBytes(unsigned int bytes) = 0;
  virtual void write(const unsigned char *payload, int len) = 0;
};

#endif

