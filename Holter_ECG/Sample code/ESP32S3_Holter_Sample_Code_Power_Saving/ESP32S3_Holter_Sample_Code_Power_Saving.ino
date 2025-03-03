#include <SPI.h>
#include <SD.h>
#include <NimBLEDevice.h>
#include <WiFi.h>
#include "esp_sleep.h"

// Pin Definitions
#define ADS1296_INT_PIN  34  // Interrupt Pin (Low-to-High triggers wakeup)
#define ADS1296_CS 5         // Chip Select for ADS1296
#define SD_CS 4              // Chip Select for SD Card
#define SPI_CLK 18
#define SPI_MISO 19
#define SPI_MOSI 23

// BLE Service & Characteristic UUIDs
#define SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0"
#define CHARACTERISTIC_UUID "abcd1234-5678-1234-5678-abcdef123456"

NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pCharacteristic = nullptr;
bool bleConnected = false;

// Callback class for BLE connections
class MyServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
        Serial.println("BLE Connected!");
        bleConnected = true;
    }
    void onDisconnect(NimBLEServer* pServer) {
        Serial.println("BLE Disconnected!");
        bleConnected = false;
    }
};

void setup() {
    Serial.begin(115200);
    
    // 1️⃣ Configure Wake-up from Deep Sleep on ADS1296 Interrupt Pin
    pinMode(ADS1296_INT_PIN, INPUT);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)ADS1296_INT_PIN, 1); // Wake up when GPIO34 goes HIGH
    
    // 2️⃣ Check if ESP32 woke up due to an interrupt
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
        Serial.println("Woke up due to ADS1296 interrupt!");
    } else {
        Serial.println("Normal boot...");
    }

    // 3️⃣ Initialize SPI for ADS1296
    SPI.begin(SPI_CLK, SPI_MISO, SPI_MOSI);
    pinMode(ADS1296_CS, OUTPUT);
    digitalWrite(ADS1296_CS, HIGH);

    // 4️⃣ Initialize SD Card
    if (!SD.begin(SD_CS)) {
        Serial.println("SD Card initialization failed!");
        return;
    }
    Serial.println("SD Card initialized");

    // 5️⃣ Acquire Data from ADS1296
    String ecgData = readADS1296();
    Serial.println("ECG Data Acquired");

    // 6️⃣ Save Data to SD Card
    saveToSDCard(ecgData);
    Serial.println("Data Saved to SD Card");

    // 7️⃣ Initialize BLE & Send Data
    Serial.println("Starting BLE...");
    NimBLEDevice::init("ESP32_BLE");
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    NimBLEService* pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );

    pService->start();
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->start();
    Serial.println("BLE Advertising Started");

    // 8️⃣ Wait for BLE connection
    while (!bleConnected) {
        delay(500);
    }

    // Send ECG Data over BLE
    pCharacteristic->setValue(ecgData.c_str());
    pCharacteristic->notify();
    Serial.println("ECG Data Sent via BLE");

    delay(1000);  // Ensure transmission is completed

    // 9️⃣ Stop BLE
    NimBLEDevice::deinit(true);
    Serial.println("BLE Turned Off");

    // 🔟 Disable WiFi to Save Power
    WiFi.mode(WIFI_OFF);

    // 1️⃣1️⃣ Enter Deep Sleep Mode, waiting for next interrupt
    Serial.println("Entering Deep Sleep... Waiting for ADS1296 Interrupt");
    esp_deep_sleep_start();
}

// Function to Read Data from ADS1296
String readADS1296() {
    digitalWrite(ADS1296_CS, LOW);
    delay(10);

    byte dataBuffer[5];
    for (int i = 0; i < 5; i++) {
        dataBuffer[i] = SPI.transfer(0x00);
    }
    digitalWrite(ADS1296_CS, HIGH);

    String result = "";
    for (int i = 0; i < 5; i++) {
        result += String(dataBuffer[i], HEX) + ",";
    }
    return result;
}

// Function to Save Data to SD Card
void saveToSDCard(String data) {
    File file = SD.open("/ECG_Data.txt", FILE_APPEND);
    if (file) {
        file.println(data);
        file.close();
        Serial.println("Data saved to SD card.");
    } else {
        Serial.println("Error opening SD card file.");
    }
}

void loop() {
    // ESP32 is in deep sleep, so loop never executes
}
