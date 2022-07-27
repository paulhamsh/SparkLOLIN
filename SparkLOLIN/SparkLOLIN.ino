#include "NimBLEDevice.h"

#define SERVICE        "ffc0"
#define CHAR1          "ffc1"
#define CHAR2          "ffc2"

BLEScan *pScan;
BLEScanResults pResults;

BLEClient *pClient;
BLERemoteService *pService;
BLERemoteCharacteristic *pChar1, *pChar2;
BLEAdvertisedDevice device;

BLEUUID ServiceUuid(SERVICE);


class MyClientCallback : public BLEClientCallbacks
{
  void onConnect(BLEClient *pclient)
  {
    Serial.println("Callback: connected");
  }
  void onDisconnect(BLEClient *pclient)
  {
    Serial.println("Callback: disconnected");   
  }
};

void notifyCB(BLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  int i;
  Serial.print("From Spark: ");
  for (i = 0; i < length; i++) {
    Serial.print(pData[i], HEX);
  }
  Serial.println();
}

void connect() {
  bool found = false;
  bool connected = false;
  int i;
  
  BLEDevice::init("SparkLOLIN");
  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
 
  pScan = BLEDevice::getScan();

  while (!found) {
    pResults = pScan->start(4);
    
    for(i = 0; i < pResults.getCount()  && !found; i++) {
      device = pResults.getDevice(i);

      if (device.isAdvertisingService(ServiceUuid)) {
        Serial.println("Found Mini");
        found = true;
        connected = false;
      }
    }

    // found it, now connect
   
    if (pClient->connect(&device)) {
      connected = true;
      pService = pClient->getService(ServiceUuid);
      if (pService != nullptr) {
        pChar1   = pService->getCharacteristic(CHAR1);
        pChar2   = pService->getCharacteristic(CHAR2);        
        if (pChar2 && pChar2->canNotify()) {
          pChar2->registerForNotify(notifyCB);
          if (!pChar2->subscribe(true, notifyCB, true)) {
            connected = false;
            Serial.println("Disconnected");
            NimBLEDevice::deleteClient(pClient);
          }   
        } 
      }
    }
  }
}

unsigned long t;
int preset;
int new_preset;

const uint8_t switchPins[]{33,32,25,26};

void setup() {
  Serial.begin(115200);

  for (int i=0; i < 4; i++)
    pinMode(switchPins[i], INPUT_PULLDOWN); 
  
  Serial.println("Connecting...");
  connect();
  Serial.println("Connected");
  t = millis();
  preset = 0;
  new_preset = 0;
}

byte preset_cmd[] = {
  0x01, 0xFE, 0x00, 0x00,
  0x53, 0xFE, 0x1A, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0xF0, 0x01, 0x24, 0x00,
  0x01, 0x38, 0x00, 0x00,
  0x00, 0xF7
};
const int preset_cmd_size = 26;

void loop() {
  if (millis() - t > 5000) {
    t = millis();

    // time process
    new_preset++;    
    if (new_preset > 3) new_preset = 0;

    // GPIO process      
    for (int i = 0; i < 4; i++) {
      if (digitalRead(switchPins[i]) == 1) {
        new_preset = i;
        Serial.print("Got a switch ");
        Serial.println(i);
      }
    }

    if (new_preset != preset) {    
      preset = new_preset;
      preset_cmd[preset_cmd_size-2] = preset;
      Serial.print("Sent a preset change to ");
      Serial.println(preset);
      pChar1->writeValue(preset_cmd, preset_cmd_size);
    }
  }
}
