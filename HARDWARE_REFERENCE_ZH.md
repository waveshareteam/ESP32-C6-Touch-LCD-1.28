# ESP32-C6-Touch-LCD-1.28 硬件参考

> 给开发者和 AI 工具使用的硬件速查文档。内容以本仓库 `hardware/schematics/ESP32-C6-Touch-LCD-1.28.pdf` 原理图为主，并与当前示例 / BSP 引脚定义交叉核对。


## 1. 板卡概览

- 产品：`ESP32-C6-Touch-LCD-1.28`，SKU `35450`
- 主控：`ESP32-C6`，RISC-V，Wi-Fi 6 / BLE / 802.15.4
- Flash：16 MB，原理图器件为 `XM25QH128DHIQT`
- 显示：1.28 英寸圆形触摸 LCD，`240 x 240`，常用 RGB565
- 主要板载资源：GC9A01 LCD、CST816S 触摸、QMI8658 IMU、PCF85063ATL RTC、TF 卡座、锂电池充放电、电池电压采样、USB、UART、SH1.0 12PIN 扩展接口

## 2. 板载外设

| 模块 | 器件 / 功能 | 接口 | 地址 / 参数 | GPIO / 信号 |
|---|---|---|---|---|
| LCD | GC9A01 | 4-wire SPI | 240 x 240，常用 RGB565，示例/BSP 使用 SPI2 | DC=GPIO19，CS=GPIO10，SCL=GPIO11，MOSI=GPIO21，MISO=GPIO22，RST=GPIO20，BL=GPIO7 |
| 触摸 | CST816S | I2C | 7-bit 地址 `0x15` | SCL=GPIO8，SDA=GPIO18，RST=GPIO23，INT=GPIO4 |
| IMU | QMI8658 6 轴传感器 | I2C | 7-bit 地址 `0x6B` | SCL=GPIO8，SDA=GPIO18，INT1=GPIO5，INT2=GPIO4 |
| RTC | PCF85063ATL | I2C | 7-bit 地址 `0x51`，32.768 kHz 晶振 | SCL=GPIO8，SDA=GPIO18，INT=GPIO6 |
| TF 卡 | TF 卡座 | SPI | 与 LCD 共享 SPI 总线 | CS=GPIO15，SCK=GPIO11，MOSI=GPIO21，MISO=GPIO22 |
| 电池采样 | BAT 分压到 ADC | ADC | R1=200K，R8=100K | GPIO0 / BAT_ADC |
| 充电管理 | ETA6098 | 电源 | 单节锂电池充放电，ISET 由 R4 配置 | 电池接口 / Battery_Charge / BC_State |
| 3.3 V 电源 | MP1605GTF-Z | 电源 | 降压输出系统 3.3 V | 3V3 |
| USB Type-C | ESP32-C6 原生 USB | USB | 下载、日志 | USB_N=GPIO12，USB_P=GPIO13 |
| UART0 | 默认串口 | UART | 调试 / 扩展焊盘 | TXD=GPIO16，RXD=GPIO17 |

## 3. GPIO 完整分配

按 GPIO 编号升序列出已被板上电路占用或明确接出的引脚。

