/*
 * ESP32 BLE SERVER (Slave)
 * Receives commands from Master and responds automatically
 */

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SERVER_NAME "ESP32_Server"
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

const int LED_PIN = 2;

BLEServer* pServer = nullptr;
BLECharacteristic* pCharacteristic = nullptr;
bool connected = false;

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param) {
    connected = true;
    digitalWrite(LED_PIN, HIGH);
    
    Serial.println("\n=== MASTER CONNECTED! ===");
    Serial.println("Waiting to receive commands from Master...");
    
    pServer->updateConnParams(param->connect.remote_bda, 10, 20, 0, 400);
  }

  void onDisconnect(BLEServer* pServer) {
    connected = false;
    digitalWrite(LED_PIN, LOW);
    
    Serial.println("\n=== MASTER DISCONNECTED ===");
    
    delay(500);
    
    Serial.println("Restarting advertising...");
    pServer->startAdvertising();
    Serial.println("Waiting for Master to reconnect...");
  }
};

// Callback when we receive a command from Master
class MyCharCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pChar) {
    String received = pChar->getValue().c_str();
    
    if (received.length() > 0) {
      Serial.println("\n>>> Received command from Master: " + received);
      
      // Determine response based on command
      String response = "";
      if (received == "PING") {
        response = "PONG";
      } else if (received == "HELLO") {
        response = "HI THERE";
      } else if (received == "STATUS") {
        response = "ALL GOOD";
      } else {
        response = "RECEIVED: " + received;
      }
      
      Serial.println("<<< Sending response: " + response);
      
      // Send response back to Master
      pChar->setValue(response.c_str());
      pChar->notify();
    }
  }
};

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  delay(1000);
  
  Serial.println("\n=== ESP32 BLE SERVER (Slave) ===");
  Serial.println("This device receives commands and responds automatically");
  Serial.println("Initializing BLE...");
  
  BLEDevice::init(SERVER_NAME);
  BLEDevice::setMTU(517);
  
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  pCharacteristic = pService->createCharacteristic(
    CHAR_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY |
    BLECharacteristic::PROPERTY_INDICATE
  );
  
  pCharacteristic->setCallbacks(new MyCharCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());
  
  pService->start();
  
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMaxPreferred(0x12);
  pAdvertising->start();
  
  Serial.println("BLE Server started!");
  Serial.println("Device name: " + String(SERVER_NAME));
  Serial.println("Waiting for Master to connect...");
}

void loop() {
  // Slave just waits for commands - no user input needed
  delay(1000);
}
