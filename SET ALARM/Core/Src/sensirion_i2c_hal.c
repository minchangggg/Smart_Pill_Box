// sensirion_i2c_hal.c
#include "stm32f1xx_hal.h"
#include "sensirion_i2c_hal.h"

extern I2C_HandleTypeDef hi2c1;

int8_t sensirion_i2c_write(uint8_t address, const uint8_t* data, uint16_t count) {
    return HAL_I2C_Master_Transmit(&hi2c1, (address << 1), (uint8_t*)data, count, HAL_MAX_DELAY);
}

int8_t sensirion_i2c_read(uint8_t address, uint8_t* data, uint16_t count) {
    return HAL_I2C_Master_Receive(&hi2c1, (address << 1), data, count, HAL_MAX_DELAY);
}

void sensirion_i2c_init(void) {}
void sensirion_i2c_release(void) {}