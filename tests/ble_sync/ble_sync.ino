#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "heltec_unofficial.h"

// Constants
#define LEQ_PERIOD 5000 // ms
#define INITIAL_CHECK_PERIODS 3
#define MAX_SC_REQUEST_LATENCY 500 // ms
#define MAX_SC_RESPONSE_LATENCY 1000 // ms
#define MAX_CS_RESPONSE_LATENCY 750 // ms
#define SC_REQUEST_DELAY 10 // ms
#define CS_RESPONSE_DELAY 5 // ms
#define SERVER_TRANSITION_DELAY 1000 // ms
#define MAX_SYNC_FAILURES 3
#define NODE_INDEX 1

// BLE UUIDs
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define SC_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CS_CHARACTERISTIC_UUID "beb5483f-36e1-4688-b7f5-ea07361b26a8"

// Global variables
BLEServer* pServer = NULL;
BLECharacteristic* pSCCharacteristic = NULL;
BLECharacteristic* pCSCharacteristic = NULL;
bool deviceConnected = false;
bool isServer = false;
int syncFailures = 0;
unsigned long lastSCRequestTime = 0;
unsigned long lastSCResponseTime = 0;
uint8_t maxGroupLeq_dB = 0;
uint8_t localLeq_dB = 0;

// Function prototypes
void checkRestrictions();
void serverOperation();
void clientOperation();
void becomeServer();
void becomeClient();
void sendSCMessage(uint8_t leq_dB);
void sendCSMessage(uint8_t leq_dB);
void displayOnOLED(uint8_t localLeq_dB, uint8_t maxGroupLeq_dB);
bool checkForExistingServer();

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class SCCharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      if (value.length() == 2) {
        uint8_t receivedLeq_dB = value[0];
        uint8_t receivedNodeIndex = value[1];
        
        if (isServer && receivedNodeIndex < NODE_INDEX) {
          becomeClient();
        }
        
        if (!isServer) {
          if (receivedLeq_dB == 0) {
            // SC request received
            delay(NODE_INDEX * CS_RESPONSE_DELAY);
            sendCSMessage(localLeq_dB);
            lastSCRequestTime = millis();
          } else {
            // SC response received
            maxGroupLeq_dB = receivedLeq_dB;
            displayOnOLED(localLeq_dB, maxGroupLeq_dB);
            lastSCResponseTime = millis();
            syncFailures = 0;
          }
        }
      }
    }
};

class CSCharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      if (isServer) {
        std::string value = pCharacteristic->getValue();
        if (value.length() == 2) {
          uint8_t receivedLeq_dB = value[0];
          if (receivedLeq_dB > maxGroupLeq_dB) {
            maxGroupLeq_dB = receivedLeq_dB;
          }
        }
      }
    }
};

void setup() {
  heltec_setup();
  
  checkRestrictions();
  
  BLEDevice::init("ESP32_BLE_Node");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  pSCCharacteristic = pService->createCharacteristic(
                        SC_CHARACTERISTIC_UUID,
                        BLECharacteristic::PROPERTY_READ |
                        BLECharacteristic::PROPERTY_WRITE |
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
  pSCCharacteristic->addDescriptor(new BLE2902());
  pSCCharacteristic->setCallbacks(new SCCharacteristicCallbacks());
  
  pCSCharacteristic = pService->createCharacteristic(
                        CS_CHARACTERISTIC_UUID,
                        BLECharacteristic::PROPERTY_READ |
                        BLECharacteristic::PROPERTY_WRITE |
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
  pCSCharacteristic->addDescriptor(new BLE2902());
  pCSCharacteristic->setCallbacks(new CSCharacteristicCallbacks());
  
  pService->start();
  
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  // Initial server check
  if (checkForExistingServer()) {
    becomeClient();
  } else {
    becomeServer();
  }
}

void loop() {
  if (isServer) {
    serverOperation();
  } else {
    clientOperation();
  }
  heltec_loop();
}

bool checkForExistingServer() {
  class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    public:
      bool found = false;
      void onResult(BLEAdvertisedDevice advertisedDevice) {
        if (advertisedDevice.haveServiceUUID() && advertisedDevice.getServiceUUID().equals(BLEUUID(SERVICE_UUID))) {
          found = true;
          advertisedDevice.getScan()->stop();
        }
      }
  };
  
  MyAdvertisedDeviceCallbacks myCallbacks;
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(&myCallbacks);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5); // Scan for 5 seconds
  
  return myCallbacks.found;
}

