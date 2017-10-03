#ifndef SCALE_H
#define SCALE_H

#include <SoftwareSerial.h>
#include <Arduino.h>

class Scale {

  float weight;
  bool weightHasChanged;

  unsigned char battery;
  
  SoftwareSerial * serial = NULL;
  bool connected;
  bool ready;
  bool notificationRequestSent;
  unsigned long lastHeartbeat;

  void serialPrintf(const char *format, ...);
  void readAtData();
  void sendMessage(char msgType, const unsigned char *payload, size_t len);
  void sendEvent(unsigned char *payload, size_t len);
  void sendHeartbeat();
  void sendTare();
  void sendId();
  void sendNotificationRequest();
  int parseAckEvent(unsigned char *payload, size_t len);
  int parseWeightEvent(unsigned char *payload, size_t len);
  int parseBatteryEvent(unsigned char *payload, size_t len);
  int parseScaleEvent(unsigned char *payload, size_t len);
  int parseScaleEvents(unsigned char *payload, size_t len);
  int parseInfo(unsigned char *payload, size_t len);
  int parseScaleData(int msgType, unsigned char *payload, size_t len);
  int readScaleData(int msgType);
  
public:
  bool hasWeightChanged();
  float getWeight();
  unsigned char getBattery();
  void update();
  void connect(char *mac);
  bool tare();
  Scale::Scale(int txPin, int rxPin);
  ~Scale();
};

#endif

