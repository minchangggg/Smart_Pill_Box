// sensirion_common.h
#ifndef SENSIRION_COMMON_H
#define SENSIRION_COMMON_H
/*
Mục đích của thư viện. Thư viện sensirion_common.h được thiết kế để:
- Chuẩn hóa kiểu dữ liệu:
Đảm bảo rằng các kiểu dữ liệu được sử dụng trong các thư viện của Sensirion (như sht4x.h) có kích thước cố định và không phụ thuộc vào trình biên dịch hoặc kiến trúc phần cứng (ví dụ: 8-bit, 16-bit, 32-bit).
Điều này rất quan trọng khi làm việc với các cảm biến nhúng, vì các giá trị đọc từ cảm biến (như nhiệt độ, độ ẩm) thường được mã hóa dưới dạng số nguyên 16-bit hoặc 32-bit.
- Tăng tính tương thích:
Các thư viện khác của Sensirion (như sht4x, sgp30, v.v.) đều sử dụng các kiểu dữ liệu này, giúp mã nguồn đồng nhất và dễ bảo trì giữa các dự án.
- Tối ưu hóa mã nguồn:
Tên ngắn gọn (s8, u16, v.v.) giúp giảm độ dài mã nguồn, phù hợp với các vi điều khiển có bộ nhớ hạn chế như STM32F1.
- Hỗ trợ phát triển phần mềm:
Cung cấp nền tảng cho các hàm xử lý dữ liệu cảm biến, chẳng hạn như chuyển đổi giá trị thô (raw data) từ cảm biến thành giá trị thực (như độ C hoặc % độ ẩm).
*/
#include <stdint.h>
typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
#endif
