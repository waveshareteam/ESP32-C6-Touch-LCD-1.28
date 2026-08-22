#include "i2c_bsp.h"
#include "pcf85063a.h"

static const uint8_t RTC_INT_PIN = 6;
static pcf85063a_dev_t pcf85063;
static char rtc_datetime_text[40] = { "" };
static volatile bool rtc_interrupt_flag = false;

#define APP_PRINT(...) Serial.printf(__VA_ARGS__)
#define APP_PRINTLN(message) Serial.println(message)

void IRAM_ATTR rtc_interrupt_handler(void) {
  rtc_interrupt_flag = true;
}

static pcf85063a_datetime_t add_seconds(pcf85063a_datetime_t datetime, uint8_t seconds) {
  uint16_t total_seconds = datetime.sec + seconds;
  datetime.sec = total_seconds % 60;
  datetime.min += total_seconds / 60;

  if (datetime.min >= 60) {
    datetime.hour += datetime.min / 60;
    datetime.min %= 60;
  }

  if (datetime.hour >= 24) {
    datetime.hour %= 24;
  }

  return datetime;
}

static esp_err_t clear_alarm_interrupt(bool enable_alarm) {
  uint8_t ctrl2 = 0;
  esp_err_t ret = pcf85063a_read_register(&pcf85063, PCF85063A_RTC_CTRL_2_ADDR, &ctrl2, 1);
  if (ret != ESP_OK) {
    return ret;
  }

  ctrl2 &= ~PCF85063A_RTC_CTRL_2_AF;
  if (enable_alarm) {
    ctrl2 |= PCF85063A_RTC_CTRL_2_AIE;
  } else {
    ctrl2 &= ~PCF85063A_RTC_CTRL_2_AIE;
  }

  uint8_t buf[2] = { PCF85063A_RTC_CTRL_2_ADDR, ctrl2 };
  return pcf85063a_write_register(&pcf85063, buf, 2);
}

static esp_err_t set_alarm_after_seconds(uint8_t seconds) {
  pcf85063a_datetime_t now = {};
  esp_err_t ret = pcf85063a_get_time_date(&pcf85063, &now);
  if (ret != ESP_OK) {
    return ret;
  }

  pcf85063a_datetime_t alarm_time = add_seconds(now, seconds);
  ret = pcf85063a_set_alarm(&pcf85063, alarm_time);
  if (ret != ESP_OK) {
    return ret;
  }

  ret = clear_alarm_interrupt(true);
  if (ret == ESP_OK) {
    APP_PRINT("RTC alarm will trigger at %02u:%02u:%02u\r\n",
              (unsigned int)alarm_time.hour,
              (unsigned int)alarm_time.min,
              (unsigned int)alarm_time.sec);
  }
  return ret;
}

void pcf85063_task(void *arg) {
  while (1) {
    if (rtc_interrupt_flag) {
      rtc_interrupt_flag = false;
      detachInterrupt(digitalPinToInterrupt(RTC_INT_PIN));

      uint8_t alarm_flag = 0;
      esp_err_t ret = pcf85063a_get_alarm_flag(&pcf85063, &alarm_flag);
      if (ret == ESP_OK) {
        APP_PRINT("RTC interrupt triggered, alarm flag: 0x%02X\r\n", alarm_flag);
      } else {
        APP_PRINT("Failed to read alarm flag (error: %d)\r\n", ret);
      }

      ret = clear_alarm_interrupt(false);
      if (ret != ESP_OK) {
        APP_PRINT("Failed to clear alarm interrupt (error: %d)\r\n", ret);
      }
    }

    pcf85063a_datetime_t datetime = {};
    esp_err_t ret = pcf85063a_get_time_date(&pcf85063, &datetime);
    if (ret != ESP_OK) {
      APP_PRINT("Failed to read PCF85063 (error: %d)\r\n", ret);
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }
    pcf85063a_datetime_to_str(rtc_datetime_text, sizeof(rtc_datetime_text), datetime);
    APP_PRINT("rtc: %s\r\n", rtc_datetime_text);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  APP_PRINTLN("start");

  esp_err_t ret = I2C_master_Init();
  if (ret != ESP_OK) {
    APP_PRINT("Failed to initialize I2C master (error: %d)\r\n", ret);
    return;
  }

  pinMode(RTC_INT_PIN, INPUT_PULLUP);

  ret = pcf85063a_init(&pcf85063, I2C_master_GetBusHandle(), PCF85063A_ADDRESS);
  if (ret != ESP_OK) {
    APP_PRINT("Failed to initialize PCF85063 (error: %d)\r\n", ret);
    return;
  }
  APP_PRINTLN("PCF85063A initialized successfully");

  ret = set_alarm_after_seconds(10);
  if (ret != ESP_OK) {
    APP_PRINT("Failed to set RTC alarm (error: %d)\r\n", ret);
    return;
  }
  attachInterrupt(digitalPinToInterrupt(RTC_INT_PIN), rtc_interrupt_handler, FALLING);

  BaseType_t task_created = xTaskCreate(pcf85063_task, "pcf85063_task", 3 * 1024, NULL, 3, NULL);
  if (task_created != pdPASS) {
    APP_PRINTLN("Failed to create PCF85063 task");
  }
}

void loop() {
}
