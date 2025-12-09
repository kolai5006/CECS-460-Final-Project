/*
 * ESP32 BLE CLIENT (Master)
 * Type commands in Serial Monitor to send to Slave
 */

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>

#define SERVER_NAME "ESP32_Server"
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

const int LED_PIN = 2;

BLEClient* pClient = nullptr;
BLERemoteCharacteristic* pRemoteChar = nullptr;
BLEAddress* pServerAddress = nullptr;
bool connected = false;
bool doConnect = false;
bool doScan = false;

// Callback when slave sends us a response
static void notifyCallback(BLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
  String received = "";
  for(int i = 0; i < length; i++) {
    received += (char)pData[i];
  }
  
  Serial.println("\n>>> Received response from Slave: " + received);
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.getName() == SERVER_NAME) {
      Serial.println("Found our slave!");
      advertisedDevice.getScan()->stop();
      pServerAddress = new BLEAddress(advertisedDevice.getAddress());
      doConnect = true;
      doScan = false;
    }
  }
};

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("=== Connected to Slave! ===");
    Serial.println("Type a message and press Enter to send to Slave");
    Serial.println("Try: PING, HELLO, STATUS, or any message");
    connected = true;
    digitalWrite(LED_PIN, HIGH);
  }

  void onDisconnect(BLEClient* pclient) {
    Serial.println("=== Disconnected from Slave! ===");
    connected = false;
    digitalWrite(LED_PIN, LOW);
    pRemoteChar = nullptr;
    doScan = true;
  }
};

bool connectToServer() {
  Serial.println("Connecting to slave...");
  
  if (pClient == nullptr) {
    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
  }
  
  if (!pClient->connect(*pServerAddress)) {
    Serial.println("Failed to connect!");
    return false;
  }
  
  Serial.println("Connected! Getting service...");
  
  BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
  if (pRemoteService == nullptr) {
    Serial.println("Failed to find service!");
    pClient->disconnect();
    return false;
  }
  
  Serial.println("Service found! Getting characteristic...");
  
  pRemoteChar = pRemoteService->getCharacteristic(CHAR_UUID);
  if (pRemoteChar == nullptr) {
    Serial.println("Failed to find characteristic!");
    pClient->disconnect();
    return false;
  }
  
  Serial.println("Characteristic found! Registering for notifications...");
  
  if(pRemoteChar->canNotify()) {
    pRemoteChar->registerForNotify(notifyCallback);
  }
  
  Serial.println("=== Setup complete! Ready to send commands ===");
  connected = true;
  digitalWrite(LED_PIN, HIGH);
  return true;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  delay(1000);
  
  Serial.println("\n=== ESP32 BLE CLIENT (Master) ===");
  Serial.println("This device sends commands to Slave");
  
  BLEDevice::init("ESP32_Client");
  BLEDevice::setMTU(517);
  
  Serial.println("Scanning for slave...");
  doScan = true;
}

void loop() {
  if (doScan) {
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    pBLEScan->start(5, false);
    doScan = false;
  }
  
  if (doConnect) {
    doConnect = false;
    if (connectToServer()) {
      Serial.println("Successfully connected and configured!");
    } else {
      Serial.println("Connection failed, will scan again...");
      doScan = true;
    }
  }
  
  // Check if user typed a command in Serial Monitor
  if (Serial.available() && connected) {
    String message = Serial.readStringUntil('\n');
    message.trim();
    
    if (message.length() > 0 && pRemoteChar != nullptr) {
      Serial.println("<<< Sending to Slave: " + message);
      pRemoteChar->writeValue(message.c_str(), message.length());
      Serial.println("Command sent!");
    }
  }
  
  if (connected && pClient != nullptr) {
    if (!pClient->isConnected()) {
      Serial.println("Connection lost detected!");
      connected = false;
      digitalWrite(LED_PIN, LOW);
      pRemoteChar = nullptr;
      delay(1000);
      doScan = true;
    }
  }
  
  delay(100);
}
// // CECS 460 Lab 7: ADC Temperature Sensing with Bluetooth Transmission
// // ESP32 Temperature monitoring using external thermistor - SENDER

// #include <BluetoothSerial.h>

// #define THERMISTOR_PIN 34  // GPIO34 = ADC1_CH6
// #define SAMPLE_SIZE 5      // Moving average filter size
// #define SAMPLE_INTERVAL 500 // Sample every 500ms

// // Check if Bluetooth is available
// #if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
// #error Bluetooth is not enabled! Please run `make menuconfig` to enable it
// #endif

// BluetoothSerial SerialBT;

// // LUT for ADC to Temperature (°C) conversion
// const int LUT_SIZE = 11;
// int adcLUT[LUT_SIZE] = {500, 1000, 1500, 2000, 2500, 3000, 3200, 3400, 3600, 3800, 4095};
// float tempC_LUT[LUT_SIZE] = {-10, 0, 10, 15, 20, 25, 30, 40, 60, 80, 100};

