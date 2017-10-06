#include "Device_HM10.h"

void DeviceHM10::serialPrintf(const char *format, ...) {

  va_list args;
  char buffer[100];

  va_start (args, format);
  vsnprintf (buffer, sizeof(buffer), format, args);
  buffer[sizeof(buffer) - 1] = '\0';

  serial->print(buffer);
}

 
void DeviceHM10::removeBytes(int bLen) {

  buffer->removeBytes(bLen);
}


unsigned char DeviceHM10::getByte(unsigned int pos) {

  return buffer->getByte(pos);
}


unsigned char * DeviceHM10::getPayload() {

  return buffer->getPayload();
}


bool DeviceHM10::hasBytes(unsigned int bytes) {
  
  if (buffer->hasBytes(bytes)) {
    return true;
  }
  
  int bAvailable = bytes - buffer->getLen();

  
  // do not loop - try to keep the buffer data queue small
  if (serial->available()) {
    if (bAvailable > buffer->getFreeLen()) {
      bAvailable = buffer->getFreeLen();
    }

    int total = serial->readBytes(buffer->getPayload() + buffer->getLen(), bAvailable);
    if (total > 0) {
      buffer->addByteCount(bAvailable);
    }
  }
  
  if (buffer->hasBytes(bytes)) {
    return true;
  }

  return false;
}


void DeviceHM10::write(const unsigned char *payload, int len) {

  if (!connected) {
    return;
  }

  serial->write(payload, len);
}


bool DeviceHM10::reset(const char * message) {

  Serial.println(message);

  connected = false;
  newConnection = false;
  buffer->reset();

  return false;
}


bool DeviceHM10::isNewConnection() {
  
  if (newConnection == true) {
    newConnection = false;
    return true;
  }

  return false;
}


bool DeviceHM10::isConnected() {

  if (connected) {
    return true;
  }

  return true;
}


void DeviceHM10::connect() {

  if (connected) {
    return;
  }

  serial->print("AT");
  // reset to factory
  serial->print("AT+RENEW");
  // stop auto connect
  serial->print("AT+IMME1");
  serial->print("AT+MODE0");
  serial->print("AT+COMP1");
  serial->print("AT+UUID0x1800");
  serial->print("AT+CHAR0x2A80");
  // central mode
  serial->print("AT+ROLE1");
  // delay required to register new mode
  delay(1000);
  serialPrintf("AT+CO0%s", mac);
  
  connected = true;
  newConnection = true;
}


void DeviceHM10::disconnect() {

  if (!this->connected) {
    return;
  }

  reset("disconnect device");
}


void DeviceHM10::init() {

#if defined(HAVE_HWSERIAL1)
  serial = &Serial1;
#else
  //serial = new SoftwareSerial(TX_PIN, RX_PIN);
  serial = new AltSoftSerial(TX_PIN, RX_PIN);
#endif

  serial->begin(9600);
}


DeviceHM10::DeviceHM10() {

  buffer = new Buffer();
  connected = false;
  newConnection = false;
}


DeviceHM10::~DeviceHM10() {

  delete(buffer);
}


