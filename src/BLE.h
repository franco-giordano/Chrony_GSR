#ifndef BLE_H_
#define BLE_H_

#include <Arduino.h>

#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

// #include "esp_ota_ops.h"

// #include "config.h"

class BLE;

class BLE
{
public:
  BLE(void);
  ~BLE(void);

  bool begin(const char *localName);
  int updateStatus();
  int howManyBytes();
  boolean sendBLE(String command, String *returnString, boolean blocking);
  boolean sendBLE(String command);
  void deinitBLE();

private:
  String local_name;

  BLEServer *pServer = NULL;

  BLEService *pESPOTAService = NULL;
  BLECharacteristic *pESPOTAIdCharacteristic = NULL;

  BLEService *pService = NULL;
  BLECharacteristic *pVersionCharacteristic = NULL;
  BLECharacteristic *pOtaCharacteristic = NULL;
  BLECharacteristic *pWatchFaceNameCharacteristic = NULL;
  BLECharacteristic *commandCharacteristic = NULL;
  BLECharacteristic *notificationUpdateCharacteristic = NULL;
};

#endif
