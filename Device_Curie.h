#ifndef DEVICE_CURIE_H_
#define DEVICE_CURIE_H_


#include "Device.h"
#include <CurieBLE.h>

class DeviceCurie : public Device {

  Buffer * buffer;
  bool connected = false;
  bool newConnection = false;
  
  BLEDevice peripheral;
  BLECharacteristic characteristic;

  bool reset(const char * message);
  
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
  DeviceCurie();
  ~DeviceCurie();
};

#endif

