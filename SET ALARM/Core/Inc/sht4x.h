// sht4x.h - Thư viện header cho cảm biến SHT40
#ifndef SHT4X_H
#define SHT4X_H

#include <stdint.h>

int8_t sht4x_init(void);
int8_t sht4x_measure_blocking_read(float* temperature, float* humidity);

#endif