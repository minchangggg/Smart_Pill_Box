/*
 * delay.h
 *
 *  Created on: Apr 11, 2025
 *      Author: meynchan
 */

#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f1xx_hal.h"
#include "stdbool.h"

extern volatile uint32_t tick_ms;
extern volatile uint32_t tick_us;

void delay_init(void);
void delay_ms_nonblocking(uint32_t ms, uint32_t *start);
void delay_us_nonblocking(uint32_t us, uint32_t *start);
uint32_t get_tick_ms(void);
uint32_t get_tick_us(void);
bool is_delay_ms_complete(uint32_t start, uint32_t ms);
bool is_delay_us_complete(uint32_t start, uint32_t us);

#endif
