/* An STM32 HAL library written for the DS1307 real-time clock IC. */
/* Library by @eepj www.github.com/eepj */
#ifndef DS1307_FOR_STM32_HAL_H
#define DS1307_FOR_STM32_HAL_H

#include "main.h"
/*----------------------------------------------------------------------------*/
/*
DS1307_I2C_ADDR (0x68): Địa chỉ I2C mặc định của DS1307 (7-bit address).
DS1307_REG_SECOND đến DS1307_REG_YEAR: Địa chỉ của các thanh ghi (register) trong DS1307 để lưu trữ giây, phút, giờ, ngày trong tuần (Day of Week), ngày, tháng, năm.
DS1307_REG_CONTROL (0x07): Thanh ghi điều khiển, dùng để cấu hình chế độ hoạt động (ví dụ: bật/tắt clock, xuất sóng vuông).
DS1307_REG_UTC_HR và DS1307_REG_UTC_MIN: Thanh ghi lưu múi giờ (UTC offset).
DS1307_REG_CENT (0x10): Thanh ghi lưu thế kỷ (nếu cần).
DS1307_REG_RAM (0x11): Thanh ghi bắt đầu vùng RAM của DS1307 (56 byte RAM).
DS1307_TIMEOUT (1000): Thời gian chờ tối đa (ms) khi giao tiếp I2C, dùng để tránh treo chương trình nếu giao tiếp thất bại.
*/
#define DS1307_I2C_ADDR 	0x68

#define DS1307_REG_SECOND 	0x00
#define DS1307_REG_MINUTE 	0x01
#define DS1307_REG_HOUR  	0x02
#define DS1307_REG_DOW    	0x03
#define DS1307_REG_DATE   	0x04
#define DS1307_REG_MONTH  	0x05
#define DS1307_REG_YEAR   	0x06

#define DS1307_REG_CONTROL 	0x07
#define DS1307_REG_UTC_HR	0x08
#define DS1307_REG_UTC_MIN	0x09
#define DS1307_REG_CENT    	0x10
#define DS1307_REG_RAM   	0x11
#define DS1307_TIMEOUT		1000
/*----------------------------------------------------------------------------*/
extern I2C_HandleTypeDef *_ds1307_ui2c;

/*
Định nghĩa các tần số ngắt (interrupt rate) cho sóng vuông (square wave) xuất ra từ chân SQW/OUT của DS1307:
DS1307_1HZ: 1 Hz.
DS1307_4096HZ: 4096 Hz.
DS1307_8192HZ: 8192 Hz.
DS1307_32768HZ: 32768 Hz (tần số chuẩn của dao động thạch anh).
*/
typedef enum DS1307_Rate{
	DS1307_1HZ, DS1307_4096HZ, DS1307_8192HZ, DS1307_32768HZ
} DS1307_Rate;

/*
Định nghĩa trạng thái bật/tắt sóng vuông:
DS1307_DISABLED: Tắt sóng vuông.
DS1307_ENABLED: Bật sóng vuông.
*/
typedef enum DS1307_SquareWaveEnable{
	DS1307_DISABLED, DS1307_ENABLED
} DS1307_SquareWaveEnable;

typedef struct DS1307_TIME{
	uint8_t Date;
	uint8_t Month;
	uint16_t Year;
	uint8_t DoW;
	uint8_t Hour;
	uint8_t Minute;
	uint8_t Second;
	uint8_t TimeZoneHour;
	uint8_t TimeZoneMin;
} DS1307_TIME;

/*Khởi tạo*/
void DS1307_Init(I2C_HandleTypeDef *hi2c);

/*Quản lý Clock Halt*/
// Bật/tắt chế độ dừng đồng hồ (clock halt). Giá trị halt = 1 dừng đồng hồ, halt = 0 chạy đồng hồ.
void DS1307_SetClockHalt(uint8_t halt);
// Lấy trạng thái hiện tại của clock halt (0 hoặc 1).
uint8_t DS1307_GetClockHalt(void);

/*Đọc/ghi thanh ghi*/
// Ghi một byte (val) vào thanh ghi có địa chỉ regAddr.
void DS1307_SetRegByte(uint8_t regAddr, uint8_t val);
// Đọc một byte từ thanh ghi có địa chỉ regAddr.
uint8_t DS1307_GetRegByte(uint8_t regAddr);

/*Quản lý sóng vuông*/
void DS1307_SetEnableSquareWave(DS1307_SquareWaveEnable mode);
void DS1307_SetInterruptRate(DS1307_Rate rate);

/*Đọc thời gian*/
uint8_t DS1307_GetDayOfWeek(void);
uint8_t DS1307_GetDate(void);
uint8_t DS1307_GetMonth(void);
uint16_t DS1307_GetYear(void);

uint8_t DS1307_GetHour(void);
uint8_t DS1307_GetMinute(void);
uint8_t DS1307_GetSecond(void);
int8_t DS1307_GetTimeZoneHour(void);
uint8_t DS1307_GetTimeZoneMin(void);

DS1307_TIME DS1307_GetTime(void);

/*Ghi thời gian*/
void DS1307_SetDayOfWeek(uint8_t dow);
void DS1307_SetDate(uint8_t date);
void DS1307_SetMonth(uint8_t month);
void DS1307_SetYear(uint16_t year);

void DS1307_SetHour(uint8_t hour_24mode);
void DS1307_SetMinute(uint8_t minute);
void DS1307_SetSecond(uint8_t second);
void DS1307_SetTimeZone(int8_t hr, uint8_t min);

/*Chuyển đổi BCD*/
uint8_t DS1307_DecodeBCD(uint8_t bin);
uint8_t DS1307_EncodeBCD(uint8_t dec);

#endif
