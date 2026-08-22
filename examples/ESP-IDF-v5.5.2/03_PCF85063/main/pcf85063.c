#include <stdio.h>
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// I2C configuration
#define I2C_MASTER_SCL_IO    8         // Set the SCL pin
#define I2C_MASTER_SDA_IO    18        // Set the SDA pin
#define I2C_MASTER_NUM       I2C_NUM_0 // I2C port number
#define I2C_MASTER_FREQ_HZ   100000    // I2C frequency
#define I2C_MASTER_TX_BUF_DISABLE 0    // Disable tx buffer
#define I2C_MASTER_RX_BUF_DISABLE 0    // Disable rx buffer
#define I2C_TIMEOUT_MS       1000

#define PCF85063_ADDR        0x51      // PCF85063 I2C address

#define PCF85063_REG_CTRL2          0x01
#define PCF85063_REG_SECONDS        0x04
#define PCF85063_REG_ALARM_SECONDS  0x0B
#define PCF85063_CTRL2_AIE          BIT(7)
#define PCF85063_CTRL2_AF           BIT(6)
#define PCF85063_ALARM_DISABLE      BIT(7)

#define RTC_INT_GPIO         GPIO_NUM_6
#define RTC_INT_PIN_BIT      (1ULL << RTC_INT_GPIO)
#define RTC_INT_INTR_TYPE    GPIO_INTR_NEGEDGE

// Queue and task configuration
#define RTC_INT_QUEUE_SIZE   10
#define RTC_INT_TASK_STACK   2048
#define RTC_INT_TASK_PRIO    10

static const char *TAG = "PCF85063";
static QueueHandle_t rtc_int_queue = NULL;

/**
 * @brief Initialize I2C master driver
 * @return ESP_OK on success, ESP_FAIL otherwise
 */
esp_err_t i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C configuration failed");
        return err;
    }
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

/**
 * @brief Read RTC register(s)
 * @param reg_addr Register address to read from
 * @param data Pointer to buffer for storing read data
 * @param len Number of bytes to read
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if data is NULL or len is 0
 */
