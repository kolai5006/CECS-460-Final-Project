#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>

#define SERVER_NAME "ESP32_Server"
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Temperature sensor setup
#define THERMISTOR_PIN 34  // GPIO34 = ADC1_CH6
#define SAMPLE_SIZE 5
#define SAMPLE_INTERVAL 2000

const int LED_PIN = 2;

// UART to website ESP32
#define HW_TX_PIN 26   // Sending to WiFi ESP32
#define HW_RX_PIN 25   // Not used, but required by Serial2.begin()

BLEClient *pClient = nullptr;
BLERemoteCharacteristic *pRemoteChar = nullptr;
BLEAddress *pServerAddress = nullptr;

bool connected = false;
bool doConnect = false;
bool doScan = false;

// Lookup table for ADC→Temp
const int LUT_SIZE = 11;
int adcLUT[LUT_SIZE] = {500, 1000, 1500, 2000, 2500, 3000, 3200, 3400, 3600, 3800, 4095};
float tempC_LUT[LUT_SIZE] = {-10, 0, 10, 15, 20, 25, 30, 40, 60, 80, 100};

int adcBuffer[SAMPLE_SIZE];
int bufferIndex = 0;
bool bufferFilled = false;
unsigned long lastTempSend = 0;

// ------------ Moving Average ----------
int calculateMovingAverage() {
  long sum = 0;
  int count = bufferFilled ? SAMPLE_SIZE : bufferIndex;
  if (count == 0) return adcBuffer[0];

  for (int i = 0; i < count; i++) sum += adcBuffer[i];
  return sum / count;
}

// ------------ LUT Temperature ----------
float lookupTemperature(int adcValue) {
  if (adcValue <= adcLUT[0]) return tempC_LUT[0];
  if (adcValue >= adcLUT[LUT_SIZE - 1]) return tempC_LUT[LUT_SIZE - 1];

  for (int i = 0; i < LUT_SIZE - 1; i++) {
    if (adcValue >= adcLUT[i] && adcValue < adcLUT[i + 1]) {
      float ratio = (float)(adcValue - adcLUT[i]) / (float)(adcLUT[i + 1] - adcLUT[i]);
      return tempC_LUT[i] + ratio * (tempC_LUT[i + 1] - tempC_LUT[i]);
    }
  }
  return tempC_LUT[LUT_SIZE - 1];
}

// ------------ BLE notify callback ----------
static void notifyCallback(BLERemoteCharacteristic *pChar, uint8_t *pData, size_t length, bool isNotify) {
  String received = "";
  for (int i = 0; i < length; i++) received += (char)pData[i];
  Serial.println(">>> Response from Slave: " + received);
}

// ------------ Device scan callback ----------
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
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

// ------------ BLE connection events ----------
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient *pclient) {
    Serial.println("=== Connected to Slave! ===");
    connected = true;
    digitalWrite(LED_PIN, HIGH);
  }

  void onDisconnect(BLEClient *pclient) {
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

  BLERemoteService *pRemoteService = pClient->getService(SERVICE_UUID);
  if (pRemoteService == nullptr) {
    Serial.println("Failed to find service!");
    pClient->disconnect();
    return false;
  }

  pRemoteChar = pRemoteService->getCharacteristic(CHAR_UUID);
  if (pRemoteChar == nullptr) {
    Serial.println("Failed to find characteristic!");
    pClient->disconnect();
    return false;
  }

  if (pRemoteChar->canNotify()) {
    pRemoteChar->registerForNotify(notifyCallback);
  }

  Serial.println("=== Connected and ready to send temperature! ===");
  return true;
}

// ------------ SETUP ----------
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  Serial2.begin(115200, SERIAL_8N1, HW_RX_PIN, HW_TX_PIN);
  Serial.println("UART Ready on TX=26");

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  for (int i = 0; i < SAMPLE_SIZE; i++) adcBuffer[i] = 0;

  BLEDevice::init("ESP32_Client");
  BLEDevice::setMTU(517);

  Serial.println("Scanning for slave...");
  doScan = true;
}

// ------------ MAIN LOOP ----------
void loop() {
  // --- BLE scan / connect logic stays the same ---
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
      Serial.println("Successfully connected to BLE slave!");
    } else {
      Serial.println("Connection failed, will scan again...");
      doScan = true;
    }
  }

  // --- ALWAYS read sensor and send every SAMPLE_INTERVAL, even if BLE is not connected ---
  int rawADC = analogRead(THERMISTOR_PIN);

  // Store in circular buffer
  adcBuffer[bufferIndex] = rawADC;
  bufferIndex = (bufferIndex + 1) % SAMPLE_SIZE;
  if (bufferIndex == 0) bufferFilled = true;

  // Time-based send
  if (millis() - lastTempSend >= SAMPLE_INTERVAL) {
    lastTempSend = millis();

    // Calculate filtered ADC value
    int filteredADC = calculateMovingAverage();

    // Convert to temperature
    float tempC = lookupTemperature(filteredADC);
    float tempF = (tempC * 9.0 / 5.0) + 32.0;

    // Internal temperature
    float internalTempC = temperatureRead();
    float internalTempF = (internalTempC * 9.0 / 5.0) + 32.0;

    // Create message
    String tempMessage = "TEMP|External:" + String(tempF, 1) + "F|Internal:" + String(internalTempF, 1) + "F";

    // --- ALWAYS send via UART to WiFi ESP32 ---
    Serial2.println(tempMessage);
    Serial.println("UART >>> " + tempMessage);

    // --- IF BLE is connected, also send to 7-seg ESP32 ---
    if (connected && pRemoteChar != nullptr) {
      pRemoteChar->writeValue(tempMessage.c_str(), tempMessage.length());
      Serial.println("BLE  >>> Sent to slave");
    } else {
      Serial.println("BLE  >>> Not connected, skipped BLE send");
    }

    // Extra debug
    Serial.println("\n--- Temperature Reading ---");
    Serial.print("Raw ADC: "); Serial.println(rawADC);
    Serial.print("Filtered ADC: "); Serial.println(filteredADC);
    Serial.print("External Temp: "); Serial.print(tempC, 1); Serial.print(" °C / ");
    Serial.print(tempF, 1); Serial.println(" °F");
    Serial.print("Internal Temp: "); Serial.print(internalTempC, 1); Serial.print(" °C / ");
    Serial.print(internalTempF, 1); Serial.println(" °F");
  }

  // --- Check BLE connection status ---
  if (connected && pClient != nullptr) {
    if (!pClient->isConnected()) {
      Serial.println("Connection lost!");
      connected = false;
      digitalWrite(LED_PIN, LOW);
      pRemoteChar = nullptr;
      delay(1000);
      doScan = true;
    }
  }

  delay(100);
}
