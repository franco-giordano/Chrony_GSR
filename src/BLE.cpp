#include "BLE.h"

// available commands
//   /notifications - gets current android notifications as a string format "appName,Title;ExtraText,ExtraInfoText,ExtraSubText,ExtraTitle;Description;"
//   /calendar - returns a string of calender events for the next 24 hours in format "title;description;startDate;startTime;endTime;eventLocation;"
//   /time - returns a string representing the time
//   /isPlaying - returns "true" or "false" indicating whether spotify is playing on the android device
//   /currentSong - returns the current song name and artist playing on spotify as one string
//   /play - hits the media play button on the android device
//   /pause - hits the media pause button on the android device
//   /nextSong - hits the media next song button on the android device
//   /lastSong - hits the media previous song button on the android device

// #define SERVICE_UUID_ESPOTA    "cd77498e-1ac8-48b6-aba8-4161c7342fce"
// #define SERVICE_UUID_OTA               "86b12865-4b70-4893-8ce6-9864fc00374d"
// #define CHARACTERISTIC_UUID_ID "cd77498f-1ac8-48b6-aba8-4161c7342fce"
// #define CHARACTERISTIC_UUID_FW         "86b12866-4b70-4893-8ce6-9864fc00374d"
// #define CHARACTERISTIC_UUID_HW_VERSION "86b12867-4b70-4893-8ce6-9864fc00374d"
// #define CHARACTERISTIC_UUID_WATCHFACE_NAME                                     \
//   "86b12868-4b70-4893-8ce6-9864fc00374d"

#define SERVICE_UUID "5ac9bc5e-f8ba-48d4-8908-98b80b566e49"
#define COMMAND_UUID "bcca872f-1a3e-4491-b8ec-bfc93c5dd91a"
// #define CHARACTERISTIC_NOTIFICATION_UPDATE "bcca872f-1a3e-4491-b8ec-bfc93c5dd901"

#define FULL_PACKET 512
#define CHARPOS_UPDATE_FLAG 5

#define STATUS_CONNECTED 0
#define STATUS_DISCONNECTED 4
#define STATUS_UPDATING 1
#define STATUS_READY 2

// esp_ota_handle_t otaHandler = 0;

int status = -1;
int bytesReceived = 0;
bool updateFlag = false;

static String currentDataField;
static boolean blockingCommandInProgress = false;
static String *bleReturnString;

// indicates connection state to the android device
static boolean connected = false;

// indiciates whether or not a operation is currently in progress
static boolean operationInProgress = false;

void addData(String data)
{
  Serial.println("Received:" + data);
  currentDataField += data;
  if (!blockingCommandInProgress)
  {
    *bleReturnString = currentDataField;
  }
}

class cb : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    connected = true;
    Serial.println("BLE Device Connected");
    // delay(1000);

    // request notifs

  }

  void onDisconnect(BLEServer *pServer)
  {
    connected = false;
    Serial.println("BLE Device Disconnected");
  }
};

// class otaCallback : public BLECharacteristicCallbacks
// {
// public:
//   otaCallback(BLE *ble)
//   {
//     _p_ble = ble;
//     Serial.println("OTA callback (?)");
//   }
//   BLE *_p_ble;

//   void onWrite(BLECharacteristic *pCharacteristic);
// };

// void otaCallback::onWrite(BLECharacteristic *pCharacteristic)
// {
//   std::string rxData = pCharacteristic->getValue();
//   Serial.print("GOT FROM BT: ");
//   Serial.println(rxData.c_str());
//   // uint8_t txData[5] = {1, 2, 3, 4, 5};
//   // // delay(1000);
//   // pCharacteristic->setValue((uint8_t *)txData, 5);
//   // pCharacteristic->notify();
// }

class ccb : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string rxValue = pCharacteristic->getValue();
    Serial.print("RECEIVED DATA (onWrite): ");
    Serial.println(rxValue.c_str());
    addData(String(pCharacteristic->getValue().c_str()));
  }
  void onRead(BLECharacteristic *pCharacteristic)
  {
    // Serial.println("Characteristic Read");
    operationInProgress = false;
    Serial.println("Complete onRead String (?): " + currentDataField);
  }
};

// class notification_update_callback : public BLECharacteristicCallbacks
// {
//   void onWrite(BLECharacteristic *pCharacteristic)
//   {
//     Serial.println("GOT Notification " + String(pCharacteristic->getValue().c_str()));
//     // onNotificationEvent(String(pCharacteristic->getValue().c_str()));
//   }
// };

