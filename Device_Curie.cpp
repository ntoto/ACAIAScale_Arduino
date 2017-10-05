#if defined(__arc__)

#include "Device_Curie.h"

void DeviceCurie::removeBytes(int bLen) {

  buffer->removeBytes(bLen);
}


unsigned char DeviceCurie::getByte(unsigned int pos) {

  return buffer->getByte(pos);
}


unsigned char * DeviceCurie::getPayload() {

  return buffer->getPayload();
}


bool DeviceCurie::hasBytes(unsigned int bytes) {

  if (buffer->hasBytes(bytes)) {
    return true;
  }

  // do not loop - try to keep the buffer data queue small
  if (characteristic.valueUpdated()) {
    buffer->addBytes(characteristic.value(), characteristic.valueLength()); 
    if (buffer->hasBytes(bytes)) {
      return true;
    }
  }

  return false;
}


void DeviceCurie::write(const unsigned char *payload, int len) {

  if (!connected) {
    return;
  }

  characteristic.writeValue(payload, len);
}


bool DeviceCurie::reset(const char * message) {

  Serial.println(message);
  
  if (peripheral.connected()) {
    peripheral.disconnect();
  }

  connected = false;
  newConnection = false;
  buffer->reset();
  BLE.scanForUuid(service);

  return false;
}


bool DeviceCurie::isNewConnection() {
  
  if (newConnection == true) {
    newConnection = false;
    return true;
  }

  return false;
}


bool DeviceCurie::isConnected() {

  if (connected && peripheral.connected()) {
    return true;
  }

  if (connected) {
    return reset("device disconnected");
  }
  
  peripheral = BLE.available();
  if (!peripheral) {
    return false;
  }

  BLE.stopScan();
  
  if (!peripheral.connect()) {
    return reset("failed to connect");
  }
  
  if (!peripheral.discoverAttributes()) {
    return reset("failed to discover attributes");
  }

  characteristic = peripheral.characteristic(characteristic);
  if (!characteristic) {
    return reset("failed to get characteristic");
  }
  
  characteristic.subscribe();
  connected = true;
  newConnection = true;

  return true;
}


void DeviceCurie::connect() {

  if (connected) {
    return;
  }
    
  BLE.scanForUuid(service);
}


void DeviceCurie::disconnect() {

  if (!this->connected) {
    return;
  }

  reset("disconnect device");
  BLE.stopScan();
}


void DeviceCurie::init() {

  BLE.begin();
}


DeviceCurie::DeviceCurie() {

  buffer = new Buffer();
  connected = false;
  newConnection = false;
}


DeviceCurie::~DeviceCurie() {

  delete(buffer);
}

#endif

