#include "Scale.h"

#define HEADER1 0xef
#define HEADER2 0xdd

#define MSG_SYSTEM 0
#define MSG_TARE 4
#define MSG_INFO 7
#define MSG_STATUS 8
#define MSG_IDENTIFY 11
#define MSG_EVENT 12

#define EVENT_WEIGHT 5
#define EVENT_BATTERY 6
#define EVENT_ACK 11

#define WEIGHT_EVENT_LEN 6
#define ACK_EVENT_LEN 2
#define BATTERY_EVENT_LEN 1


void Scale::serialPrintf(const char *format, ...) {

  va_list args;
  char buffer[100];

  va_start (args, format);
  vsnprintf (buffer, sizeof(buffer), format, args);
  buffer[sizeof(buffer) - 1] = '\0';

  serial->print(buffer);
}


void Scale::readAtData() {
  
}


void Scale::sendMessage(char msgType, const unsigned char *payload, size_t len) {
  
  unsigned char *bytes = (unsigned char *)malloc(5 + len);
  unsigned char cksum1 = 0;
  unsigned char cksum2 = 0;
  int i;

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

  serial->write(bytes, len + 5);
  
  free(bytes);
}


void Scale::sendEvent(unsigned char *payload, size_t len) {

  unsigned int i;
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


void dump(char * msg, unsigned char * payload, size_t len) {

  Serial.print(msg);
  
  for (int i = 0; i < len; i++) {
    char buf[4];
    snprintf(buf, sizeof(buf), "%.2X", payload[i]);
    Serial.print(buf);
  }

  Serial.println("");
}


int Scale::parseWeightEvent(unsigned char *payload, size_t len) {

  if (len < WEIGHT_EVENT_LEN) {
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

  return WEIGHT_EVENT_LEN;
}


int Scale::parseAckEvent(unsigned char *payload, size_t len) {

  if (len < ACK_EVENT_LEN) {
    dump("Invalid ack event length: ", payload, len);
    return -1;
  }

  // ignore ack
  
  return ACK_EVENT_LEN;
}


int Scale::parseBatteryEvent(unsigned char *payload, size_t len) {

  if (len < BATTERY_EVENT_LEN) {
    dump("Invalid battery event length: ", payload, len);
    return -1;
  }

  this->battery = payload[0];
  
  return BATTERY_EVENT_LEN;
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
      
    case EVENT_ACK:
      val = parseAckEvent(ptr, ptrLen);
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

  int lastPos = 0;
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
      if (!notificationRequestSent) {
        sendNotificationRequest();
        notificationRequestSent = true;
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


int Scale::readScaleData(int msgType) {

  unsigned char len = 0;
  unsigned char cksum[2];
  
  if (msgType == 8 || msgType == 12 || msgType == 7) {
    // using readBytes for read timeout
    int val = serial->readBytes(&len, 1);
    if (val < 1) {
      Serial.println("Failed to receive message data length");
      return -1;
    }
    
    len--;
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
    
  unsigned char *payload = (unsigned char*)malloc(len);
  if (serial->readBytes(payload, len) < 0) {
    Serial.println("Failed to receive message data");
    return -1;
  }

  if (serial->readBytes(cksum, 2) < 0) {
    Serial.println("Failed to receive message checksum");
    return -1;
  }

  // TODO: verify checksum

  int ret = parseScaleData(msgType, payload, len);
  free(payload);

  return ret;
}


void Scale::update() {

  // potentially need circular header to keep track of last data set if not enough
  unsigned char header[3];

  if (!this->connected) {
    return;
  }

  sendHeartbeat();

  while (serial->available()) {

    int val = serial->read();
    if (val < 0) {
      return;
    }

    if (val != HEADER1) {
      continue;
    }

    header[0] = val;
    if (serial->readBytes(header+1, sizeof(header)-1) != sizeof(header)-1) {
      return -1;
    }

    if (strncmp(header, "AT+", sizeof(header)) == 0) {
      readAtData();
    }
    else if (header[0] == HEADER1 && header[1] == HEADER2) {
      readScaleData(header[2]);
    }
  }
}

void Scale::connect(char *mac) {

  if (this->connected) {
    return true;
  }
    
  serial->print("AT+IMME1");
  serial->print("AT+MODE0");
  serial->print("AT+COMP1");
  serial->print("AT+UUID0x1800");
  serial->print("AT+CHAR0x2A80");
  serial->print("AT+ROLE1");
  // delay required to register new mode
  delay(1000);
  serialPrintf("AT+CO0%s", mac);

  this->connected = true;
}


bool Scale::tare() {

  if (!ready) {
    return false;
  }

      Serial.println("sending tare");
  sendTare();
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


Scale::Scale(int txPin, int rxPin) {

  this->connected = false;
  this->ready = false;
  this->notificationRequestSent = false;
  this->weight = 0;
  this->weightHasChanged = true;
  this->battery = 0;
  this->lastHeartbeat = 0;

  serial = new SoftwareSerial(txPin, rxPin);
  serial->begin(9600);
}


Scale::~Scale() {
}

