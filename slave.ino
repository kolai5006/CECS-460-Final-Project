/*
 * ESP32 BLE SERVER (Slave)
 * This will advertise and wait for client to connect
 * LED lights up when client connects
 */

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// Server name (client will search for this)
#define SERVER_NAME "ESP32_Server"

// LED Pin
const int LED_PIN = 2;

// BLE variables
BLEServer* pServer = nullptr;
bool connected = false;

// Callback for connection events
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    Serial.println("\n=== CLIENT CONNECTED! ===");
    connected = true;
    digitalWrite(LED_PIN, HIGH);
  }

  void onDisconnect(BLEServer* pServer) {
    Serial.println("\n=== CLIENT DISCONNECTED ===");
    connected = false;
    digitalWrite(LED_PIN, LOW);
    
    // Restart advertising so client can reconnect
    Serial.println("Restarting advertising...");
    pServer->startAdvertising();
  }
};

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  Serial.println("\n=== ESP32 BLE SERVER (Slave) ===");
  Serial.println("Initializing BLE...");
  
  // Initialize BLE
  BLEDevice::init(SERVER_NAME);
  
  // Create BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->start();
  
  Serial.println("BLE Server started!");
  Serial.println("Device name: " + String(SERVER_NAME));
  Serial.println("Waiting for client to connect...");
}

void loop() {
  // Just monitor connection status
  if (connected) {
    Serial.print(".");
    delay(1000);
  } else {
    delay(100);
  }
}
