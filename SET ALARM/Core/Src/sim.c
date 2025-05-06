/*
 * sim.c
 *
 *  Created on: May 4, 2025
 *      Author: meynchan
 */

#include "sim.h"

void SIM_SendString(UART_HandleTypeDef *huart, const char *cmd) {
    HAL_UART_Transmit(huart, (uint8_t *)cmd, strlen(cmd), HAL_MAX_DELAY);
}

void SIM_SendCtrlZ(UART_HandleTypeDef *huart) {
    uint8_t ctrlz = 26;
    HAL_UART_Transmit(huart, &ctrlz, 1, HAL_MAX_DELAY);
}

// Khởi tạo FSM gửi SMS
void SIM_SendSMS_Init(SMS_Context *ctx, UART_HandleTypeDef *huart, const char *phone, const char *message) {
    ctx->state = SMS_IDLE; // Ban đầu ở trạng thái IDLE, không gửi ngay
    ctx->huart = huart;
    ctx->phone = phone;
    ctx->message = message;
    ctx->delay_start = 0;
    ctx->delay_ms = 0;
}

// Cập nhật FSM gửi SMS không chặn
void SIM_SendSMS_Update(SMS_Context *ctx, uint8_t send_now) {
    switch (ctx->state) {
        case SMS_IDLE:
            if (send_now) {
                ctx->state = SMS_SEND_AT; // Chỉ bắt đầu gửi khi send_now = 1
            }
            break;

        case SMS_SEND_AT:
            SIM_SendString(ctx->huart, "AT\r\n");
            delay_ms_nonblocking(1000, &ctx->delay_start);
            ctx->delay_ms = 1000;
            ctx->state = SMS_SEND_ATE0;
            break;

        case SMS_SEND_ATE0:
            if (is_delay_ms_complete(ctx->delay_start, ctx->delay_ms)) {
                SIM_SendString(ctx->huart, "ATE0\r\n");
                delay_ms_nonblocking(500, &ctx->delay_start);
                ctx->delay_ms = 500;
                ctx->state = SMS_SET_CMGF;
            }
            break;

        case SMS_SET_CMGF:
            if (is_delay_ms_complete(ctx->delay_start, ctx->delay_ms)) {
                SIM_SendString(ctx->huart, "AT+CMGF=1\r\n");
                delay_ms_nonblocking(500, &ctx->delay_start);
                ctx->delay_ms = 500;
                ctx->state = SMS_SEND_CMGS;
            }
            break;

        case SMS_SEND_CMGS:
            if (is_delay_ms_complete(ctx->delay_start, ctx->delay_ms)) {
                sprintf(ctx->cmd, "AT+CMGS=\"%s\"\r\n", ctx->phone);
                SIM_SendString(ctx->huart, ctx->cmd);
                delay_ms_nonblocking(1000, &ctx->delay_start);
                ctx->delay_ms = 1000;
                ctx->state = SMS_SEND_MESSAGE;
            }
            break;

        case SMS_SEND_MESSAGE:
            if (is_delay_ms_complete(ctx->delay_start, ctx->delay_ms)) {
                SIM_SendString(ctx->huart, ctx->message);
                SIM_SendCtrlZ(ctx->huart);
                delay_ms_nonblocking(5000, &ctx->delay_start);
                ctx->delay_ms = 5000;
                ctx->state = SMS_WAIT_DONE;
            }
            break;

        case SMS_WAIT_DONE:
            if (is_delay_ms_complete(ctx->delay_start, ctx->delay_ms)) {
                ctx->state = SMS_IDLE; // Kết thúc gửi SMS
            }
            break;
    }
}
