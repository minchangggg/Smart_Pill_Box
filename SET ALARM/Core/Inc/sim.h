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

// Tập hợp trạng thái FSM để gửi SMS
typedef enum {
    SMS_IDLE,               // Trạng thái nghỉ, không thực hiện hành động nào.
    SMS_SEND_AT,            // Gửi lệnh AT để kiểm tra kết nối với module SIM.
    SMS_SEND_ATE0,          // Gửi lệnh ATE0 để tắt echo (phản hồi lệnh) từ module SIM.
    SMS_SET_CMGF,           // Gửi lệnh AT+CMGF=1 để đặt module SIM vào chế độ SMS dạng văn bản (text mode).
    SMS_SEND_CMGS,          // Gửi lệnh AT+CMGS="+84935120229" để chỉ định số điện thoại nhận SMS.
    SMS_SEND_MESSAGE,       // Gửi nội dung tin nhắn.
    SMS_WAIT_DONE           // Chờ hoàn tất gửi SMS (gửi Ctrl+Z và chờ phản hồi từ module).
} SMS_State;

// Cấu trúc lưu trữ trạng thái
typedef struct {
    SMS_State state;                // Trạng thái hiện tại
    UART_HandleTypeDef *huart;      // UART handle
    const char *phone;              // Số điện thoại
    const char *message;            // Tin nhắn
    uint32_t delay_start;           // Thời điểm bắt đầu delay
    uint32_t delay_ms;              // Thời gian delay hiện tại
    char cmd[64];                   // Buffer cho lệnh AT+CMGS
} SMS_Context;

/*
- Mục đích: Gửi một chuỗi AT command qua UART tới module SIM.
- Tham số:
huart: Con trỏ tới cấu trúc UART.
cmd: Chuỗi AT command (như "AT\r", "ATE0\r").
- Chi tiết:
Hàm này sử dụng HAL_UART_Transmit để gửi chuỗi lệnh tới module SIM.
- Ví dụ: SIM_SendString(&huart3, "AT\r") gửi lệnh AT để kiểm tra kết nối.
*/
void SIM_SendString(UART_HandleTypeDef *huart, const char *cmd);

/*
- Mục đích: Gửi ký tự Ctrl+Z (mã ASCII 0x1A) để kết thúc việc gửi tin nhắn SMS.
- Tham số:
huart: Con trỏ tới cấu trúc UART.
- Chi tiết: Module SIM yêu cầu gửi Ctrl+Z sau nội dung tin nhắn để xác nhận việc gửi SMS hoàn tất.
- Ví dụ: Sau khi gửi nội dung tin nhắn, SIM_SendCtrlZ(&huart3) gửi ký tự 0x1A.
*/
void SIM_SendCtrlZ(UART_HandleTypeDef *huart);

// Khởi tạo và cập nhật FSM gửi SMS không chặn
/*
- Mục đích: Khởi tạo context cho việc gửi SMS.
- Tham số:
ctx: Con trỏ tới cấu trúc SMS_Context.
huart: Con trỏ tới cấu trúc UART.
phone: Số điện thoại người nhận.
message: Nội dung tin nhắn.
- Chi tiết: Hàm này thiết lập các giá trị ban đầu cho ctx:
Gán huart, phone, message.
Đặt trạng thái ban đầu là SMS_IDLE.
Tạo lệnh AT+CMGS bằng cách định dạng số điện thoại vào ctx->cmd.
*/
void SIM_SendSMS_Init(SMS_Context *ctx, UART_HandleTypeDef *huart, const char *phone, const char *message);

/*
State machine được điều khiển bởi hàm SIM_SendSMS_Update, được gọi trong vòng lặp chính (main.c):
- Mục đích: Cập nhật state machine để gửi SMS, được gọi trong vòng lặp chính.
- Tham số:
ctx: Con trỏ tới cấu trúc SMS_Context.
send_now: Cờ để kích hoạt gửi SMS (1: gửi ngay, 0: không gửi).
=> Cơ chế hoạt động: Hàm SIM_SendSMS_Update sử dụng một cấu trúc switch-case để kiểm tra trạng thái hiện tại (ctx->state) và thực hiện các hành động tương ứng, sau đó chuyển sang trạng thái tiếp theo.
- Chi tiết: Nếu send_now = 1, state machine bắt đầu gửi SMS bằng cách chuyển từ SMS_IDLE sang SMS_SEND_AT, sau đó lần lượt qua các trạng thái:
SMS_SEND_AT: Gửi AT\r để kiểm tra kết nối.
SMS_SEND_ATE0: Gửi ATE0\r để tắt echo.
SMS_SET_CMGF: Gửi AT+CMGF=1\r để đặt chế độ SMS dạng văn bản.
SMS_SEND_CMGS: Gửi lệnh AT+CMGS với số điện thoại.
SMS_SEND_MESSAGE: Gửi nội dung tin nhắn.
SMS_WAIT_DONE: Gửi Ctrl+Z và chờ hoàn tất.
[Sử dụng delay không chặn (delay_ms_nonblocking và is_delay_ms_complete) để chờ phản hồi giữa các bước.]
*/
void SIM_SendSMS_Update(SMS_Context *ctx, uint8_t send_now);

#endif /* INC_SIM_H_ */
