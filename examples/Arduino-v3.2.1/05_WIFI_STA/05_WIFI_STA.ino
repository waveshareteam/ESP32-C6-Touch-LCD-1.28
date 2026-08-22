#include <WiFi.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>

/**
 * @file 05_WIFI_STA.ino
 * @brief ESP32-C6 Arduino WiFi station example.
 *
 * Hardware: ESP32-C6-Touch-LCD-1.28. No external wiring is required for WiFi.
 * Dependencies: ESP32 Arduino core and the built-in WiFi library.
 *
 * Configure credentials by defining WIFI_SSID and WIFI_PASSWORD in the build
 * flags, or replace the placeholder values below for local testing only.
 */

#ifndef WIFI_SSID
#define WIFI_SSID "YOUR_WIFI_SSID"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#endif

#define SERIAL_BAUD_RATE 115200U
#define WIFI_CONNECT_TIMEOUT_MS 30000UL
#define WIFI_STATUS_INTERVAL_MS 500UL

static bool connect_wifi(const char *wifi_ssid, const char *wifi_password);
static void console_print(const char *message);
static void console_println(const char *message);
static void console_printf(const char *format, ...);
static void print_ip_address(const IPAddress &ip_address);

/**
 * @brief Initializes serial output and connects the board to WiFi in STA mode.
 */
void setup()
{
  Serial.begin(SERIAL_BAUD_RATE);
  delay(1500);

  console_println("[WiFi] Serial output ready");
  WiFi.mode(WIFI_STA);

  if (!connect_wifi(WIFI_SSID, WIFI_PASSWORD))
  {
    console_printf("[WiFi] Connection failed for SSID: %s\n", WIFI_SSID);
    return;
  }

  print_ip_address(WiFi.localIP());
}

/**
 * @brief Main application loop.
 */
void loop()
{
}

/**
 * @brief Connects to a WiFi access point before the timeout expires.
 *
 * @param wifi_ssid WiFi network name.
 * @param wifi_password WiFi network password.
 * @return true if the station is connected, false otherwise.
 */
static bool connect_wifi(const char *wifi_ssid, const char *wifi_password)
{
  const bool has_valid_ssid = (wifi_ssid != nullptr) && (wifi_ssid[0] != '\0');
  const bool has_valid_password = (wifi_password != nullptr);

  if (!has_valid_ssid || !has_valid_password)
  {
    console_println("[WiFi] Invalid WiFi credentials");
    return false;
  }

  console_printf("[WiFi] Connecting to SSID: %s\n", wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_password);

  const uint32_t start_time_ms = millis();
  uint32_t last_status_time_ms = start_time_ms;

  while (WiFi.status() != WL_CONNECTED)
  {
    const uint32_t current_time_ms = millis();

    if ((current_time_ms - start_time_ms) >= WIFI_CONNECT_TIMEOUT_MS)
    {
      console_println("[WiFi] Connection timeout");
      return false;
    }

    if ((current_time_ms - last_status_time_ms) >= WIFI_STATUS_INTERVAL_MS)
    {
      console_print(".");
      last_status_time_ms = current_time_ms;
    }

    yield();
  }

  console_println("");
  console_println("[WiFi] Connected");
  return true;
}

/**
 * @brief Prints application output through Serial so it remains visible when
 * Arduino Core Debug Level is set to None.
 */
static void console_print(const char *message)
{
  Serial.print(message);
}

/**
 * @brief Prints one line of application output through Serial.
 */
static void console_println(const char *message)
{
  Serial.println(message);
}

/**
 * @brief Prints formatted application output through Serial.
 */
static void console_printf(const char *format, ...)
{
  char buffer[128];
  va_list args;
  va_start(args, format);
  const int length = vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  if (length < 0)
  {
    return;
  }

  Serial.print(buffer);
}

/**
 * @brief Prints an IPv4 address without allocating an Arduino String.
 *
 * @param ip_address IP address assigned to the station interface.
 */
static void print_ip_address(const IPAddress &ip_address)
{
  console_printf("[WiFi] IP Address: %u.%u.%u.%u\n",
                 ip_address[0],
                 ip_address[1],
                 ip_address[2],
                 ip_address[3]);
}
