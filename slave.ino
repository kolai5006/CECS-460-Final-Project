/*
 * ESP32 BLE SERVER (Slave) - Temperature Receiver with LCD Display
 * Receives and displays temperature data from Master on LCD1602
 */

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <LiquidCrystal.h>

#define SERVER_NAME "ESP32_Server"
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

const int LED_PIN = 2;

// Initialize LCD with pin numbers: RS, E, D4, D5, D6, D7
// Using GPIO: 19, 23, 18, 5, 4, 2 (changed from LED_PIN to 13)
LiquidCrystal lcd(19, 23, 18, 5, 4, 13);

BLEServer* pServer = nullptr;
BLECharacteristic* pCharacteristic = nullptr;
bool connected = false;

String externalTemp = "--";
String internalTemp = "--";

// Forward declaration
void updateLCD();

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param) {
    connected = true;
    
    Serial.println("\n========================================");
    Serial.println("=== MASTER CONNECTED! ===");
    Serial.println("Waiting to receive temperature data...");
    Serial.println("========================================\n");
    
    // Update LCD to show connected status
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Master Connected");
    lcd.setCursor(0, 1);
    lcd.print("Waiting for data");
    
    pServer->updateConnParams(param->connect.remote_bda, 10, 20, 0, 400);
  }

  void onDisconnect(BLEServer* pServer) {
    connected = false;
    
    Serial.println("\n=== MASTER DISCONNECTED ===");
    
    // Update LCD to show disconnected status
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Disconnected");
    lcd.setCursor(0, 1);
    lcd.print("Waiting...");
    
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
        externalTemp = received.substring(extStart, extEnd);
        externalTemp.trim();
        
        // Extract internal temperature
        int intStart = received.indexOf("Internal:") + 9;
        int intEnd = received.indexOf("F", intStart);
        internalTemp = received.substring(intStart, intEnd);
        internalTemp.trim();
        
        Serial.println("  📊 External Temperature: " + externalTemp + " °F");
        Serial.println("  🌡️  Internal Temperature: " + internalTemp + " °F");
        Serial.println("══════════════════════════════════════════\n");
        
        // Display on LCD
        updateLCD();
        
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

void updateLCD() {
  lcd.clear();
  
  // Line 1: External temperature
  lcd.setCursor(0, 0);
  lcd.print("Ext:");
  lcd.print(externalTemp);
  lcd.print((char)223); // Degree symbol
  lcd.print("F");
  
  // Line 2: Internal temperature
  lcd.setCursor(0, 1);
  lcd.print("Int:");
  lcd.print(internalTemp);
  lcd.print((char)223); // Degree symbol
  lcd.print("F");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Initialize LCD
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("BLE Temp Receiver");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  
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
  
  // Update LCD to show ready status
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ready!");
  lcd.setCursor(0, 1);
  lcd.print("Waiting...");
}

void loop() {
  // Display connection status indicator
  static unsigned long lastStatus = 0;
  if (millis() - lastStatus > 5000) {
    lastStatus = millis();
    if (connected) {
      Serial.println("[Status: Connected - Receiving data...]");
      // Blink LED when connected
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    } else {
      Serial.println("[Status: Waiting for connection...]");
      digitalWrite(LED_PIN, LOW);
    }
  }
  
  delay(1000);
