/*
 * sim.h
 *
 *  Created on: May 4, 2025
 *      Author: meynchan
 */

#ifndef INC_SIM_H_
#define INC_SIM_H_

#include "stm32f1xx_hal.h"
#include "string.h"
#include "stdio.h"
#include "delay.h"

// Trạng thái FSM để gửi SMS
typedef enum {
    SMS_IDLE,
    SMS_SEND_AT,
    SMS_SEND_ATE0,
    SMS_SET_CMGF,
    SMS_SEND_CMGS,
    SMS_SEND_MESSAGE,
    SMS_WAIT_DONE
} SMS_State;

typedef struct {
    SMS_State state;                // Trạng thái hiện tại
    UART_HandleTypeDef *huart;      // UART handle
    const char *phone;              // Số điện thoại
    const char *message;            // Tin nhắn
    uint32_t delay_start;           // Thời điểm bắt đầu delay
    uint32_t delay_ms;              // Thời gian delay hiện tại
    char cmd[64];                   // Buffer cho lệnh AT+CMGS
} SMS_Context;

// Gửi chuỗi AT command
void SIM_SendString(UART_HandleTypeDef *huart, const char *cmd);

// Gửi Ctrl+Z
void SIM_SendCtrlZ(UART_HandleTypeDef *huart);

// Khởi tạo và cập nhật FSM gửi SMS không chặn
void SIM_SendSMS_Init(SMS_Context *ctx, UART_HandleTypeDef *huart, const char *phone, const char *message);
void SIM_SendSMS_Update(SMS_Context *ctx, uint8_t send_now);

#endif /* INC_SIM_H_ */
