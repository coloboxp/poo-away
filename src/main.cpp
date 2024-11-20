#include <esp_system.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_bt_device.h>
#include <esp_gap_ble_api.h>
#include "oui.h"
#include "uuidgen.h"
#include <Arduino.h>

#define LED_PIN 15
#define SENSOR_PIN 5

// Fast Pair Service UUID
#define FAST_PAIR_SERVICE_UUID 0xFE2C

static esp_ble_adv_params_t ble_adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_RANDOM,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY};

uint8_t raw_adv_data[31];
uint8_t current_mode = 0;
bool isAdvertising = false;

void setRandomMac()
{
    uint8_t mac[6];
    uint32_t random = esp_random();

    // Get random OUI from list
    int oui_index = (random & 0x3) * 3;
    memcpy(mac, &oui_list[oui_index], 3);

    // Random NIC specific part
    mac[3] = (random >> 8) & 0xFF;
    mac[4] = (random >> 16) & 0xFF;
    mac[5] = (random >> 24) & 0xFF;

    // For static random address, top 2 bits must be '1'
    // and the remaining 46 bits should be random
    mac[0] &= 0x3F; // Clear top 2 bits of first byte
    mac[5] |= 0xC0; // Set top 2 bits of last byte to '11'

    esp_ble_gap_set_rand_addr(mac);
}

void setup()
{
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);

    // Initialize BLE
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_bt_controller_init(&bt_cfg);
    if (ret)
    {
        Serial.println("BLE init failed");
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret)
    {
        Serial.println("BLE enable failed");
        return;
    }

    ret = esp_bluedroid_init();
    if (ret)
    {
        Serial.println("Bluedroid init failed");
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret)
    {
        Serial.println("Bluedroid enable failed");
        return;
    }
}

void loop()
{
    float sensorValue = analogRead(SENSOR_PIN) * (3.3 / 4095.0);
    Serial.println(sensorValue);

    if (sensorValue > 0.2)
    {
        if (!isAdvertising)
        {
            setRandomMac();

            // Create Fast Pair advertising data
            uint8_t adv_data_len = 0;
            switch (current_mode)
            {
            case 0: // Google Fast Pair
                // Model ID for Fast Pair (3 bytes)
                uint32_t modelId = 0x2C8ABC; // Example model ID
                raw_adv_data[0] = 0x03;      // Length
                raw_adv_data[1] = 0x03;      // Complete List of 16-bit Service UUIDs
                raw_adv_data[2] = 0x2C;      // Fast Pair Service UUID (0xFE2C)
                raw_adv_data[3] = 0xFE;
                raw_adv_data[4] = 0x06; // Length
                raw_adv_data[5] = 0x16; // Service Data
                raw_adv_data[6] = 0x2C; // Fast Pair Service UUID
                raw_adv_data[7] = 0xFE;
                raw_adv_data[8] = modelId & 0xFF;          // Model ID (LSB)
                raw_adv_data[9] = (modelId >> 8) & 0xFF;   // Model ID
                raw_adv_data[10] = (modelId >> 16) & 0xFF; // Model ID (MSB)
                adv_data_len = 11;
                break;

                // Add other modes as needed
            }

            esp_ble_gap_config_adv_data_raw(raw_adv_data, adv_data_len);
            esp_ble_gap_start_advertising(&ble_adv_params);

            delay(500); // Advertise for 500ms
            esp_ble_gap_stop_advertising();
            isAdvertising = false;

            current_mode = (current_mode + 1) % 1; // Only one mode for now
        }
    }
    delay(100);
}
