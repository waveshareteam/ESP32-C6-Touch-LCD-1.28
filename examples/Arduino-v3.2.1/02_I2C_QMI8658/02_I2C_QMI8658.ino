#include "qmi8658c.h"
#include "i2c_bsp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "QMI8658_EXAMPLE";

void setup()
{
  Serial.begin(115200);
  esp_err_t ret = I2C_master_Init();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "I2C init failed: %d", ret);
    return;
  }

  BaseType_t task_ret = xTaskCreate(qmi8658c_example, "qmi8658c_example", 3000, NULL, 2, NULL);
  if (task_ret != pdPASS) {
    ESP_LOGE(TAG, "Failed to create qmi8658c task");
  }
}
void loop()
{
  
}
