idf_component_register(SRCS "test_qmi8658.c"
                       REQUIRES esp_driver_i2c freertos
                       PRIV_REQUIRES spi_flash driver
                       INCLUDE_DIRS "")