//
// Constructor
BLE::BLE(void) {}

//
// Destructor
BLE::~BLE(void) {}

//
// begin
bool BLE::begin(const char *localName = "Watchy BLE OTA")
{
  // Create the BLE Device
  Serial.println("Starting BLE");
  BLEDevice::init(localName);

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  // pServer->setCallbacks(new BLECustomServerCallbacks());

  // Create the BLE Service
  // pESPOTAService = pServer->createService(SERVICE_UUID_ESPOTA);
  pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  // pVersionCharacteristic = pService->createCharacteristic(
  //     CHARACTERISTIC_UUID_HW_VERSION, BLECharacteristic::PROPERTY_READ);

  // pWatchFaceNameCharacteristic = pService->createCharacteristic(
  //     CHARACTERISTIC_UUID_WATCHFACE_NAME, BLECharacteristic::PROPERTY_READ);

  // pOtaCharacteristic = pService->createCharacteristic(
  //     CHARACTERISTIC_UUID_FW,
  //     BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE);
  commandCharacteristic = pService->createCharacteristic(
      COMMAND_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY);

  // notificationUpdateCharacteristic = pService->createCharacteristic(
  //     CHARACTERISTIC_NOTIFICATION_UPDATE,
  //     BLECharacteristic::PROPERTY_READ |
  //         BLECharacteristic::PROPERTY_WRITE |
  //         BLECharacteristic::PROPERTY_NOTIFY);

  // pOtaCharacteristic->addDescriptor(new BLE2902());
  // pOtaCharacteristic->setCallbacks(new otaCallback(this));
  commandCharacteristic->setCallbacks(new ccb());
  commandCharacteristic->setValue("");

  // notificationUpdateCharacteristic->setCallbacks(new notification_update_callback());
  // notificationUpdateCharacteristic->setValue("");

  // add server callback so we can detect when we're connected.
  pServer->setCallbacks(new cb());

  pService->start();
  // startBLEAdvertising(); VV
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  // // Start the service(s)
  // pESPOTAService->start();
  // pService->start();

  // // Start advertising
  // pServer->getAdvertising()->addServiceUUID(SERVICE_UUID_ESPOTA);
  // pServer->getAdvertising()->start();

  // uint8_t hardwareVersion[5] = {1, 0, 1, 0, 1};
  // pVersionCharacteristic->setValue((uint8_t *)hardwareVersion, 5);
  // pWatchFaceNameCharacteristic->setValue("Watchy 7 Segment");

  return true;
}

int BLE::updateStatus() { return status; }

int BLE::howManyBytes() { return bytesReceived; }

// sends BLE command and returns data to a specific string. This function can be blocking (if you need it to perform a specific action) or non-blocking
// if you don't mind the data being used as its received.
boolean BLE::sendBLE(String command, String *returnString, boolean blocking)
{
  if (connected && !operationInProgress)
  {
    blockingCommandInProgress = blocking;
    operationInProgress = true;
    commandCharacteristic->setValue(command.c_str());
    commandCharacteristic->notify();

    Serial.println("Sent BLE Command: " + command);

    if (blockingCommandInProgress)
    {

      currentDataField = "";

      unsigned long startTime = millis();
      while (operationInProgress && (startTime + 2000 > millis()))
        delay(25);

      operationInProgress = false;
      if (currentDataField.length() == 0)
      {
        return false;
      }
      else
      {
        *returnString = currentDataField;
        return true;
      }
    }
    else
    {
      currentDataField = "";
      bleReturnString = returnString;
      *returnString = currentDataField;

      unsigned long startTime = millis();
      while ((currentDataField.length() == 0) && (startTime + 1000 > millis()))
        delay(25);

      if (currentDataField.length() == 0)
      {
        operationInProgress = false;
        return false;
      }
      else
      {
        return true;
      }
    }
  }

  return false;
}

boolean BLE::sendBLE(String command)
{
  if (connected && !operationInProgress)
  {
    operationInProgress = true;
    commandCharacteristic->setValue(command.c_str());
    commandCharacteristic->notify();

    Serial.println("Sent BLE Command: " + command);
    unsigned long startTime = millis();
    while (operationInProgress && (startTime + 200 > millis()))
      delay(25);
    return !operationInProgress;
  }
  return false;
}

void BLE::deinitBLE()
{
  BLEDevice::deinit(false);
  connected = false;
  Serial.println("Shutting down BLE");
}
