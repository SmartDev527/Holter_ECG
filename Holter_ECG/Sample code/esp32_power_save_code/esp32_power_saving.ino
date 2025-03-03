#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "esp_sleep.h"

#define SERVICE_UUID        "0000FFE0-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_UUID "0000FFE1-0000-1000-8000-00805F9B34FB"

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;

// Callback for client connection/disconnection
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println("Device Connected");
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("Device Disconnected");
        delay(500);  // Short delay before advertising
        pServer->getAdvertising()->start();  // Restart advertising
    }
};

void setup() {
    Serial.begin(115200);
    BLEDevice::init("ESP32-S3_LowPower");

    // Create BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create BLE Characteristic
    pCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID,
                        BLECharacteristic::PROPERTY_READ |
                        BLECharacteristic::PROPERTY_WRITE |
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
    pCharacteristic->addDescriptor(new BLE2902());

    // Start the service
    pService->start();

    // Set BLE Advertising parameters (Longer interval to save power)
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->setMinInterval(1600); // 1600 * 0.625ms = 1s
    pAdvertising->setMaxInterval(3200); // 3200 * 0.625ms = 2s
    pAdvertising->start();

    // Set BLE Transmit Power (Lower to Save Power)
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_CONN_HDL0, ESP_PWR_LVL_N12); // -12 dBm

    Serial.println("BLE Device Ready");
}

// Light Sleep Function (Saves Power)
void enterLightSleep() {
    Serial.println("Entering Light Sleep...");
    delay(500);  // Small delay before sleeping
    esp_sleep_enable_timer_wakeup(1000000); // 1 second wake-up
    esp_light_sleep_start();
    Serial.println("Woke Up!");
}

void loop() {
    if (!deviceConnected) {
        enterLightSleep();  // Enter Light Sleep if no device is connected
    } else {
        pCharacteristic->setValue("Hello from ESP32-S3!");
        pCharacteristic->notify();  // Send BLE Notification
        delay(2000);  // Send data every 2 seconds
    }
}
