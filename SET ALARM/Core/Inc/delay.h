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

// extern: Cho biết rằng các biến này được khai báo ở nơi khác (thường trong main.c hoặc delay.c) và chỉ được tham chiếu trong file header này.
// volatile: Từ khóa này đảm bảo rằng giá trị của tick_ms và tick_us không bị tối ưu hóa bởi trình biên dịch, vì chúng có thể được thay đổi bởi ngắt phần cứng (ví dụ: trong HAL_TIM_PeriodElapsedCallback).
extern volatile uint32_t tick_ms;
extern volatile uint32_t tick_us;

void delay_init(void);

/*
Mục đích: Bắt đầu một khoảng thời gian delay không chặn (non-blocking) với đơn vị mili giây.
Tham số:
ms: Thời gian delay cần thiết (tính bằng mili giây).
start: Con trỏ tới một biến lưu thời điểm bắt đầu delay (được lấy từ get_tick_ms()).
*/
void delay_ms_nonblocking(uint32_t ms, uint32_t *start);
/*
Mục đích: Tương tự như delay_ms_nonblocking, nhưng với đơn vị micro giây.
Tham số:
us: Thời gian delay cần thiết (tính bằng micro giây).
start: Con trỏ tới một biến lưu thời điểm bắt đầu delay (được lấy từ get_tick_us()).
*/
void delay_us_nonblocking(uint32_t us, uint32_t *start);

/*
Mục đích: Lấy giá trị hiện tại của bộ đếm mili giây (tick_ms).
Trả về: Giá trị của tick_ms, là số mili giây kể từ khi hệ thống khởi động.
*/
uint32_t get_tick_ms(void);

/*
Mục đích: Tương tự như delay_ms_nonblocking, nhưng với đơn vị micro giây.
Tham số:
us: Thời gian delay cần thiết (tính bằng micro giây).
start: Con trỏ tới một biến lưu thời điểm bắt đầu delay (được lấy từ get_tick_us()).
*/
uint32_t get_tick_us(void);

/*
Mục đích: Kiểm tra xem khoảng thời gian delay (ở đơn vị mili giây) đã hoàn tất chưa.
Tham số:
start: Thời điểm bắt đầu delay (lấy từ delay_ms_nonblocking).
ms: Thời gian delay mong muốn.
Trả về:
true: Nếu khoảng thời gian delay đã hoàn tất.
false: Nếu chưa hoàn tất.
*/
bool is_delay_ms_complete(uint32_t start, uint32_t ms);

/*
Mục đích: Tương tự is_delay_ms_complete, nhưng với đơn vị micro giây.
Tham số:
start: Thời điểm bắt đầu delay (lấy từ delay_us_nonblocking).
us: Thời gian delay mong muốn.
Trả về:
true: Nếu khoảng thời gian delay đã hoàn tất.
false: Nếu chưa hoàn tất.
*/
bool is_delay_us_complete(uint32_t start, uint32_t us);

#endif
