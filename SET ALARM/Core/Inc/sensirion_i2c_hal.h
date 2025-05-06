// sensirion_i2c_hal.h
#ifndef SENSIRION_I2C_HAL_H
#define SENSIRION_I2C_HAL_H

#include <stdint.h>

int8_t sensirion_i2c_write(uint8_t address, const uint8_t* data, uint16_t count);
int8_t sensirion_i2c_read(uint8_t address, uint8_t* data, uint16_t count);
void sensirion_i2c_init(void);
void sensirion_i2c_release(void);

#endif