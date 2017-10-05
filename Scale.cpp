#include "Scale.h"

#include <Arduino.h>

#if defined (__arc__)
#include "Device_Curie.h"
#else
#include "Device_HM10.h"
#endif

#define HEADER1 0xef
#define HEADER2 0xdd

#define MSG_SYSTEM 0
#define MSG_TARE 4
#define MSG_INFO 7
#define MSG_STATUS 8
#define MSG_IDENTIFY 11
#define MSG_EVENT 12
#define MSG_TIMER 13

#define EVENT_WEIGHT 5
#define EVENT_BATTERY 6
#define EVENT_TIMER 7
#define EVENT_KEY 8
#define EVENT_ACK 11

#define EVENT_WEIGHT_LEN 6
#define EVENT_BATTERY_LEN 1
#define EVENT_TIMER_LEN 3
#define EVENT_KEY_LEN 1
#define EVENT_ACK_LEN 2

#define TIMER_START 0
#define TIMER_STOP 1
#define TIMER_PAUSE 2

#define READ_HEADER 0
#define READ_DATA 1

#define HEADER_SIZE 3

#define TIMER_STATE_STOPPED 0
#define TIMER_STATE_STARTED 1
#define TIMER_STATE_PAUSED 2

void Scale::printf(const char *format, ...) {

  va_list args;
  char buffer[100];

  va_start (args, format);
  vsnprintf (buffer, sizeof(buffer), format, args);
  buffer[sizeof(buffer) - 1] = '\0';

  Serial.print(buffer);
}


void Scale::sendMessage(char msgType, const unsigned char *payload, size_t len) {

  // TODO: malloc can return NULL
  unsigned char *bytes = (unsigned char *)malloc(5 + len);
  unsigned char cksum1 = 0;
  unsigned char cksum2 = 0;
  unsigned int i;

  bytes[0] = HEADER1;
  bytes[1] = HEADER2;
  bytes[2] = msgType;


  for (i = 0; i < len; i++) {
    unsigned char val = payload[i] & 0xff;
    bytes[3+i] = val;
    if (i % 2 == 0) {
      cksum1 += val;
    }
    else {
      cksum2 += val;
    }
  }

  bytes[len + 3] = (cksum1 & 0xFF);
  bytes[len + 4] = (cksum2 & 0xFF);


  device->write(bytes, len + 5);  
  free(bytes);
}


void Scale::sendEvent(unsigned char *payload, size_t len) {

  unsigned int i;
  // TODO: malloc can return NULL
  unsigned char *bytes = (unsigned char*)malloc(len + 1);
  bytes[0] = len + 1;

  for (i = 0; i < len; i++) {
    bytes[i+1] = payload[i] & 0xff;
  }

  sendMessage(MSG_EVENT, bytes, len+1);
  free(bytes);
}


void Scale::sendHeartbeat() {

  unsigned long now = millis();

  if (lastHeartbeat + 3000 > now) {
    return;
  }
    
  unsigned char payload[] = {0x02,0x00};
  sendMessage(MSG_SYSTEM, payload, sizeof(payload));
  this->lastHeartbeat = now;
}


void Scale::sendTare() {

  unsigned char payload[] = {0x00};
  sendMessage(MSG_TARE, payload, sizeof(payload));
}


void Scale::sendTimerCommand(unsigned char command) {

  unsigned char payload[] = {0x00, command};
  sendMessage(MSG_TIMER, payload, sizeof(payload));
}


void Scale::sendId() {
  
  unsigned char payload[] = {0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d};
  sendMessage(MSG_IDENTIFY, payload, sizeof(payload));
}


void Scale::sendNotificationRequest() {
  
    unsigned char payload[] = {
      0,  // weight
      1,  // weight argument
      1,  // battery
      2,  // battery argument
      2,  // timer
      5,  // timer argument
      3,  // key
      4   // setting
    };

    sendEvent(payload, sizeof(payload));
    this->ready = true;
}


void dump(const char * msg, const unsigned char * payload, size_t len) {

  Serial.print(msg);
  
  for (unsigned int i = 0; i < len; i++) {
    char buf[4];
    snprintf(buf, sizeof(buf), "%.2X", payload[i]);
    Serial.print(buf);
  }

  Serial.println("");
}


int Scale::parseWeightEvent(unsigned char *payload, size_t len) {

  if (len < EVENT_WEIGHT_LEN) {
    dump("Invalid weight event length: ", payload, len);
    return -1;
  }

  float value = (unsigned int) (((payload[1] & 0xff) << 8) + (payload[0] & 0xff));
  int unit = payload[4] & 0xFF;

  if (unit == 1) {
    value /= 10;
  }
  else if (unit == 2) {
    value /= 100;
  }
  else if (unit == 3) {
    value /= 1000;
  }
  else if (unit == 4) {
    value /= 10000;
  }

  if ((payload[5] & 0x02) == 0x02) {
    value *= -1;
  }

  this->weight = value;
  this->weightHasChanged = true;

  return EVENT_WEIGHT_LEN;
}


int Scale::parseAckEvent(unsigned char *payload, size_t len) {

  if (len < EVENT_ACK_LEN) {
    dump("Invalid ack event length: ", payload, len);
    return -1;
  }

  // ignore ack
  
  return EVENT_ACK_LEN;
}


int Scale::parseKeyEvent(unsigned char *payload, size_t len) {

  if (len < EVENT_KEY_LEN) {
    dump("Invalid ack event length: ", payload, len);
    return -1;
  }

  // ignore key event
  
  return EVENT_KEY_LEN;
}


int Scale::parseBatteryEvent(unsigned char *payload, size_t len) {

  if (len < EVENT_BATTERY_LEN) {
    dump("Invalid battery event length: ", payload, len);
    return -1;
  }

  this->battery = payload[0];
  
  return EVENT_BATTERY_LEN;
}


