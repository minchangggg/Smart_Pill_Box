/*
 * DFPlayer.h
 *
 *  Created on: May 6, 2025
 *      Author: meynchan
 */

#ifndef INC_DFPLAYER_H_
#define INC_DFPLAYER_H_

#include "stm32f1xx_hal.h"
#include "string.h"
#include "stdio.h"
#include "delay.h"

// Định nghĩa các trạng thái cho DFPlayer
typedef enum {
    DF_IDLE,            // Trạng thái nghỉ
    DF_INIT,            // Đang khởi tạo
    DF_INIT_SOURCE,     // Đang chọn nguồn (TF card)
    DF_INIT_VOLUME,     // Đang đặt âm lượng
    DF_PLAY,            // Đang phát nhạc
    DF_PAUSE,           // Đang tạm dừng
    DF_NEXT,            // Chuyển bài tiếp theo
    DF_PREVIOUS,        // Quay lại bài trước
    DF_PLAYBACK         // Tiếp tục phát nhạc
} DFPlayer_State;

// Context để quản lý trạng thái của DFPlayer
typedef struct {
    UART_HandleTypeDef *huart;  // UART sử dụng cho DFPlayer
    DFPlayer_State state;       // Trạng thái hiện tại
    uint8_t volume;             // Âm lượng
    uint32_t delay_start;       // Thời điểm bắt đầu delay
    uint32_t delay_duration;    // Thời gian delay (ms)
    uint8_t is_playing;         // 1: Đang phát, 0: Không phát
    uint8_t is_paused;          // 1: Đang tạm dừng, 0: Không tạm dừng
} DFPlayer_Context;

/*
1) Dùng con trỏ Truyền tham chiếu để thay đổi giá trị của biến
Nếu không dùng con trỏ, mà truyền df_ctx trực tiếp vào hàm 
=> Khi gọi DF_Init(df_ctx, &huart2, 20), tham số ctx trong hàm là một bản sao của df_ctx. 
=> Mọi thay đổi (như ctx.state = DF_INIT) chỉ ảnh hưởng đến bản sao này, không ảnh hưởng đến df_ctx gốc trong main.c. Sau khi hàm kết thúc, df_ctx vẫn giữ nguyên giá trị ban đầu.

2) Cấu trúc DFPlayer_Context chứa 7 biến, chiếm một lượng bộ nhớ đáng kể (khoảng 16 byte trên STM32F1). Nếu truyền cấu trúc này trực tiếp vào hàm (pass-by-value), 
mỗi lần gọi hàm sẽ tạo một bản sao của toàn bộ cấu trúc, gây lãng phí bộ nhớ và thời gian xử lý.
=> Khi sử dụng con trỏ, bạn chỉ truyền địa chỉ của cấu trúc (4 byte trên STM32F1), thay vì sao chép toàn bộ dữ liệu. Điều này giúp:
- Tiết kiệm bộ nhớ stack (bộ nhớ dùng để lưu trữ tham số và biến cục bộ trong hàm).
- Tăng hiệu suất, đặc biệt khi hàm được gọi thường xuyên (như DF_Update trong vòng lặp chính).
*/

// Hàm khởi tạo context cho DFPlayer
void DF_Init(DFPlayer_Context *ctx, UART_HandleTypeDef *huart, uint8_t volume);

// Hàm cập nhật trạng thái của DFPlayer (gọi trong vòng lặp chính)
void DF_Update(DFPlayer_Context *ctx);

// Hàm yêu cầu phát nhạc từ đầu
void DF_PlayFromStart(DFPlayer_Context *ctx);

// Hàm yêu cầu tạm dừng
void DF_Pause(DFPlayer_Context *ctx);

// Hàm yêu cầu tiếp tục phát
void DF_Playback(DFPlayer_Context *ctx);

// Hàm yêu cầu chuyển bài tiếp theo
void DF_Next(DFPlayer_Context *ctx);

// Hàm yêu cầu quay lại bài trước
void DF_Previous(DFPlayer_Context *ctx);

// Hàm kiểm tra trạng thái nút bấm
void Check_Key(DFPlayer_Context *ctx);

#endif /* INC_DFPLAYER_H_ */
