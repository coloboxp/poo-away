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
    .adv_type = ADV_TYPE_NONCONN_IND,
    .own_addr_type = BLE_ADDR_TYPE_RANDOM,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY};

uint8_t raw_adv_data[31];
uint8_t current_mode = 0;
bool isAdvertising = false;

// Add GAP event handler
void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
            esp_ble_gap_start_advertising(&ble_adv_params);
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if (param->adv_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
                Serial.println("Advertising started");
                isAdvertising = true;
            }
            break;
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if (param->adv_stop_cmpl.status == ESP_BT_STATUS_SUCCESS) {
                Serial.println("Advertising stopped");
                isAdvertising = false;
            }
            break;
        default:
            break;
    }
}

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

    // Register GAP callback
    esp_ble_gap_register_callback(gap_event_handler);
}

void loop()
{
    float sensorValue = analogRead(SENSOR_PIN) * (3.3 / 4095.0);
    Serial.println(sensorValue);

    if (sensorValue > 0.2 && !isAdvertising)
    {
        setRandomMac();

        // Create Fast Pair advertising data
        uint8_t adv_data_len = 0;
        memset(raw_adv_data, 0, sizeof(raw_adv_data));
        
        // Google Fast Pair format
        uint32_t modelId = 0x2C8ABC; // Example model ID
        
        // Service Data
        raw_adv_data[0] = 0x07;      // Length of service data
        raw_adv_data[1] = 0x16;      // Service Data AD type
        raw_adv_data[2] = 0x2C;      // Fast Pair Service UUID (0xFE2C)
        raw_adv_data[3] = 0xFE;
        raw_adv_data[4] = modelId & 0xFF;          // Model ID (LSB)
        raw_adv_data[5] = (modelId >> 8) & 0xFF;   
        raw_adv_data[6] = (modelId >> 16) & 0xFF;  // Model ID (MSB)
        raw_adv_data[7] = 0x00;      // Salt byte
        
        adv_data_len = 8;

        esp_ble_gap_config_adv_data_raw(raw_adv_data, adv_data_len);
        
        // Advertising control is now handled by the GAP event handler
        delay(1000); // Keep advertising for 1 second
        
        if (isAdvertising) {
            esp_ble_gap_stop_advertising();
        }
        
        current_mode = (current_mode + 1) % 1;
    }
    delay(100);
}
