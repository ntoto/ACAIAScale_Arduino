#ifndef DEVICE_HM10_H_
#define DEVICE_HM10_H_

#include <Arduino.h>
#include "Device.h"

#if defined(HAVE_HWSERIAL1)
#include <HardwareSerial.h>
#else
// NOTE: The SoftwareSerial is too slow and the notifications too fast
// every now and then the internal circular buffer (64bytes max) is 
// full and some data is overwritten/lost
// we end up with invalid packet. 
// The DeviceHM10 implementation can recover from that - 1 byte is removed
// at a time until the correct header is found.
// #include <SoftwareSerial.h>

// AltSoftSerial uses two separate (non circular) buffer or RX (80 bytes) and RX
// It is faster than SoftwareSerial and does not seem to lose packets

#include <AltSoftSerial.h>

#define TX_PIN 8
#define RX_PIN 9
#endif

class DeviceHM10 : public Device {

  Buffer * buffer;
  bool connected = false;
  bool newConnection = false;

  const char * mac = "001C9714F68E";
  
#if defined(HAVE_HWSERIAL1)
  HardwareSerial * serial = NULL;
#else
  //SoftwareSerial * serial = NULL;
  AltSoftSerial * serial = NULL;
#endif
  
  bool reset(const char * message);
  void serialPrintf(const char *format, ...);
  bool sendCommand(const char *cmd, const char *value);
  
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

