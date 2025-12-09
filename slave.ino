/*
 * ESP32 BLE SERVER (Slave) - Temperature Receiver
 * Receives and displays temperature data from Master
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
    
    Serial.println("\n========================================");
    Serial.println("=== MASTER CONNECTED! ===");
    Serial.println("Waiting to receive temperature data...");
    Serial.println("========================================\n");
    
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

// Callback when we receive temperature data from Master
class MyCharCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pChar) {
    String received = pChar->getValue().c_str();
    
    if (received.length() > 0) {
      // Check if it's a temperature message
      if (received.startsWith("TEMP|")) {
        // Parse the temperature data
        Serial.println("\n╔════════════════════════════════════════╗");
        Serial.println("║    TEMPERATURE DATA RECEIVED           ║");
        Serial.println("╚════════════════════════════════════════╝");
        
        // Extract external temperature
        int extStart = received.indexOf("External:") + 9;
        int extEnd = received.indexOf("F", extStart);
        String externalTemp = received.substring(extStart, extEnd);
        
        // Extract internal temperature
        int intStart = received.indexOf("Internal:") + 9;
        int intEnd = received.indexOf("F", intStart);
        String internalTemp = received.substring(intStart, intEnd);
        
        Serial.println("  📊 External Temperature: " + externalTemp + " °F");
        Serial.println("  🌡️  Internal Temperature: " + internalTemp + " °F");
        Serial.println("══════════════════════════════════════════\n");
        
        // Send acknowledgment back to Master
        String ack = "ACK|Received temps";
        pChar->setValue(ack.c_str());
        pChar->notify();
        
      } else {
        // Handle other messages
        Serial.println("\n>>> Received: " + received);
        
        String response = "RECEIVED: " + received;
        pChar->setValue(response.c_str());
        pChar->notify();
      }
    }
  }
};

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  delay(1000);
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║  ESP32 BLE SLAVE - Temp Receiver      ║");
  Serial.println("╚════════════════════════════════════════╝");
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
  
  Serial.println("✓ BLE Server started!");
  Serial.println("✓ Device name: " + String(SERVER_NAME));
  Serial.println("✓ Waiting for Master to connect...\n");
}

void loop() {
  // Display connection status indicator
  static unsigned long lastStatus = 0;
  if (millis() - lastStatus > 5000) {
    lastStatus = millis();
    if (connected) {
      Serial.println("[Status: Connected - Receiving data...]");
    } else {
      Serial.println("[Status: Waiting for connection...]");
    }
  }
  
  delay(1000);
}