// // Moving average filter buffer
// int adcBuffer[SAMPLE_SIZE];
// int bufferIndex = 0;
// bool bufferFilled = false;

// void setup() {
//   Serial.begin(115200);
  
//   // Initialize Bluetooth as CLIENT (connects to receiver)
//   SerialBT.begin("ESP32_Sender", true); // true = master/client mode
//   Serial.println("========================================");
//   Serial.println("Bluetooth Sender Started!");
//   Serial.println("Device name: ESP32_Sender");
//   Serial.println("Mode: CLIENT (searching for receiver...)");
//   Serial.println("========================================");
  
//   // Wait a moment for Bluetooth to fully initialize
//   delay(2000);
  
//   // Try to connect to receiver
//   Serial.println("Attempting to connect to ESP32_Receiver...");
//   bool connected = SerialBT.connect("ESP32_Receiver");
  
//   if (connected) {
//     Serial.println("✓✓✓ CONNECTED TO RECEIVER! ✓✓✓");
//   } else {
//     Serial.println("✗ Connection failed - will keep trying...");
//   }
  
//   // Configure ADC for thermistor input
//   analogReadResolution(12);  // 12-bit resolution (0-4095)
//   analogSetAttenuation(ADC_11db);  // Full range ~0-3.3V
  
//   // Initialize moving average buffer
//   for (int i = 0; i < SAMPLE_SIZE; i++) {
//     adcBuffer[i] = 0;
//   }
  
//   delay(1000);
  
//   Serial.println("ESP32 Temperature Sender - Ready");
//   Serial.println("Waiting for receiver connection...");
//   Serial.println();
// }

// void loop() {
//   // Read raw ADC value from thermistor
//   int rawADC = analogRead(THERMISTOR_PIN);
  
//   // Store new reading in circular buffer
//   adcBuffer[bufferIndex] = rawADC;
//   bufferIndex = (bufferIndex + 1) % SAMPLE_SIZE;
//   if (bufferIndex == 0) bufferFilled = true;
  
//   // Calculate filtered ADC value
//   int filteredADC = calculateMovingAverage();
  
//   // Convert filtered ADC to temperature using LUT
//   float tempC_LUT = lookupTemperature(filteredADC);
//   float tempF_LUT = (tempC_LUT * 9.0 / 5.0) + 32.0;
  
//   // Read internal temperature sensor
//   float internalTempC = temperatureRead();
//   float internalTempF = (internalTempC * 9.0 / 5.0) + 32.0;
  
//   // Send temperature via Bluetooth (send Fahrenheit as float)
//   if (SerialBT.connected()) {
//     SerialBT.println(tempF_LUT);  // Send just the temperature value
//     Serial.print("✓ Sent via BT: ");
//     Serial.print(tempF_LUT, 1);
//     Serial.println(" °F");
//   } else {
//     Serial.println("✗ Not connected - attempting reconnection...");
//     // Try to reconnect
//     if (SerialBT.connect("ESP32_Receiver")) {
//       Serial.println("✓ Reconnected!");
//     }
//   }
  
//   // Output to Serial Monitor for debugging
//   Serial.print("Raw_ADC:");
//   Serial.print(rawADC);
//   Serial.print(" | Filtered_ADC:");
//   Serial.print(filteredADC);
//   Serial.print(" | Temp_C:");
//   Serial.print(tempC_LUT, 1);
//   Serial.print(" | Temp_F:");
//   Serial.print(tempF_LUT, 1);
//   Serial.print(" | Internal_C:");
//   Serial.print(internalTempC, 1);
//   Serial.print(" | Internal_F:");
//   Serial.println(internalTempF, 1);
  
//   delay(SAMPLE_INTERVAL);
// }

// // Moving average filter function
// int calculateMovingAverage() {
//   long sum = 0;
//   int count = bufferFilled ? SAMPLE_SIZE : bufferIndex;
  
//   if (count == 0) return adcBuffer[0];
  
//   for (int i = 0; i < count; i++) {
//     sum += adcBuffer[i];
//   }
  
//   return sum / count;
// }

// // LUT lookup function with linear interpolation
// float lookupTemperature(int adcValue) {
//   // Handle boundary cases
//   if (adcValue <= adcLUT[0]) {
//     return tempC_LUT[0];
//   }
//   if (adcValue >= adcLUT[LUT_SIZE - 1]) {
//     return tempC_LUT[LUT_SIZE - 1];
//   }
  
//   // Linear interpolation
//   for (int i = 0; i < LUT_SIZE - 1; i++) {
//     if (adcValue >= adcLUT[i] && adcValue < adcLUT[i + 1]) {
//       float ratio = (float)(adcValue - adcLUT[i]) / (float)(adcLUT[i + 1] - adcLUT[i]);
//       float tempResult = tempC_LUT[i] + ratio * (tempC_LUT[i + 1] - tempC_LUT[i]);
//       return tempResult;
//     }
//   }
  
//   return tempC_LUT[LUT_SIZE - 1];
// }