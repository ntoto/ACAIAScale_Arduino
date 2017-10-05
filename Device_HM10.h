#ifndef DEVICE_HM10_H_
#define DEVICE_HM10_H_

#include "Device.h"
#include <SoftwareSerial.h>


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

