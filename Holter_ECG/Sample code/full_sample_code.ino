#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <SPI.h>
#include <SD.h>
#include <esp_sleep.h>
#include <Wire.h>
#include <ADS1293.h>

// GPIO pin where the ADS1293 interrupt is connected
#define ADS1293_INT_PIN 33 // Change based on your configuration

// SD Card configuration
#define SD_CS_PIN 5 // Change according to your setup

// BLE configuration
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pCommandCharacteristic;

// Data variables
String dataToSend = "Sample Data"; // Replace with actual ADS1293 data
File dataFile;
String receivedCommand = "";

// Command to listen for
const char* commandToSendData = "send data"; // The command to trigger sending data

void setup() {
  Serial.begin(115200);
  
  // Initialize SD card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD Card initialization failed!");
    return;
  }
  Serial.println("SD Card initialized.");

  // Set up GPIO interrupt to wake up from deep sleep
  pinMode(ADS1293_INT_PIN, INPUT_PULLUP);  // ADS1293 interrupt pin
  attachInterrupt(digitalPinToInterrupt(ADS1293_INT_PIN), wakeUp, FALLING); // Change FALLING to RISING if needed

  // Configure BLE
  BLEDevice::init("ESP32S3_BLE"); // Change to your desired device name
  pServer = BLEDevice::createServer();
  pService = pServer->createService(SERVICE_UUID);
  
  // Create the command characteristic to listen for "send data"
  pCommandCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_COMMAND,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
  );

  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_DATA,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
  );

  pServer->setCallbacks(new MyServerCallbacks()); // Set callback for connection events

  // Start BLE service
  pService->start();

  // Set the ESP32 to deep sleep mode
  enterDeepSleep();
}

void loop() {
  // This code will run after the device wakes up

  Serial.println("Woke up from deep sleep!");

  // Step 2: Get data from ADS1293 (Assuming ADS1293 is already configured)
  readADS1293Data();

  // Step 3: Store data on SD card
  saveDataToSDCard();

  // Step 4: Turn on BLE and check if the "send data" command is received
  if (deviceConnected) {
    checkCommandReceived();
    if (receivedCommand == commandToSendData) {
      sendDataViaBLE();
    } else {
      // If the command is not received, move to next step
      Serial.println("No 'send data' command received. Proceeding to next step.");
    }
  }

  // Step 5: Turn off BLE and go to deep sleep
  disableBLE();

  // Step 6: Enter deep sleep and monitor GPIO for interrupts
  enterDeepSleep();
}

// Function to read data from ADS1293
void readADS1293Data() {
  // Read actual data from ADS1293 here and store it in dataToSend
  // For now, we simulate with a placeholder message.
  dataToSend = "Simulated ADS1293 Data"; // Replace with actual ADS1293 data
  Serial.println("ADS1293 Data: " + dataToSend);
}

// Function to save data to SD card
void saveDataToSDCard() {
  dataFile = SD.open("/data.txt", FILE_APPEND);
  if (dataFile) {
    dataFile.println(dataToSend);
    dataFile.close();
    Serial.println("Data saved to SD card.");
  } else {
    Serial.println("Error opening SD card file.");
  }
}

// Function to send data via BLE
void sendDataViaBLE() {
  if (deviceConnected) {
    pCharacteristic->setValue(dataToSend.c_str());
    pCharacteristic->notify(); // Send data to connected BLE device
    Serial.println("Data sent via BLE.");
  }
}

// Function to disable BLE and go to deep sleep
void disableBLE() {
  BLEDevice::deinit(true); // Turn off BLE
  btStop(); // Explicitly stop the Bluetooth controller
  Serial.println("BLE turned off completely.");
}

// Function to put ESP32 into deep sleep mode
void enterDeepSleep() {
  Serial.println("Entering deep sleep...");
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 0); // Wake up on GPIO interrupt (use your pin)
  esp_deep_sleep_start(); // Enter deep sleep
}

// Interrupt handler to wake up from deep sleep
void wakeUp() {
  // This function will be called when an interrupt is detected on ADS1293_INT_PIN
  // It's used to wake the ESP32 from deep sleep
  Serial.println("Interrupt detected, waking up...");
}

// Check if the "send data" command is received via BLE
void checkCommandReceived() {
  if (pCommandCharacteristic->getValue().length() > 0) {
    receivedCommand = pCommandCharacteristic->getValue().c_str();
    Serial.println("Command received: " + receivedCommand);
  }
}

// BLE Server Callbacks
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Device connected!");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Device disconnected!");
  }
};
