/**
 * @file 04_WIFI_AP.ino
 * @brief Starts the ESP32-C6-Touch-LCD-1.28 board as a Wi-Fi access point and
 *        periodically logs connected station MAC addresses.
 *
 * Hardware: ESP32-C6-Touch-LCD-1.28.
 * Dependencies: Arduino ESP32 WiFi library and ESP-IDF Wi-Fi/netif APIs.
 */

#include <Arduino.h>
#include <WiFi.h>
#include "esp_err.h"
#include "esp_netif.h"
#include "esp_wifi.h"

static constexpr char WIFI_AP_SSID[] = "ESP32-C6-Touch-LCD-1.28";
static constexpr char WIFI_AP_PASSWORD[] = "waveshare";
static constexpr uint32_t SERIAL_BAUD_RATE = 115200U;
static constexpr uint32_t DEVICE_POLL_INTERVAL_MS = 5000U;

static uint32_t last_device_poll_ms = 0U;

static bool start_wifi_ap(void);
static void print_connected_devices(void);

/**
 * @brief Initializes the board Wi-Fi access point.
 */
void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);
    delay(1000);

    if (!start_wifi_ap()) {
        Serial.println("[wifi_ap] Wi-Fi AP initialization failed");
    }
}

/**
 * @brief Polls connected Wi-Fi stations without blocking the main loop.
 */
void loop()
{
    const uint32_t now_ms = millis();

    if ((now_ms - last_device_poll_ms) >= DEVICE_POLL_INTERVAL_MS) {
        last_device_poll_ms = now_ms;
        print_connected_devices();
    }
}

/**
 * @brief Starts Wi-Fi AP mode and logs the AP IP address.
 *
 * @return true if AP mode starts successfully, otherwise false.
 */
static bool start_wifi_ap(void)
{
    WiFi.mode(WIFI_AP);

    if (!WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD)) {
        Serial.printf("[wifi_ap] Failed to start AP, ssid=%s\r\n", WIFI_AP_SSID);
        return false;
    }

    const IPAddress ap_ip = WiFi.softAPIP();
    Serial.printf("[wifi_ap] Wi-Fi AP started, ssid=%s, ip=%u.%u.%u.%u\r\n",
                  WIFI_AP_SSID,
                  ap_ip[0],
                  ap_ip[1],
                  ap_ip[2],
                  ap_ip[3]);

    return true;
}

/**
 * @brief Logs the MAC addresses of stations currently connected to the AP.
 */
static void print_connected_devices(void)
{
    wifi_sta_list_t station_list = {};
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");

    if (netif == nullptr) {
        Serial.println("[wifi_ap] Failed to get AP netif handle");
        return;
    }

    const esp_err_t station_list_err = esp_wifi_ap_get_sta_list(&station_list);
    if (station_list_err != ESP_OK) {
        Serial.printf("[wifi_ap] Failed to get connected station list, err=%s\r\n",
                      esp_err_to_name(station_list_err));
        return;
    }

    Serial.printf("[wifi_ap] Connected devices: %u\r\n", station_list.num);

    for (uint8_t station_index = 0U; station_index < station_list.num; station_index++) {
        const uint8_t *mac = station_list.sta[station_index].mac;

        Serial.printf("[wifi_ap] Device %u MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                      static_cast<unsigned int>(station_index + 1U),
                      mac[0],
                      mac[1],
                      mac[2],
                      mac[3],
                      mac[4],
                      mac[5]);
    }
}
