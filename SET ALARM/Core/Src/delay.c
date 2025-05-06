#include "delay.h"

extern TIM_HandleTypeDef htim1;  // TIM1 được khai báo trong main.c

// Khởi tạo delay không chặn
void delay_init(void) {
    // Đảm bảo TIM1 đã được cấu hình (Prescaler = 7, ARR = 999 để ngắt mỗi 1ms)
    HAL_TIM_Base_Start_IT(&htim1);
}

// Callback ngắt của TIM1 (gọi mỗi 1ms), đc gọi trong main.c
//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
//    if (htim->Instance == TIM1) {
//        tick_ms++;  // Tăng biến đếm mili giây mỗi 1ms
//        tick_us += 1000;  // Tăng biến đếm micro giây (1000 µs = 1 ms)
//    }
//}

// Lấy giá trị tick hiện tại (mili giây)
uint32_t get_tick_ms(void) {
    return tick_ms;
}

// Lấy giá trị tick hiện tại (micro giây)
uint32_t get_tick_us(void) {
    // Kết hợp tick_ms và giá trị hiện tại của TIM1 counter để tính micro giây
    return (tick_ms * 1000) + __HAL_TIM_GET_COUNTER(&htim1);
}

// Bắt đầu độ trễ mili giây (non-blocking)
void delay_ms_nonblocking(uint32_t ms, uint32_t *start) {
    *start = get_tick_ms();
}

// Bắt đầu độ trễ micro giây (non-blocking)
void delay_us_nonblocking(uint32_t us, uint32_t *start) {
    *start = get_tick_us();
}

// Kiểm tra xem độ trễ mili giây đã hoàn tất chưa
bool is_delay_ms_complete(uint32_t start, uint32_t ms) {
    return (get_tick_ms() - start >= ms);
}

// Kiểm tra xem độ trễ micro giây đã hoàn tất chưa
bool is_delay_us_complete(uint32_t start, uint32_t us) {
    return (get_tick_us() - start >= us);
}