| GPIO | 信号名 | 连接到 | 备注 |
|---:|---|---|---|
| GPIO0 | BAT_ADC | 电池电压分压采样 | 分压网络 R1=200K / R8=100K |
| GPIO1 | GPIO1 | 扩展接口 | 原理图 GPIO 表中列出 |
| GPIO2 | GPIO2 | 扩展接口 | P1 / GPIO_OUT |
| GPIO3 | GPIO3 | 扩展接口 | P1 / GPIO_OUT |
| GPIO4 | TP_INT / IMU_INT2 | CST816S 触摸中断 / QMI8658 INT2 / 扩展接口 | 触摸和 IMU_INT2 共用该 GPIO，使用时注意中断冲突 |
| GPIO5 | IMU_INT1 | QMI8658 INT1 / 扩展接口 | - |
| GPIO6 | RTC_INT | PCF85063ATL 中断 / 扩展接口 | - |
| GPIO7 | LCD_BL | LCD 背光控制 | 可作 PWM 亮度控制 |
| GPIO8 | I2C_SCL / TP_SCL / RTC_SCL / IMU_SCL / 扩展接口 | 触摸 + IMU + RTC 共享 I2C SCL / 扩展接口 | I2C 设备见第 4 节 |
| GPIO9 | BOOT / Key1 | BOOT 按键 / 扩展接口 | Strapping pin；按键接入下载模式 |
| GPIO10 | LCD_CS | GC9A01 片选 | - |
| GPIO11 | LCD_CLK / SPI_CLK | LCD 与 TF 卡共享 SPI 时钟 | - |
| GPIO12 | USB_N | USB Type-C D- | ESP32-C6 原生 USB |
| GPIO13 | USB_P | USB Type-C D+ | ESP32-C6 原生 USB |
| GPIO15 | SD_CS | TF 卡片选 | 与 SPI 总线共享 |
| GPIO16 | TXD | UART TX 焊盘 / 扩展接口 | 扩展 TX |
| GPIO17 | RXD | UART RX 焊盘 / 扩展接口 | 扩展 RX |
| GPIO18 | I2C_SDA / TP_SDA / RTC_SDA / IMU_SDA | 触摸 + IMU + RTC 共享 I2C SDA / 扩展接口 | I2C 设备见第 4 节 |
| GPIO19 | LCD_DC | GC9A01 数据/命令 | - |
| GPIO20 | LCD_RST | GC9A01 复位 | - |
| GPIO21 | SPI_MOSI / LCD_MOSI / SD_MOSI | LCD 与 TF 卡共享 SPI MOSI | - |
| GPIO22 | SPI_MISO / LCD_MISO / SD_MISO | TF 卡 SPI MISO / LCD MISO / 扩展接口 | LCD 示例可不读回数据，TF 卡需要该脚 |
| GPIO23 | TP_RST / BC_State | CST816S 触摸复位 / 充电状态信号 / 扩展接口 | BSP 将 GPIO23 用作触摸复位；外接使用需避开冲突 |

## 4. I2C 板载设备

| 设备 | 器件 | I2C 地址 | 额外引脚 |
|---|---|---:|---|
| 触摸 | CST816S | `0x15` | RST=GPIO23，INT=GPIO4 |
| IMU | QMI8658 | `0x6B` | INT1=GPIO5，INT2=GPIO4 |
| RTC | PCF85063ATL | `0x51` | INT=GPIO6 |

共享 I2C 总线：`SCL=GPIO8`，`SDA=GPIO18`。

## 5. 扩展口

| 类型 | 信号 |
|---|---|
| 电源 | `3V3` / `GND` |
| I2C | `SCL(GPIO8)` / `SDA(GPIO18)` |
| UART | `TX(GPIO16)` / `RX(GPIO17)` |
| GPIO | `GPIO1` / `GPIO2` / `GPIO3` / `GPIO4` / `GPIO5` / `GPIO6` / `GPIO8` / `GPIO9` / `GPIO16` / `GPIO17` / `GPIO18` / `GPIO22` / `GPIO23` / `RESET` |

## 6. 使用注意

- Arduino / ESP32 项目排查外设不工作时，优先检查板型是否选为 ESP32-C6、端口、波特率、`USB CDC On Boot`、`Core Debug Level` 等设置。
- 外接 I2C 设备时，需要避免与 `0x15`、`0x6B`、`0x51` 地址冲突；触摸、IMU、RTC 共用 `GPIO8/GPIO18`。
- `GPIO4` 同时连接 CST816S 触摸中断和 QMI8658 INT2；如果两个中断都启用，需要在软件上处理共享中断或避免同时使用。
- `GPIO23` 在当前 BSP 中作为 CST816S 触摸复位使用，原理图中也能看到 `BC_State` 相关信号；不要当作普通 GPIO 随意拉高/拉低。
- `GPIO12/GPIO13` 已连接 USB Type-C，不建议当普通 GPIO 使用。
- `GPIO9` 是 BOOT 引脚，`EN/CHIP_PU` 是复位信号，不建议作为普通用户输入。
- LCD 与 TF 卡共享 SPI 时钟和 MOSI：`SCK=GPIO11`、`MOSI=GPIO21`；TF 卡额外使用 `MISO=GPIO22`、`CS=GPIO15`。

## 7. 参考资料

| 类型 | 链接 |
|---|---|
| 中文 Wiki | https://docs.waveshare.net/ESP32-C6-Touch-LCD-1.28/ |
| 英文文档 | https://docs.waveshare.com/ESP32-C6-Touch-LCD-1.28 |
| 原理图 | `hardware/schematics/ESP32-C6-Touch-LCD-1.28.pdf` |
| 板级支持包（BSP） | `examples/ESP-IDF-v5.5.2/08_LVGL_Demo/components/esp32_c6_touch_lcd_1_28` |
