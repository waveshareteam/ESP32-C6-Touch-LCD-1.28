#include "sd_card_bsp.h"

/**
 * @brief SD card example for ESP32-C6-Touch-LCD-1.28.
 *
 * Hardware connection defaults to SPI mode:
 * MISO=GPIO22, MOSI=GPIO21, CLK=GPIO11, CS=GPIO15.
 * The card is mounted at /sd_card by the ESP-IDF FATFS SD card driver.
 */
void setup()
{
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("SD card example start");

  esp_err_t err = sd_card_init();
  if (err != ESP_OK) {
    Serial.printf("SD card init failed: %s\n", esp_err_to_name(err));
  }
}

void loop()
{
}