void serverOperation() {
  static unsigned long lastOperationTime = 0;
  
  if (millis() - lastOperationTime >= LEQ_PERIOD) {
    lastOperationTime = millis();
    
    localLeq_dB = random(1, 4);
    maxGroupLeq_dB = localLeq_dB;
    
    delay(SC_REQUEST_DELAY);
    
    sendSCMessage(0); // SC request
    
    unsigned long requestTime = millis();
    
    while (millis() - requestTime < MAX_CS_RESPONSE_LATENCY) {
      // Wait for CS messages, maxGroupLeq_dB is updated in CSCharacteristicCallbacks
    }
    
    sendSCMessage(maxGroupLeq_dB); // SC response
    displayOnOLED(localLeq_dB, maxGroupLeq_dB);
  }
}

void clientOperation() {
  static unsigned long lastOperationTime = 0;
  
  if (millis() - lastOperationTime >= LEQ_PERIOD) {
    lastOperationTime = millis();
    localLeq_dB = random(1, 4);
  }
  
  if (millis() - lastSCRequestTime > MAX_SC_REQUEST_LATENCY) {
    syncFailures++;
    if (syncFailures >= MAX_SYNC_FAILURES) {
      if (checkForExistingServer()) {
        becomeClient(); // Re-sync
      } else {
        becomeServer();
      }
      return;
    }
  }
  
  if (millis() - lastSCResponseTime > MAX_SC_RESPONSE_LATENCY) {
    syncFailures++;
    if (syncFailures >= MAX_SYNC_FAILURES) {
      if (checkForExistingServer()) {
        becomeClient(); // Re-sync
      } else {
        becomeServer();
      }
      return;
    }
  }
  
  displayOnOLED(localLeq_dB, maxGroupLeq_dB);
}

void becomeServer() {
  isServer = true;
  sendSCMessage(0);
  displayOnOLED(localLeq_dB, maxGroupLeq_dB);
}

void becomeClient() {
  isServer = false;
  syncFailures = 0;
  lastSCRequestTime = millis();
  lastSCResponseTime = millis();
  displayOnOLED(localLeq_dB, maxGroupLeq_dB);
}

void sendSCMessage(uint8_t leq_dB) {
  if (deviceConnected) {
    uint8_t txValue[2] = {leq_dB, NODE_INDEX};
    pSCCharacteristic->setValue(txValue, 2);
    pSCCharacteristic->notify();
  }
}

void sendCSMessage(uint8_t leq_dB) {
  if (deviceConnected) {
    uint8_t txValue[2] = {leq_dB, NODE_INDEX};
    pCSCharacteristic->setValue(txValue, 2);
    pCSCharacteristic->notify();
  }
}

void displayOnOLED(uint8_t localLeq_dB, uint8_t maxGroupLeq_dB) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Node: " + String(NODE_INDEX));
  display.drawString(0, 15, "Local Leq: " + String(localLeq_dB));
  display.drawString(0, 30, "Max Group Leq: " + String(maxGroupLeq_dB));
  display.drawString(0, 45, isServer ? "Mode: Server" : "Mode: Client");
  display.display();
}

void checkRestrictions() {
  if (NODE_INDEX * CS_RESPONSE_DELAY >= MAX_CS_RESPONSE_LATENCY) {
    Serial.println("Error: NODE_INDEX * CS_RESPONSE_DELAY must be less than MAX_CS_RESPONSE_LATENCY");
    while(1);
  }
  
  if (SC_REQUEST_DELAY >= MAX_SC_REQUEST_LATENCY) {
    Serial.println("Error: SC_REQUEST_DELAY must be less than MAX_SC_REQUEST_LATENCY");
    while(1);
  }
  
  if (MAX_CS_RESPONSE_LATENCY >= MAX_SC_RESPONSE_LATENCY) {
    Serial.println("Error: MAX_CS_RESPONSE_LATENCY must be less than MAX_SC_RESPONSE_LATENCY");
    while(1);
  }
  
  if (SC_REQUEST_DELAY <= CS_RESPONSE_DELAY) {
    Serial.println("Error: SC_REQUEST_DELAY must be greater than CS_RESPONSE_DELAY");
    while(1);
  }
}