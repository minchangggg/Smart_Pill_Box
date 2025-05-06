// sensirion_i2c_hal.h
#ifndef SENSIRION_I2C_HAL_H
#define SENSIRION_I2C_HAL_H

#include <stdint.h>

/*
- Mục đích: Gửi dữ liệu qua giao thức I2C đến một thiết bị có địa chỉ được chỉ định.
- Tham số:
address: Địa chỉ I2C của thiết bị (thường là 7-bit, ví dụ: 0x44 cho SHT4x).
data: Con trỏ tới mảng dữ liệu (loại uint8_t) cần gửi.
count: Số byte dữ liệu cần gửi.
- Trả về: int8_t: Giá trị trả về để báo cáo trạng thái:
+ 0: Thành công.
+ Giá trị âm (như -1): Lỗi (có thể do timeout, không tìm thấy thiết bị, v.v.).
- Chi tiết:
Hàm này được sử dụng để gửi lệnh hoặc dữ liệu (như lệnh đo lường) tới cảm biến Sensirion.
Dữ liệu được gửi dưới dạng mảng byte, và const đảm bảo rằng mảng data không bị thay đổi trong quá trình xử lý.
Trong mã của bạn, sensirion_i2c_write có thể được gọi trong sht4x.c để gửi lệnh như MEASURE tới SHT4x.
*/
int8_t sensirion_i2c_write(uint8_t address, const uint8_t* data, uint16_t count);

/*
- Mục đích: Đọc dữ liệu từ một thiết bị I2C có địa chỉ được chỉ định.
- Tham số:
address: Địa chỉ I2C của thiết bị.
data: Con trỏ tới mảng (loại uint8_t) để lưu dữ liệu đọc được.
count: Số byte dữ liệu cần đọc.
- Trả về: int8_t: Giá trị trả về để báo cáo trạng thái:
+ 0: Thành công.
+ Giá trị âm: Lỗi.
- Chi tiết:
Hàm này được sử dụng để nhận dữ liệu thô (raw data) từ cảm biến, chẳng hạn như giá trị nhiệt độ và độ ẩm từ SHT4x.
Dữ liệu đọc được sẽ được lưu vào mảng được chỉ định bởi data.
Trong main.c, sensirion_i2c_read được gọi gián tiếp qua sht4x_measure_blocking_read để lấy dữ liệu từ SHT4x.
*/
int8_t sensirion_i2c_read(uint8_t address, uint8_t* data, uint16_t count);

/*
- Mục đích: Khởi tạo giao thức I2C để sử dụng với các cảm biến Sensirion.
- Tham số: Không có.
- Trả về: Không có (void).
- Chi tiết: Hàm này được gọi để cấu hình phần cứng I2C (thường là I2C1 trong mã của bạn) và chuẩn bị cho giao tiếp.
*/
void sensirion_i2c_init(void);

/*
- Mục đích: Giải phóng tài nguyên I2C sau khi sử dụng.
- Tham số: Không có.
- Trả về: Không có (void).
- Chi tiết: Hàm này được gọi để dừng hoặc giải phóng giao tiếp I2C, thường khi chương trình kết thúc hoặc cần chuyển sang chế độ khác.
Trong mã của bạn, sensirion_i2c_release() không được gọi trực tiếp, có thể do chương trình chạy trong vòng lặp vô hạn (while (1)), nên không cần giải phóng.
*/
void sensirion_i2c_release(void);

#endif
