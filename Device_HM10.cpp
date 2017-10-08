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
  checkConnectionStatus();
}


unsigned char DeviceHM10::getByte(unsigned int pos) {

  return buffer->getByte(pos);
}


unsigned char * DeviceHM10::getPayload() {

  return buffer->getPayload();
}


bool DeviceHM10::checkConnectionStatus() {

  if (buffer->getLen() < 3) {
    return true;
  }
  
  char * payload = buffer->getPayload();
  // OK+CONN / OK+LOST
  if (payload[0] != 'O' || payload[1] != 'K' || payload[2] != '+') {
    return true;
  }

  if (buffer->getLen() < 7) {
    int total = serial->readBytes(buffer->getPayload() + buffer->getLen(), 7 - buffer->getLen());
    if (total > 0) {
      buffer->addByteCount(total);
    }
  }
  
  if (buffer->getLen() < 7) {
    return true;
  }
  
  if (strncmp(buffer->getPayload(), "OK+CONN", 7) == 0) {
    buffer->removeBytes(7);
    return true;
  }

  // BAD: assuming it is OK+LOST
  return reset("device disconnected");
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

  if (!checkConnectionStatus()) {
    return false;
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

  // purge serial buffer
  while (serial->available()) {
    int val = serial->read();
  }

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

  int count = 0;
  if (connected) {
    return true;
  }
  
  if (!sendCommand("", "")) return false;
  if (!sendCommand("RENEW", "")) return false;
  if (!sendCommand("IMME", "1")) return false;
  if (!sendCommand("MODE", "1")) return false;
  if (!sendCommand("COMP", "1")) return false;
  if (!sendCommand("NOTI", "1")) return false;
  if (!sendCommand("UUID", "0x1800")) return false;
  if (!sendCommand("CHAR", "0x2A80")) return false;
  if (!sendCommand("ROLE", "1")) return false;
  delay(1000);
  if (!sendCommand("CO0", mac)) return false;
  
  connected = true;
  newConnection = true;

  return true;
}


bool DeviceHM10::sendCommand(const char *cmd, const char *value) {

  char buffer[40];
  char expected[40];
  int count = 0;

  while (count < 5) {
    delay(count * 100);
    count++;

    // purge serial buffer
    while (serial->available()) {
      int val = serial->read();
    }
    
    if (cmd == NULL || strlen(cmd) == 0) {
      serialPrintf("AT", cmd, value);
      snprintf(expected, sizeof(expected), "OK");
    }
    else {
      serialPrintf("AT+%s%s", cmd, value);
  
      if (value == NULL || strlen(value) == 0) {
        snprintf(expected, sizeof(expected), "OK+%s", cmd);
      }
      else if (cmd == "CO0") {
        snprintf(expected, sizeof(expected), "OK+CO00A");
      }
      else {
        snprintf(expected, sizeof(expected), "OK+Set:%s", value);
      }
    }
  
    int total = serial->readBytes(buffer, strlen(expected));
  
    if (total >= 0) {
      // read extra bytes still in buffer
      while (serial->available() && (total < sizeof(buffer))) {
        buffer[total++] = serial->read();
      }
    
      buffer[total] = '\0';
    }
    
    if (total <= 0) {
      continue;
    }
    
    if (strcmp(buffer, expected) != 0) {
      Serial.print("command ");
      Serial.print(cmd);
      Serial.print(" failed: ");
      Serial.println(buffer);
      Serial.print("expected: ");
      Serial.println(expected);
      
      return false;
    }
    
    return true;
  }

  Serial.print("failed to get answer for command ");
  Serial.println(cmd);

  return false;
}


void DeviceHM10::connect() {

  if (connected) {
    return;
  }

  // TODO: Do discovery
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


