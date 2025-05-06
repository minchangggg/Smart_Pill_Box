/*
 * DFPLAYER_MINI.c
 *
 *  Created on: May 06, 2025
 *      Author: meynchan
 */

#include "DFPlayer.h"

#define DEBOUNCE_TIME 50  // milliseconds

#define Source 0x02  // TF CARD

#define Previous_Key   GPIO_PIN_1
#define Previous_Port  GPIOA
#define Pause_Key      GPIO_PIN_2
#define Pause_Port     GPIOA
#define Next_Key       GPIO_PIN_3
#define Next_Port      GPIOA

#define Start_Byte 0x7E
#define End_Byte   0xEF
#define Version    0xFF
#define Cmd_Len    0x06
#define Feedback   0x00    // If need for Feedback: 0x01, No Feedback: 0x00

// Gửi lệnh tới DFPlayer qua UART
static void Send_cmd(UART_HandleTypeDef *huart, uint8_t cmd, uint8_t Parameter1, uint8_t Parameter2)
{
    uint16_t Checksum = Version + Cmd_Len + cmd + Feedback + Parameter1 + Parameter2;
    Checksum = 0 - Checksum;

    uint8_t CmdSequence[10] = { Start_Byte, Version, Cmd_Len, cmd, Feedback, Parameter1, Parameter2, (Checksum>>8)&0x00ff, (Checksum&0x00ff), End_Byte};

    HAL_UART_Transmit(huart, CmdSequence, 10, 1000);
}

// Khởi tạo context cho DFPlayer
void DF_Init(DFPlayer_Context *ctx, UART_HandleTypeDef *huart, uint8_t volume)
{
    ctx->huart = huart;
    ctx->state = DF_INIT;
    ctx->volume = volume;
    ctx->is_playing = 1;
    ctx->is_paused = 0;
    ctx->delay_start = 0;
    ctx->delay_duration = 0;
}

// Hàm cập nhật trạng thái của DFPlayer
void DF_Update(DFPlayer_Context *ctx)
{
    switch (ctx->state)
    {
        case DF_IDLE:
            // Không làm gì, chờ yêu cầu
            break;

        case DF_INIT:
            // Bắt đầu chọn nguồn (TF card)
            Send_cmd(ctx->huart, 0x3F, 0x00, Source);
            delay_ms_nonblocking(50, &ctx->delay_start);
            ctx->delay_duration = 50;
            ctx->state = DF_INIT_SOURCE;
            break;

        case DF_INIT_SOURCE:
            // Chờ delay hoàn tất trước khi đặt âm lượng
            if (is_delay_ms_complete(ctx->delay_start, ctx->delay_duration))
            {
                Send_cmd(ctx->huart, 0x06, 0x00, ctx->volume);
                delay_ms_nonblocking(100, &ctx->delay_start);
                ctx->delay_duration = 100;
                ctx->state = DF_INIT_VOLUME;
            }
            break;

        case DF_INIT_VOLUME:
            // Chờ delay hoàn tất, sau đó chuyển về trạng thái IDLE
            if (is_delay_ms_complete(ctx->delay_start, ctx->delay_duration))
            {
                ctx->state = DF_IDLE;
            }
            break;

        case DF_PLAY:
            // Phát nhạc từ đầu
            Send_cmd(ctx->huart, 0x03, 0x00, 0x01);
            delay_ms_nonblocking(50, &ctx->delay_start);
            ctx->delay_duration = 50;
            ctx->is_playing = 1;
            ctx->is_paused = 0;
            ctx->state = DF_IDLE;
            break;

        case DF_PAUSE:
            // Tạm dừng nhạc
            Send_cmd(ctx->huart, 0x0E, 0x00, 0x00);
            delay_ms_nonblocking(50, &ctx->delay_start);
            ctx->delay_duration = 50;
            ctx->is_playing = 0;
            ctx->is_paused = 1;
            ctx->state = DF_IDLE;
            break;

        case DF_PLAYBACK:
            // Tiếp tục phát nhạc
            Send_cmd(ctx->huart, 0x0D, 0x00, 0x00);
            delay_ms_nonblocking(50, &ctx->delay_start);
            ctx->delay_duration = 50;
            ctx->is_playing = 1;
            ctx->is_paused = 0;
            ctx->state = DF_IDLE;
            break;

        case DF_NEXT:
            // Chuyển bài tiếp theo
            Send_cmd(ctx->huart, 0x01, 0x00, 0x00);
            delay_ms_nonblocking(50, &ctx->delay_start);
            ctx->delay_duration = 50;
            ctx->state = DF_IDLE;
            break;

        case DF_PREVIOUS:
            // Quay lại bài trước
            Send_cmd(ctx->huart, 0x02, 0x00, 0x00);
            delay_ms_nonblocking(50, &ctx->delay_start);
            ctx->delay_duration = 50;
            ctx->state = DF_IDLE;
            break;

        default:
            ctx->state = DF_IDLE;
            break;
    }
}

// Yêu cầu phát nhạc từ đầu
void DF_PlayFromStart(DFPlayer_Context *ctx)
{
    if (ctx->state == DF_IDLE)
    {
        ctx->state = DF_PLAY;
    }
}

// Yêu cầu tạm dừng
void DF_Pause(DFPlayer_Context *ctx)
{
    if (ctx->state == DF_IDLE)
    {
        ctx->state = DF_PAUSE;
    }
}

// Yêu cầu tiếp tục phát
void DF_Playback(DFPlayer_Context *ctx)
{
    if (ctx->state == DF_IDLE)
    {
        ctx->state = DF_PLAYBACK;
    }
}

// Yêu cầu chuyển bài tiếp theo
void DF_Next(DFPlayer_Context *ctx)
{
    if (ctx->state == DF_IDLE)
    {
        ctx->state = DF_NEXT;
    }
}

// Yêu cầu quay lại bài trước
void DF_Previous(DFPlayer_Context *ctx)
{
    if (ctx->state == DF_IDLE)
    {
        ctx->state = DF_PREVIOUS;
    }
}

// Kiểm tra trạng thái nút bấm
void Check_Key(DFPlayer_Context *ctx)
{
    static uint32_t lastTickPause = 0;
    static uint32_t lastTickNext = 0;
    static uint32_t lastTickPrev = 0;

    uint32_t now = get_tick_ms();

    if (HAL_GPIO_ReadPin(Pause_Port, Pause_Key) == GPIO_PIN_SET && (now - lastTickPause) > DEBOUNCE_TIME)
    {
        lastTickPause = now;

        if (ctx->is_playing)
        {
            DF_Pause(ctx);
        }
        else
        {
            DF_Playback(ctx);
        }
    }

    if (HAL_GPIO_ReadPin(Previous_Port, Previous_Key) == GPIO_PIN_SET && (now - lastTickPrev) > DEBOUNCE_TIME)
    {
        lastTickPrev = now;
        DF_Previous(ctx);
    }

    if (HAL_GPIO_ReadPin(Next_Port, Next_Key) == GPIO_PIN_SET && (now - lastTickNext) > DEBOUNCE_TIME)
    {
        lastTickNext = now;
        DF_Next(ctx);
    }
}
