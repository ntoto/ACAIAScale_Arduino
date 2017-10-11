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


void DeviceHM10::dump(const char * msg, const unsigned char * payload, size_t len) {

  Serial.print(msg);
  
  for (unsigned int i = 0; i < len; i++) {
    char buf[4];
    snprintf(buf, sizeof(buf), "%.2X", payload[i]);
    Serial.print(buf);
  }

  Serial.println("");
}


bool DeviceHM10::isDeviceConnected() {

  return status == STATUS_CONNECTED || status == STATUS_DISCONNECTING;
}


bool DeviceHM10::checkConnectionStatus() {
  
  bool connectionStatus = DeviceHM10::isDeviceConnected();
  
  if (buffer->getLen() < 3) {
    int count = serial->readBytes(buffer->getPayload() + buffer->getLen(), 3);
    buffer->addByteCount(count);
    if (count < 3) {
      return connectionStatus;
    }
  }
  
  unsigned char * payload = buffer->getPayload();
  // OK+CONNX / OK+LOST
  if (payload[0] != 'O' || payload[1] != 'K' || payload[2] != '+') {
    return connectionStatus;
  }


  if (buffer->getLen() < 7) {
    int need = 8 - buffer->getLen();
    int count = serial->readBytes(buffer->getPayload() + buffer->getLen(), need);
    buffer->addByteCount(count);
    if (buffer->getLen() < 7) {
      return connectionStatus;
    }
  }

  if (buffer->getLen() > 7) {
    if ( memcmp(buffer->getPayload(), "OK+CONNE", 8) == 0 || 
         memcmp(buffer->getPayload(), "OK+CONNF", 8) == 0 
       ) {
      
      if (status == STATUS_CONNECTING) {
        status = STATUS_INITIALIZING;
      }
      else {
        status = STATUS_DISCONNECTED;
      }
      buffer->removeBytes(8);
      return connectionStatus;
    }

    if (memcmp(buffer->getPayload(), "OK+CONNA", 8) == 0) {
      buffer->removeBytes(8);
      return connectionStatus;
    }
  }
    
  if (memcmp(buffer->getPayload(), "OK+CONN", 7) == 0) {
    buffer->removeBytes(7);
    newConnection = true;
    status = STATUS_CONNECTED;
    Serial.println("device connected");
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
      buffer->addByteCount(total);
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

  if (status != STATUS_CONNECTED) {
    return;
  }

  serial->write(payload, len);
}


bool DeviceHM10::reset(const char * message) {

  Serial.println(message);

  if (status == STATUS_DISCONNECTING) {
    status = STATUS_DISCONNECTED;
  }
  else {
    status = STATUS_INITIALIZING;
  }
  
  newConnection = false;
  serial->flush();
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

  if (status == STATUS_CONNECTED) {
    return true;
  }
  else if (status == STATUS_DISCONNECTED) {
    return false;
  }

  if (status == STATUS_INITIALIZING) {
    if (!sendCommand("CON", mac)) return false;
    status = STATUS_CONNECTING;
  }

  if (status == STATUS_DISCONNECTING) {
    // Empty AT command disconnects the device
    serial->print("AT");
  }

  return checkConnectionStatus();
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
      serialPrintf("AT");
      snprintf(expected, sizeof(expected), "OK");
    }
    else {
      serialPrintf("AT+%s%s", cmd, value);
  
      if (value == NULL || strlen(value) == 0) {
        snprintf(expected, sizeof(expected), "OK+%s", cmd);
      }
      else if (cmd == "CON") {
        return true;
        //snprintf(expected, sizeof(expected), "OK+CONNA");
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

  if (status == STATUS_CONNECTED || status == STATUS_INITIALIZING || status == STATUS_CONNECTING) {
    return;
  }
  
  status = STATUS_INITIALIZING;
  // TODO: Do discovery
}


void DeviceHM10::disconnect() {

  if (status == STATUS_DISCONNECTED) {
    return;
  }

  status = STATUS_DISCONNECTING;
}


boolean DeviceHM10::initDevice() {
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

    return true;
}


void DeviceHM10::init() {

#if defined(HAVE_HWSERIAL1)
  serial = &Serial1;
#else
  //serial = new SoftwareSerial(TX_PIN, RX_PIN);
  serial = new AltSoftSerial(TX_PIN, RX_PIN);
#endif

  serial->begin(9600);

  while (!initDevice()) {
  }
}


DeviceHM10::DeviceHM10() {

  buffer = new Buffer();
  newConnection = false;
  status = STATUS_DISCONNECTED;
}


DeviceHM10::~DeviceHM10() {

  delete(buffer);
}


