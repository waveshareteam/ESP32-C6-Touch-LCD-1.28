#include <Arduino_GFX_Library.h>

/**
 * @file 06_HelloWorld_GFX.ino
 * @brief Arduino_GFX GC9A01 hello world example for ESP32-C6 Touch LCD 1.28.
 *
 * Hardware connection:
 * - LCD DC: GPIO19
 * - LCD CS: GPIO10
 * - LCD SCK: GPIO11
 * - LCD MOSI: GPIO21
 * - LCD MISO: not connected
 * - LCD RST: GPIO20
 * - LCD backlight: GPIO7
 *
 * Dependency: Arduino_GFX_Library.
 */

static constexpr int8_t LCD_BACKLIGHT_PIN = 7;
static constexpr int8_t LCD_DC_PIN = 19;
static constexpr int8_t LCD_CS_PIN = 10;
static constexpr int8_t LCD_SCK_PIN = 11;
static constexpr int8_t LCD_MOSI_PIN = 21;
static constexpr int8_t LCD_MISO_PIN = -1;
static constexpr int8_t LCD_RST_PIN = 20;
static constexpr uint8_t LCD_ROTATION = 0;
static constexpr uint16_t LCD_WIDTH = 240;
static constexpr uint16_t LCD_HEIGHT = 240;
static constexpr uint32_t SERIAL_BAUD_RATE = 115200;
static constexpr uint32_t SPLASH_INTERVAL_MS = 1000;
static constexpr uint32_t STARTUP_MESSAGE_MS = 5000;
static constexpr uint32_t LOOP_UPDATE_INTERVAL_MS = 1000;

static Arduino_HWSPI lcd_bus(LCD_DC_PIN, LCD_CS_PIN, LCD_SCK_PIN, LCD_MOSI_PIN, LCD_MISO_PIN);
static Arduino_GC9A01 lcd(&lcd_bus, LCD_RST_PIN, LCD_ROTATION, true, LCD_WIDTH, LCD_HEIGHT);

static uint32_t last_update_ms = 0;

/**
 * @brief Initialize serial output, the LCD panel, and the startup demo screen.
 */
void setup(void)
{
#ifdef DEV_DEVICE_INIT
  DEV_DEVICE_INIT();
#endif

  Serial.begin(SERIAL_BAUD_RATE);
  Serial.println("[gfx] Arduino_GFX Hello World example");

  if (!lcd.begin())
  {
    Serial.println("[gfx] LCD initialization failed");
    while (true)
    {
      delay(1000);
    }
  }

  pinMode(LCD_BACKLIGHT_PIN, OUTPUT);
  digitalWrite(LCD_BACKLIGHT_PIN, HIGH);

  lcd.fillScreen(RGB565_RED);
  delay(SPLASH_INTERVAL_MS);
  lcd.fillScreen(RGB565_BLUE);
  delay(SPLASH_INTERVAL_MS);
  lcd.fillScreen(RGB565_GREEN);
  delay(SPLASH_INTERVAL_MS);
  lcd.fillScreen(RGB565_BLACK);

  lcd.setCursor(10, 10);
  lcd.setTextColor(RGB565_RED);
  lcd.println("Hello World!");

  delay(STARTUP_MESSAGE_MS);
  last_update_ms = millis();
}

/**
 * @brief Periodically draw "Hello World!" at random positions without blocking.
 */
void loop()
{
  const uint32_t now_ms = millis();

  if ((now_ms - last_update_ms) >= LOOP_UPDATE_INTERVAL_MS)
  {
    last_update_ms = now_ms;

    lcd.setCursor(random(lcd.width()), random(lcd.height()));
    lcd.setTextColor(random(0xffff), random(0xffff));
    lcd.setTextSize(random(1, 6), random(1, 6), random(2));
    lcd.println("Hello World!");
  }
}