int Scale::parseTimerEvent(unsigned char *payload, size_t len) {

  if (len < EVENT_TIMER_LEN) {
    dump("Invalid timer event length: ", payload, len);
    return -1;
  }

  this->minutes = payload[0];
  this->seconds = payload[1];
  this->mseconds = payload[2];
  
  return EVENT_TIMER_LEN;
}


// returns last position in payload
int Scale::parseScaleEvent(unsigned char *payload, size_t len) {

  int event = payload[0];
  unsigned char *ptr = payload + 1;
  size_t ptrLen = len - 1;
  int val = 0;

  switch(event) {

    case EVENT_WEIGHT:
      val = parseWeightEvent(ptr, ptrLen);
      break;

    case EVENT_BATTERY:
      val = parseBatteryEvent(ptr, ptrLen);
      break;

    case EVENT_TIMER:
      val = parseTimerEvent(ptr, ptrLen);
      break;
      
    case EVENT_ACK:
      val = parseAckEvent(ptr, ptrLen);
      break;
      
    case EVENT_KEY:
      val = parseKeyEvent(ptr, ptrLen);
      break;

    default:
      dump("Unknown event: ", payload, len);
      return -1;
  }

  if (val < 0) {
    return -1;
  }

  return val + 1;
}


int Scale::parseScaleEvents(unsigned char *payload, size_t len) {

  unsigned int lastPos = 0;
  while (lastPos < len) {
    int pos = parseScaleEvent(payload + lastPos, len - lastPos);
    if (pos < 0) {
      return -1;
    }

    lastPos += pos;
  }

  return 0;
}


int Scale::parseInfo(unsigned char *payload, size_t len) {

  this->battery = payload[4];
  // TODO parse other infos
  
  return 0;
}


int Scale::parseScaleData(int msgType, unsigned char *payload, size_t len) {

  int ret = 0;
  
  switch(msgType) {
    case MSG_INFO:
      ret = parseInfo(payload, len);
      sendId();
      break;

    case MSG_STATUS:
      if (device->isNewConnection()) {
        sendNotificationRequest();
      }
      break;

    case MSG_EVENT:
      ret = parseScaleEvents(payload, len);
      break;
      
    default:
      break;
  }

  return ret;
}


void Scale::update() {

  unsigned char * header = NULL;

  if (!device->isConnected()) {
    return;
  }
  
  sendHeartbeat();

  while (1) {

    if (state == READ_HEADER) {
      if (!device->hasBytes(HEADER_SIZE)) {
        break;
      }

      header = device->getPayload();
      if (header[0] != HEADER1 || header[1] != HEADER2) {
        dump("invalid header: ", header, HEADER_SIZE);
        device->removeBytes(1);
        continue;
      }

      state = READ_DATA;
    }
    else {
      if (!device->hasBytes(HEADER_SIZE + 1)) {
        break;
      }

      unsigned char msgType = device->getByte(2);
      unsigned char len = 0;
      unsigned char offset = 0;
      
      if (msgType == MSG_STATUS || msgType == MSG_EVENT || msgType == MSG_INFO) {
        len = device->getByte(3);
        if (len == 0) {
          len = 1;
        }
        offset = 1;
      }
      else {
        switch(msgType) {
        case 0:
          len = 2;
          break;
        
        default:
          len = 0;
        }
      }
      
      if (!device->hasBytes(HEADER_SIZE + len + 2)) {
        break;
      }

      parseScaleData(msgType, device->getPayload() + HEADER_SIZE + offset, len - offset);
      device->removeBytes(HEADER_SIZE + len + 2);
      state = READ_HEADER;
    }
  }
}


void Scale::connect() {

  device->connect();
}


void Scale::disconnect() {

  device->disconnect();
}


bool Scale::tare() {

  if (!ready) {
    return false;
  }

  sendTare();
  return true;
}


// TODO:: track internal scale state to prevent sending
// multiple start/stop commands. That seems to cause some
// issues with the packets received from the scale (BAD)

bool Scale::startTimer() {

  if (!ready) {
    return false;
  }

  if (timerState == TIMER_STATE_STARTED) {
    return false;
  }

  sendTimerCommand(TIMER_START);
  timerState = TIMER_STATE_STARTED;
  
  return true;
}


bool Scale::stopTimer() {

  if (!ready) {
    return false;
  }
  
  if (timerState == TIMER_STATE_STOPPED) {
    return false;
  }

  sendTimerCommand(TIMER_STOP);
  timerState = TIMER_STATE_STOPPED;
  
  return true;
}


bool Scale::pauseTimer() {

  if (!ready) {
    return false;
  }

  
  if (timerState == TIMER_STATE_PAUSED) {
    return false;
  }
  
  sendTimerCommand(TIMER_PAUSE);
  timerState = TIMER_STATE_PAUSED;
  
  return true;
}


bool Scale::hasWeightChanged() {

  return weightHasChanged;
}


float Scale::getWeight() {

  weightHasChanged = false;
  return this->weight;
}


unsigned char Scale::getBattery() {

  return this->battery;
}


unsigned char Scale::getSeconds() {

  return this->seconds;
}


Scale::Scale() {

  this->state = READ_HEADER;
  this->ready = false;
  this->weight = 0;
  this->weightHasChanged = false;
  this->battery = 0;
  this->lastHeartbeat = 0;
  this->timerState = TIMER_STATE_STOPPED;

  this->minutes = 0;
  this->seconds = 0;
  this->mseconds = 0;

#if defined (__arc__)
  device = (Device *)new DeviceCurie();
#else
  device = (Device *)new DeviceHM10();
#endif
  
  device->init();
}