esp_err_t rtc_read_reg(uint8_t reg_addr, uint8_t *data, size_t len) {
    if (data == NULL || len == 0) {
        ESP_LOGE(TAG, "Invalid parameter: data pointer is NULL or len is 0");
        return ESP_ERR_INVALID_ARG;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (PCF85063_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (PCF85063_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

/**
 * @brief Write RTC register(s)
 * @param reg_addr Register address to write to
 * @param data Pointer to data buffer
 * @param len Number of bytes to write
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if data is NULL or len is 0
 */
esp_err_t rtc_write_reg(uint8_t reg_addr, uint8_t *data, size_t len) {
    if (data == NULL || len == 0) {
        ESP_LOGE(TAG, "Invalid parameter: data pointer is NULL or len is 0");
        return ESP_ERR_INVALID_ARG;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (PCF85063_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_write(cmd, data, len, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

/**
 * @brief Convert BCD to decimal
 * @param val BCD encoded value
 * @return Decimal value
 */
uint8_t bcd_to_dec(uint8_t val) {
    return (val >> 4) * 10 + (val & 0x0F);
}

/**
 * @brief Convert decimal to BCD
 * @param val Decimal value
 * @return BCD encoded value
 */
uint8_t dec_to_bcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

/**
 * @brief Read current time from RTC and log it
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t rtc_get_time(void) {
    uint8_t data[7];
    esp_err_t ret = rtc_read_reg(PCF85063_REG_SECONDS, data, 7); // PCF85063 time registers start from 0x04
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read time registers");
        return ret;
    }

    uint8_t seconds = bcd_to_dec(data[0] & 0x7F); // Masking out the stop bit
    uint8_t minutes = bcd_to_dec(data[1]);
    uint8_t hours = bcd_to_dec(data[2]);
    uint8_t day = bcd_to_dec(data[3]);
    uint8_t weekday = bcd_to_dec(data[4]);
    uint8_t month = bcd_to_dec(data[5] & 0x1F); // Masking out the century bit
    uint8_t year = bcd_to_dec(data[6]);

    ESP_LOGI(TAG, "Current Time: %02d:%02d:%02d weekday=%02d %02d/%02d/%04d", hours, minutes, seconds, weekday, day, month, 2000 + year);
    return ESP_OK;
}

/**
 * @brief Set time on the RTC
 * @param hours Hours (0-23)
 * @param minutes Minutes (0-59)
 * @param seconds Seconds (0-59)
 * @param day Day of month (1-31)
 * @param month Month (1-12)
 * @param year Year (0-99, represents 2000-2099)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t rtc_set_time(uint8_t hours, uint8_t minutes, uint8_t seconds, uint8_t day, uint8_t month, uint8_t year) {
    uint8_t data[7] = {
        dec_to_bcd(seconds),
        dec_to_bcd(minutes),
        dec_to_bcd(hours),
        dec_to_bcd(day),
        dec_to_bcd(0), // Weekday, you can set this if needed
        dec_to_bcd(month),
        dec_to_bcd(year)
    };
    return rtc_write_reg(PCF85063_REG_SECONDS, data, 7);
}

/**
 * @brief Enable RTC alarm interrupt and set alarm for next minute
 * @details Sets the alarm to trigger at the start of the next minute,
 *          then enables the alarm interrupt on PCF85063
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t rtc_enable_alarm_for_next_minute(void) {
    uint8_t time_data[7];
    esp_err_t err = rtc_read_reg(PCF85063_REG_SECONDS, time_data, 7);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read current time for alarm setup");
        return err;
    }

    uint8_t current_minute = bcd_to_dec(time_data[1]);
    uint8_t alarm_minute = (current_minute + 1) % 60;

    uint8_t alarm_regs[5] = {
        dec_to_bcd(0),            // match second 00
        dec_to_bcd(alarm_minute), // match next minute
        PCF85063_ALARM_DISABLE,   // disable hour match
        PCF85063_ALARM_DISABLE,   // disable day match
        PCF85063_ALARM_DISABLE    // disable weekday match
    };
    err = rtc_write_reg(PCF85063_REG_ALARM_SECONDS, alarm_regs, sizeof(alarm_regs));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write alarm registers");
        return err;
    }

    uint8_t ctrl2;
    err = rtc_read_reg(PCF85063_REG_CTRL2, &ctrl2, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read RTC control register");
        return err;
    }

    ctrl2 &= ~PCF85063_CTRL2_AF; // clear stale alarm flag before waiting for a new edge
    ctrl2 |= PCF85063_CTRL2_AIE; // enable alarm interrupt
    err = rtc_write_reg(PCF85063_REG_CTRL2, &ctrl2, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable alarm interrupt");
        return err;
    }

    ESP_LOGI(TAG, "Alarm set for next minute: %02d", alarm_minute);
    return ESP_OK;
}

static esp_err_t rtc_clear_alarm_flag(void) {
    uint8_t ctrl2;
    esp_err_t err = rtc_read_reg(PCF85063_REG_CTRL2, &ctrl2, 1);
    if (err != ESP_OK) {
        return err;
    }

    ctrl2 &= ~PCF85063_CTRL2_AF;
    return rtc_write_reg(PCF85063_REG_CTRL2, &ctrl2, 1);
}

static void IRAM_ATTR gpio_isr_handler(void *arg) {
    uint32_t gpio_num = (uint32_t)arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(rtc_int_queue, &gpio_num, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

/**
 * @brief Task to handle RTC interrupts from queue
 * @param arg Unused parameter (required by FreeRTOS)
 */
static void rtc_int_task(void *arg) {
    (void)arg; // Mark parameter as intentionally unused
    uint32_t io_num;
    while (xQueueReceive(rtc_int_queue, &io_num, portMAX_DELAY)) {
        ESP_LOGI(TAG, "RTC interrupt detected on IO%u", io_num);
        esp_err_t err = rtc_clear_alarm_flag();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to clear RTC alarm flag");
        }
    }
}

static esp_err_t rtc_interrupt_gpio_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = RTC_INT_PIN_BIT,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = RTC_INT_INTR_TYPE,
    };
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        return err;
    }

    err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }
    err = gpio_isr_handler_add(RTC_INT_GPIO, gpio_isr_handler, (void *)RTC_INT_GPIO);
    return err;
}

void app_main(void) {
    esp_err_t err = i2c_master_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C initialization failed");
        return;
    }

    err = rtc_set_time(12, 30, 45, 27, 9, 24); // Example time
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Time set successfully");
    } else {
        ESP_LOGE(TAG, "Failed to set time");
    }

    rtc_int_queue = xQueueCreate(RTC_INT_QUEUE_SIZE, sizeof(uint32_t));
    if (rtc_int_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create RTC interrupt queue");
        return;
    }
    
    BaseType_t task_created = xTaskCreate(rtc_int_task, "rtc_int_task", RTC_INT_TASK_STACK, NULL, RTC_INT_TASK_PRIO, NULL);
    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create rtc_int_task");
        vQueueDelete(rtc_int_queue);
        return;
    }

    err = rtc_interrupt_gpio_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize RTC interrupt GPIO (IO6)");
        return;
    }

    err = rtc_enable_alarm_for_next_minute();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "RTC alarm interrupt setup failed");
    }

    while (1) {
        err = rtc_get_time();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to get time");
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
