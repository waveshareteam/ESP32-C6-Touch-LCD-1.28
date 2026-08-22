#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "qmi8658.h"

#define I2C_MASTER_SCL_IO           8
#define I2C_MASTER_SDA_IO           18
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          400000

static const char *TAG = "qmi8658_example";

/**
 * @brief Initialize I2C master bus for QMI8658 communication
 * 
 * @param[out] bus_handle Pointer to store the I2C bus handle
 * @return esp_err_t
 *         - ESP_OK: I2C bus initialized successfully
 *         - Other: I2C bus initialization failed
 */
static esp_err_t i2c_master_init(i2c_master_bus_handle_t *bus_handle) {
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags.enable_internal_pullup = true
    };

    return i2c_new_master_bus(&bus_config, bus_handle);
}

/**
 * @brief FreeRTOS task for continuous QMI8658 sensor data polling and logging
 * 
 * Reads accelerometer, gyroscope, and temperature data from QMI8658 at 100ms intervals.
 * Data is formatted and logged with the configured precision.
 * 
 * @param[in] arg I2C bus handle (i2c_master_bus_handle_t) passed from xTaskCreate
 * 
 * @note This task runs indefinitely and will self-delete on initialization failure
 */
static void qmi8658_test_task(void *arg) {
    i2c_master_bus_handle_t bus_handle = (i2c_master_bus_handle_t)arg;
    qmi8658_dev_t dev;
    qmi8658_data_t data;
    
    ESP_LOGI(TAG, "Initializing QMI8658...");
    esp_err_t ret = qmi8658_init(&dev, bus_handle, QMI8658_ADDRESS_HIGH);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize QMI8658 (error: %d)", ret);
        vTaskDelete(NULL);
    }

    qmi8658_set_accel_range(&dev, QMI8658_ACCEL_RANGE_8G);
    qmi8658_set_accel_odr(&dev, QMI8658_ACCEL_ODR_1000HZ);
    qmi8658_set_gyro_range(&dev, QMI8658_GYRO_RANGE_512DPS);
    qmi8658_set_gyro_odr(&dev, QMI8658_GYRO_ODR_1000HZ);
    
    qmi8658_set_accel_unit_mps2(&dev, true);
    qmi8658_set_gyro_unit_rads(&dev, true);
    
    qmi8658_set_display_precision(&dev, 4);

    // Main sensor polling loop - runs every 100ms
    while (1) {
        // Check if new sensor data is ready
        bool ready;
        ret = qmi8658_is_data_ready(&dev, &ready);
        if (ret == ESP_OK && ready) {
            // Read and convert sensor data
            ret = qmi8658_read_sensor_data(&dev, &data);
            if (ret == ESP_OK) {
                // Log accelerometer data (m/s² or mg depending on unit setting)
                ESP_LOGI(TAG, "Accel: X=%.4f m/s², Y=%.4f m/s², Z=%.4f m/s²",
                         data.accelX, data.accelY, data.accelZ);
                // Log gyroscope data (rad/s or dps depending on unit setting)
                ESP_LOGI(TAG, "Gyro:  X=%.4f rad/s, Y=%.4f rad/s, Z=%.4f rad/s",
                         data.gyroX, data.gyroY, data.gyroZ);
                // Log temperature and timestamp
                ESP_LOGI(TAG, "Temp:  %.2f °C, Timestamp: %lu",
                         data.temperature, data.timestamp);
                ESP_LOGI(TAG, "----------------------------------------");
            } else {
                ESP_LOGE(TAG, "Failed to read sensor data (error: %d)", ret);
            }
        } else {
            ESP_LOGE(TAG, "Data not ready or error reading status (error: %d)", ret);
        }

        // Delay before next poll
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * @brief ESP-IDF application entry point
 * 
 * Initializes I2C bus and creates the sensor task for QMI8658 monitoring.
 * Returns on any initialization failure.
 */
void app_main(void)
{
    ESP_LOGI(TAG, "Initializing I2C...");
    i2c_master_bus_handle_t bus_handle;
    esp_err_t ret = i2c_master_init(&bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C (error: %d)", ret);
        return;
    }

    BaseType_t task_ret = xTaskCreate(qmi8658_test_task, "qmi8658_test_task", 4096, bus_handle, 5, NULL);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create qmi8658_test_task (error: %d)", task_ret);
        return;
    }
    ESP_LOGI(TAG, "QMI8658 task created successfully");
}