// sht4x.c - Giao tiếp cơ bản với cảm biến SHT40
#include "sht4x.h"
#include "sensirion_i2c_hal.h"
#include "sensirion_common.h"

int8_t sht4x_init(void) {
    return 0;
}

int8_t sht4x_measure_blocking_read(float* temperature, float* humidity) {
    uint8_t cmd[2] = {0xFD}; // Command đo nhanh
    uint8_t read_data[6];
    sensirion_i2c_write(0x44, cmd, 1);
    HAL_Delay(10);
    sensirion_i2c_read(0x44, read_data, 6);

    uint16_t raw_temp = (read_data[0] << 8) | read_data[1];
    uint16_t raw_hum = (read_data[3] << 8) | read_data[4];

    *temperature = -45 + 175 * ((float)raw_temp / 65535);
    *humidity = 100 * ((float)raw_hum / 65535);

    return 0;
}