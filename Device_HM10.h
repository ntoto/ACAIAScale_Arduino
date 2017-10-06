#ifndef DEVICE_HM10_H_
#define DEVICE_HM10_H_

#include "Device.h"
#include <SoftwareSerial.h>


// NOTE: The SoftwareSerial is too slow and the notifications too fast
// every now and then the internal circular buffer (64bytes max) is 
// full and some data is overwritten/lost
// we end up with invalid packet. 
// The DeviceHM10 implementation can recover from that - 1 byte is removed
// at a time until the correct header is found.

class DeviceHM10 : public Device {

  Buffer * buffer;
  bool connected = false;
  bool newConnection = false;

  const char * mac = "001C9714F68E";

  SoftwareSerial * serial = NULL;
  
  bool reset(const char * message);
  void serialPrintf(const char *format, ...);
  
public:
  bool isNewConnection();
  bool isConnected();
  void connect();
  void disconnect();
  void init();
  void removeBytes(int bLen);
  unsigned char getByte(unsigned int pos);
  unsigned char * getPayload();
  bool hasBytes(unsigned int bytes);
  void write(const unsigned char *payload, int len);
  DeviceHM10();
  ~DeviceHM10();
};

#endif

