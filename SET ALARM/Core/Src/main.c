/* Includes ------------------------------------------------------------------*/
// Bao gồm các file tiêu đề cần thiết cho chương trình, được tự động sinh bởi STM32CubeMX
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
// Bao gồm các thư viện chuẩn của C và các thư viện dành riêng cho phần cứng
#include <string.h>              // Thư viện xử lý chuỗi (dùng cho sprintf, strlen, v.v.)
#include <stdio.h>               // Thư viện để định dạng chuỗi (dùng cho sprintf)
#include <stdbool.h>             // Thư viện hỗ trợ kiểu bool (true/false)
#include <time.h>                // Thư viện xử lý thời gian (dùng cho struct tm nếu cần)
#include <stdint.h>              // Thư viện định nghĩa các kiểu dữ liệu chuẩn (uint8_t, uint32_t, v.v.)

#include "delay.h"               // Thư viện hỗ trợ delay chính xác bằng timer

#include "ssd1306.h"             // Thư viện điều khiển màn hình OLED SSD1306
#include "fonts.h"               // Thư viện font chữ cho OLED
#include "ds1307_for_stm32_hal.h" // Thư viện điều khiển RTC DS1307 qua giao thức I2C

#include "sht4x.h"               // Thư viện điều khiển cảm biến nhiệt độ và độ ẩm SHT4x
#include "sensirion_i2c_hal.h"   // Thư viện hỗ trợ giao tiếp I2C cho cảm biến SHT4x

#include "DFPlayer.h"            // Thư viện điều khiển module DFPlayer Mini (phát nhạc MP3)
#include "sim.h"                 // Thư viện điều khiển module SIM (gửi SMS)
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// Định nghĩa cấu trúc và kiểu dữ liệu cho chương trình
typedef struct ALARM_TIME {
    uint8_t Hour;   // Giờ của báo thức (0-23)
    uint8_t Minute; // Phút của báo thức (0-59)
} ALARM_TIME;

/* SystemState => Máy trạng thái để quản lý quá trình cài đặt thời gian báo thức qua HC-05 */
typedef enum {
    /* Trạng thái nghỉ */
    IDLE,
    /* Các trạng thái để nhập giờ */
    SET_HOUR,
    SET_HOUR_INPUT,  // Trạng thái để nhập giá trị giờ
    SET_HOUR_TENS,   // Trạng thái để nhập chữ số thứ hai của giờ
    /* Các trạng thái để nhập phút */
    SET_MIN,
    SET_MIN_INPUT,   // Trạng thái để nhập giá trị phút
    SET_MIN_TENS,    // Trạng thái để nhập chữ số thứ hai của phút
    /* Xác nhận thời gian báo thức */
    CONFIRM
} SystemState;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// Định nghĩa các hằng số cho chương trình
#define INIT_RTC_TIME 1     // Khởi tạo RTC với thời gian mặc định (1: Bật, 0: Tắt)
#define PREDEFINED_INPUT 0  // Sử dụng thời gian báo thức định sẵn (1: Bật, 0: Tắt)
#define USER_INPUT 1        // Cho phép người dùng nhập thời gian báo thức (1: Bật, 0: Tắt)
#define UART_TIMEOUT 100    // Thời gian chờ tối đa cho truyền UART (tính bằng mili giây)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
// Khai báo các biến cấu hình cho ngoại vi, tự động sinh bởi STM32CubeMX
I2C_HandleTypeDef hi2c1;    // Cấu hình giao thức I2C1 (cho DS1307 và SHT4x)

TIM_HandleTypeDef htim1;    // Cấu hình Timer 1 (dùng cho delay chính xác)
TIM_HandleTypeDef htim3;    // Cấu hình Timer 3 (dùng để cập nhật thời gian mỗi giây)

UART_HandleTypeDef huart1;  // Cấu hình UART1 (giao tiếp với HC-05 để nhập thời gian)
UART_HandleTypeDef huart2;  // Cấu hình UART2 (giao tiếp với DFPlayer để phát nhạc)
UART_HandleTypeDef huart3;  // Cấu hình UART3 (giao tiếp với module SIM để gửi SMS)

/* USER CODE BEGIN PV */
// Khai báo các biến toàn cục cho chương trình
volatile uint32_t tick_ms = 0; // Biến đếm mili giây (dùng cho delay)
volatile uint32_t tick_us = 0; // Biến đếm micro giây (dùng cho delay)

const char* dayOfWeek[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"}; // Mảng chứa tên các ngày trong tuần
DS1307_TIME current_time;      // Thời gian hiện tại lấy từ DS1307
ALARM_TIME set_time = {0, 0};  // Thời gian báo thức, khởi tạo mặc định là 00:00

DFPlayer_Context df_ctx;       // Cấu trúc điều khiển DFPlayer (phát nhạc)
SMS_Context sim_ctx;           // Cấu trúc điều khiển module SIM (gửi SMS)

int current_time_to_secs = 0;  // Thời gian hiện tại tính bằng giây
int set_time_to_secs = 0;      // Thời gian báo thức tính bằng giây
int lower_bound_secs = 0;      // Giới hạn dưới của cửa sổ thời gian (set_time - 30 phút)
int upper_bound_secs = 0;      // Giới hạn trên của cửa sổ thời gian (set_time + 30 phút)

float temp = 0.0f;             // Biến lưu giá trị nhiệt độ từ SHT4x
float humi = 0.0f;             // Biến lưu giá trị độ ẩm từ SHT4x

volatile uint8_t flag_update_time = 0; // Cờ để cập nhật thời gian mỗi giây
volatile uint8_t current_box_cnt = 0;  // Biến đếm số lần nhấn nút (dùng để debug)
volatile uint8_t box_mode = 0;         // Trạng thái hộp: 0 - Đóng, 1 - Mở
volatile uint8_t alarm_completed = 0;  // Cờ đánh dấu báo thức đã hoàn tất

uint32_t tick = 0;                     // Biến debug cho sự kiện timer
volatile uint8_t already_warned_mp3 = 0; // Cờ đảm bảo nhạc MP3 chỉ phát một lần
volatile uint8_t already_warned_sms = 0; // Cờ đảm bảo SMS chỉ gửi một lần
volatile uint8_t send_sms_now = 0;     // Cờ kích hoạt gửi SMS
char sms_message[50];                  // Bộ đệm chứa nội dung tin nhắn SMS

uint8_t rxData;                        // Bộ đệm chứa dữ liệu nhận từ HC-05
SystemState current_state = IDLE;      // Trạng thái hiện tại của hệ thống
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
// Khai báo prototype các hàm cấu hình ngoại vi, tự động sinh bởi STM32CubeMX
void SystemClock_Config(void);           // Cấu hình xung nhịp hệ thống
static void MX_GPIO_Init(void);          // Cấu hình GPIO
static void MX_I2C1_Init(void);          // Cấu hình I2C1
static void MX_USART1_UART_Init(void);   // Cấu hình USART1
static void MX_TIM3_Init(void);          // Cấu hình Timer 3
static void MX_USART2_UART_Init(void);   // Cấu hình USART2
static void MX_USART3_UART_Init(void);   // Cấu hình USART3
static void MX_TIM1_Init(void);          // Cấu hình Timer 1
/* USER CODE BEGIN PFP */
// Khai báo prototype các hàm tự viết
void Convert_Time_To_Seconds(DS1307_TIME _current_time, ALARM_TIME _set_time, int *_current_time_to_secs, int *_set_time_to_secs, int *_lower_bound_secs, int *_upper_bound_secs); // Chuyển thời gian thành giây
void Display_On_Oled(DS1307_TIME _current_time, ALARM_TIME _set_time, float _temp, float _humi); // Hiển thị thông tin lên màn hình OLED
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// Hàm callback khi nhận dữ liệu UART hoàn tất
/*
- Hàm HAL_UART_RxCpltCallback là callback sau khi nhận dữ liệu:
+ Hàm này được gọi sau khi vi điều khiển đã nhận được 1 byte dữ liệu từ HC-05 (qua UART1). Dữ liệu nhận được lưu vào biến rxData.
+ Ví dụ: Khi bạn gửi ký tự 'S' từ điện thoại qua HC-05, rxData sẽ chứa giá trị 'S', và hàm này được gọi.

- Truyền dữ liệu (HAL_UART_Transmit) là phản hồi:
+ Sau khi nhận dữ liệu (ví dụ: 'S'), chương trình xử lý logic (chuyển trạng thái current_state = SET_HOUR) và gửi một phản hồi lại cho HC-05 (và đến điện thoại của bạn) để thông báo trạng thái tiếp theo.

- Chuẩn bị nhận dữ liệu tiếp theo (HAL_UART_Receive_IT):
Sau khi gửi phản hồi, chương trình gọi: HAL_UART_Receive_IT(&huart1, &rxData, 1); để chuẩn bị nhận ký tự tiếp theo từ HC-05 (ví dụ: bạn nhấn 'H' để bắt đầu nhập giờ). 
Đây là cách hoạt động của UART trong chế độ ngắt (Interrupt Mode): mỗi lần nhận xong 1 byte, bạn cần gọi lại HAL_UART_Receive_IT để sẵn sàng nhận byte tiếp theo.
*/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) { // Kiểm tra UART1 (giao tiếp với HC-05)
        switch (current_state) {
            case IDLE: // Trạng thái nghỉ
	    /*
            Điều kiện: Chờ ký tự S (START).
	    Hành động:
		Reset các biến set_time, set_time_to_mins, lower_bound_mins, upper_bound_mins về 0.
		Chuyển sang SET_HOUR.
		Gửi thông báo qua UART1: "PRESS [SET HOUR]\r\n".
	    Mục đích: Khởi động quy trình cài đặt khi nhận lệnh bắt đầu.
	    */
                if (rxData == 'S') { // Nhấn 'S' để bắt đầu cài đặt
                    set_time.Hour = 0;
                    set_time.Minute = 0;
                    set_time_to_secs = 0;
                    lower_bound_secs = 0;
                    upper_bound_secs = 0;
                    current_state = SET_HOUR;
                    HAL_UART_Transmit(&huart1, (uint8_t*)"PRESS [SET HOUR]\r\n", strlen("PRESS [SET HOUR]\r\n"), UART_TIMEOUT);
                }
                break;

            case SET_HOUR: // Trạng thái cài đặt giờ
	    /*
            Điều kiện: Chờ ký tự H (SET HOUR).
            Hành động:
		Gửi thông báo: "Enter H (0-23)\r\n".
		Chuyển sang SET_HOUR_INPUT.
            Mục đích: Yêu cầu người dùng nhập giờ (0-23).
            */
                if (rxData == 'H') { // Nhấn 'H' để nhập giờ
                    HAL_UART_Transmit(&huart1, (uint8_t*)"Enter H (0-23)\r\n", strlen("Enter H (0-23)\r\n"), UART_TIMEOUT);
                    current_state = SET_HOUR_INPUT;
                }
                break;

            case SET_HOUR_INPUT: // Nhập chữ số đầu tiên của giờ
	    /*
     	    Điều kiện: Nhận ký tự đầu tiên của giờ (0-2).
	    Hành động:
		Chuyển ký tự ASCII thành số nguyên (rxData - '0').
		Lưu vào set_time.Hour (chỉ chữ số đầu tiên).
		Chuyển sang SET_HOUR_TENS.
		Gọi HAL_UART_Receive_IT để nhận chữ số thứ hai.
   	    Mục đích: Nhận chữ số hàng chục của giờ (0-2).
            */
                if (rxData >= '0' && rxData <= '2') { // Chấp nhận giá trị từ 0-2
                    set_time.Hour = rxData - '0'; // Chuyển ASCII thành số
                    current_state = SET_HOUR_TENS;
                    HAL_UART_Receive_IT(&huart1, &rxData, 1);
                }
                break;

            case SET_HOUR_TENS: // Nhập chữ số thứ hai của giờ
	    /*
	    Điều kiện: Nhận chữ số thứ hai của giờ:
		Nếu set_time.Hour = 2, chấp nhận 0-3 (cho 20-23).
		Nếu set_time.Hour < 2, chấp nhận 0-9 (cho 00-19).
	    Hành động:
		Ghép chữ số thứ hai vào set_time.Hour (ví dụ: 2 + 3 = 23).
		Chuyển sang SET_MIN.
		Gửi thông báo: "PRESS [SET MIN]\r\n".
	    Mục đích: Hoàn tất nhập giờ và chuyển sang nhập phút.
            */
                if (set_time.Hour == 2 && rxData >= '0' && rxData <= '3') { // Giờ từ 20-23
                    set_time.Hour = (set_time.Hour * 10) + (rxData - '0');
                    current_state = SET_MIN;
                    HAL_UART_Transmit(&huart1, (uint8_t*)"PRESS [SET MIN]\r\n", strlen("PRESS [SET MIN]\r\n"), UART_TIMEOUT);
                }
                else if (set_time.Hour < 2 && rxData >= '0' && rxData <= '9') { // Giờ từ 00-19
                    set_time.Hour = (set_time.Hour * 10) + (rxData - '0');
                    current_state = SET_MIN;
                    HAL_UART_Transmit(&huart1, (uint8_t*)"PRESS [SET MIN]\r\n", strlen("PRESS [SET MIN]\r\n"), UART_TIMEOUT);
                }
                break;
		
            case SET_MIN: // Trạng thái cài đặt phút
            /*
    	    Điều kiện: Chờ ký tự M (SET MIN).
	    Hành động:
		Gửi thông báo: "Enter M (0-59)\r\n".
		Chuyển sang SET_MIN_INPUT.
	    Mục đích: Yêu cầu người dùng nhập phút (0-59).
            */
                if (rxData == 'M') { // Nhấn 'M' để nhập phút
                    HAL_UART_Transmit(&huart1, (uint8_t*)"Enter M (0-59)\r\n", strlen("Enter M (0-59)\r\n"), UART_TIMEOUT);
                    current_state = SET_MIN_INPUT;
                }
                break;

            case SET_MIN_INPUT: // Nhập chữ số đầu tiên của phút
	    /*
	    Điều kiện: Nhận ký tự đầu tiên của phút (0-5).
	    Hành động:
		Chuyển ký tự ASCII thành số nguyên.
		Lưu vào set_time.Minute (chỉ chữ số đầu tiên).
		Chuyển sang SET_MIN_TENS.
		Gọi HAL_UART_Receive_IT để nhận chữ số thứ hai.
	    Mục đích: Nhận chữ số hàng chục của phút (0-5).
            */
                if (rxData >= '0' && rxData <= '5') { // Chấp nhận giá trị từ 0-5
                    set_time.Minute = rxData - '0';
                    HAL_UART_Receive_IT(&huart1, &rxData, 1);
                    current_state = SET_MIN_TENS;
                }
                break;

            case SET_MIN_TENS: // Nhập chữ số thứ hai của phút
	    /*
	    Điều kiện: Nhận chữ số thứ hai của phút (0-9).
	    Hành động:
		Ghép chữ số thứ hai vào set_time.Minute (ví dụ: 5 + 9 = 59).
		Chuyển sang CONFIRM.
		Gửi thông báo: "Press [CONFIRM]\r\n".
    	    Mục đích: Hoàn tất nhập phút.
            */
                if (rxData >= '0' && rxData <= '9') { // Chấp nhận giá trị từ 0-9
                    set_time.Minute = (set_time.Minute * 10) + (rxData - '0');
                    current_state = CONFIRM;
                    HAL_UART_Transmit(&huart1, (uint8_t*)"Press [CONFIRM]\r\n", strlen("Press [CONFIRM]\r\n"), UART_TIMEOUT);
                }
                break;

            case CONFIRM: // Xác nhận thời gian báo thức
	    /*
    	    Điều kiện: Chờ ký tự C (CONFIRM).
	    Hành động:
		Hiển thị thời gian đã đặt: "Set to %02d:%02d\r\n".
		Quay về IDLE.
		Gửi thông báo: "DONE\r\n".
		Cập nhật tin nhắn SMS với thời gian mới.
		Tính lại các khoảng thời gian (lower_bound_mins, upper_bound_mins) bằng Convert_Time_To_Mins.
	    Mục đích: Xác nhận và lưu thời gian báo thức.
            */
                if (rxData == 'C') { // Nhấn 'C' để xác nhận
                    char confirm_msg[20];
                    sprintf(confirm_msg, "Set to %02d:%02d\r\n", set_time.Hour, set_time.Minute);
                    HAL_UART_Transmit(&huart1, (uint8_t*)confirm_msg, strlen(confirm_msg), UART_TIMEOUT);
                    current_state = IDLE;
                    HAL_UART_Transmit(&huart1, (uint8_t*)"DONE\r\n", strlen("DONE\r\n"), UART_TIMEOUT);
                    sprintf(sms_message, "You need to take your medicine at %02d:%02d", set_time.Hour, set_time.Minute);
                    Convert_Time_To_Seconds(current_time, set_time, ¤t_time_to_secs, &set_time_to_secs, &lower_bound_secs, &upper_bound_secs);
                    // Gửi thông báo thời gian báo thức ra UART để kiểm tra trên điện thoại
                    HAL_UART_Transmit(&huart1, (uint8_t*)sms_message, strlen(sms_message), UART_TIMEOUT);
                }
                break;

            default:
                break;
        }
        HAL_UART_Receive_IT(&huart1, &rxData, 1); // Chuẩn bị nhận dữ liệu tiếp theo
    }
}

// Hàm callback cho ngắt ngoài (khi nhấn nút)
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_PIN) {
    if (GPIO_PIN == GPIO_PIN_14) { // Kiểm tra ngắt từ chân GPIO PC14
        current_box_cnt++; // Tăng biến đếm số lần nhấn nút (dùng để debug)
        for (int i = 0; i < 100000; i++); // Delay đơn giản để chống dội nút

        int timeout = 0;
        while (!HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_14) && (timeout < 50000)) { // Đợi thả nút
            timeout++;
        }
        EXTI->PR |= GPIO_PIN_14; // Xóa cờ ngắt

        // Kiểm tra xem thời gian hiện tại có nằm trong cửa sổ hợp lệ [set_time - 30 phút; set_time + 30 phút)
        uint8_t is_valid_press_button = (lower_bound_secs <= upper_bound_secs)
            ? (current_time_to_secs >= lower_bound_secs && current_time_to_secs < upper_bound_secs)
            : (current_time_to_secs >= lower_bound_secs || current_time_to_secs < upper_bound_secs);

        // Kiểm tra xem có nằm trong cửa sổ phát nhạc [set_time; set_time + 30 phút)
        uint8_t in_window_mp3 = (set_time_to_secs < upper_bound_secs)
            ? (current_time_to_secs >= set_time_to_secs && current_time_to_secs < upper_bound_secs)
            : (current_time_to_secs >= set_time_to_secs || current_time_to_secs < upper_bound_secs);

        if (is_valid_press_button) { // Nếu nhấn nút trong cửa sổ hợp lệ
            box_mode = 1; // Đặt trạng thái hộp là mở
            alarm_completed = 1; // Đánh dấu báo thức đã hoàn tất
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET); // Bật đèn báo hoặc relay

            if (in_window_mp3 && df_ctx.is_playing) { // Nếu đang phát nhạc trong cửa sổ
                DF_Pause(&df_ctx); // Tạm dừng nhạc
                already_warned_mp3 = 1; // Đánh dấu đã phát nhạc
            }
        }
        else {
            box_mode = 0; // Đặt trạng thái hộp là đóng
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET); // Tắt đèn báo hoặc relay
        }
    }
}

// Hàm callback cho ngắt Timer
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM1) { // Timer 1: dùng để đếm thời gian chính xác
        tick_ms++; // Tăng biến đếm mili giây
        tick_us += 1000; // Tăng biến đếm micro giây (1000 µs = 1 ms)
    }
    if (htim->Instance == TIM3) { // Timer 3: cập nhật thời gian mỗi giây
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); // Đảo trạng thái chân PC13 (dùng để debug)
        flag_update_time = 1; // Đặt cờ để cập nhật thời gian và hiển thị

        current_time = DS1307_GetTime(); // Lấy thời gian hiện tại từ DS1307
        Convert_Time_To_Seconds(current_time, set_time, &current_time_to_secs, &set_time_to_secs, &lower_bound_secs, &upper_bound_secs);

        if (current_state != IDLE) { // Nếu đang trong trạng thái cài đặt thời gian, bỏ qua logic báo thức
            return;
        }

        // Kiểm tra xem thời gian hiện tại có nằm trong cửa sổ hợp lệ [set_time - 30 phút; set_time + 32 phút)
        uint8_t is_valid_alarm = (lower_bound_secs < (upper_bound_secs + 2 * 60) % 86400)
            ? (current_time_to_secs >= lower_bound_secs && current_time_to_secs <= (upper_bound_secs + 2 * 60) % 86400)
            : (current_time_to_secs >= lower_bound_secs || current_time_to_secs <= (upper_bound_secs + 2 * 60) % 86400);

        // Kiểm tra xem có nằm trong cửa sổ phát nhạc [set_time; set_time + 30 phút)
        uint8_t in_window_mp3 = (set_time_to_secs < upper_bound_secs)
            ? (current_time_to_secs >= set_time_to_secs && current_time_to_secs < upper_bound_secs)
            : (current_time_to_secs >= set_time_to_secs || current_time_to_secs < upper_bound_secs);

        if (is_valid_alarm) { // Nếu thời gian hiện tại nằm trong cửa sổ hợp lệ
            if (box_mode || alarm_completed) { // Nếu hộp đã mở hoặc báo thức đã hoàn tất
                if (already_warned_mp3) {
                    DF_Pause(&df_ctx); // Tạm dừng nhạc nếu đã phát
                }
                return;
            }
            else if (in_window_mp3 && !already_warned_mp3) { // Nếu trong cửa sổ phát nhạc và chưa phát
                DF_PlayFromStart(&df_ctx); // Phát nhạc từ đầu
                already_warned_mp3 = 1; // Đánh dấu đã phát nhạc
                tick = 1; // Biến debug
            }
            else if (current_time_to_secs >= upper_bound_secs) { // Nếu đã vượt quá cửa sổ phát nhạc
                if (df_ctx.is_playing) {
                    DF_Pause(&df_ctx); // Tạm dừng nhạc
                }
                if (!already_warned_sms) { // Nếu chưa gửi SMS
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET); // Bật đèn báo hoặc relay
                    tick = 2; // Biến debug
                    send_sms_now = 1; // Kích hoạt gửi SMS
                    already_warned_sms = 1; // Đánh dấu đã gửi SMS
                }
            }
        }
        else { // Nếu ngoài cửa sổ hợp lệ, reset tất cả trạng thái
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
            box_mode = 0;
            already_warned_mp3 = 0;
            send_sms_now = 0;
            already_warned_sms = 0;
            alarm_completed = 0;
            if (df_ctx.is_playing) {
                DF_Pause(&df_ctx);
            }
        }
    }
}
/* USER CODE END 0 */

/**
  * @brief  Điểm vào của chương trình
  * @retval int
  */
int main(void) {
    HAL_Init(); // Khởi tạo HAL
    SystemClock_Config(); // Cấu hình xung nhịp hệ thống
    MX_GPIO_Init(); // Cấu hình GPIO
    MX_I2C1_Init(); // Cấu hình I2C1
    MX_USART1_UART_Init(); // Cấu hình USART1
    MX_TIM3_Init(); // Cấu hình Timer 3
    MX_USART2_UART_Init(); // Cấu hình USART2
    MX_USART3_UART_Init(); // Cấu hình USART3
    MX_TIM1_Init(); // Cấu hình Timer 1
    HAL_TIM_Base_Start_IT(&htim3); // Bật Timer 3 với ngắt
    delay_init(); // Khởi tạo module delay
    DS1307_Init(&hi2c1); // Khởi tạo RTC DS1307
    SSD1306_Init(); // Khởi tạo màn hình OLED
    sensirion_i2c_init(); // Khởi tạo I2C cho SHT4x
    sht4x_init(); // Khởi tạo cảm biến SHT4x
    DF_Init(&df_ctx, &huart2, 15); // Khởi tạo DFPlayer với âm lượng 15
    sprintf(sms_message, "You need to take your medicine at %02d:%02d", set_time.Hour, set_time.Minute);
    SIM_SendSMS_Init(&sim_ctx, &huart3, "+84935120229", sms_message); // Khởi tạo module SIM

    #if INIT_RTC_TIME // Nếu bật tùy chọn khởi tạo thời gian RTC
        DS1307_SetClockHalt(0); // Bật đồng hồ
        DS1307_SetTimeZone(+7, 0); // Đặt múi giờ +7
        DS1307_SetDate(04); // Đặt ngày 04
        DS1307_SetMonth(05); // Đặt tháng 05
        DS1307_SetYear(2025); // Đặt năm 2025
        DS1307_SetDayOfWeek(0); // Đặt ngày trong tuần (0 = Chủ nhật)
        DS1307_SetHour(15); // Đặt giờ 15
        DS1307_SetMinute(00); // Đặt phút 00
        DS1307_SetSecond(40); // Đặt giây 40
    #endif
    /*
    Preprocessor directives – Chỉ thị tiền xử lý: 
    + Chỉ thị tiền xử lý là những chỉ thị được sử dụng mang mục đích để trình biên dịch xử lý những thông tin trước khi bắt đầu quá trình dịch. 
    + Tất cả các chỉ thị tiền xử lý đều bắt đầu với # và các chỉ thị tiền xử lý không phải là lệnh C/C++ vì vậy không có dấu ; khi kết thúc. 
    + Tiền xử lý được nhận dạng và xử lý trước khi mở rộng các macro (macro expansion). 
    Nên khi macro được mở rộng thành lệnh chứa chỉ thị tiền xử lý thì lệnh đó sẽ không được phát hiện. 
    Các chỉ thị tiền xử lý có thể xuất hiện ở bất kỳ đâu trong source file, nhưng chúng chỉ được áp dụng cho phần source kể từ dòng code mà chúng xuất hiện.
	Trong ngôn ngữ C, chỉ thị tiền xử lý được chia ra làm các nhóm phổ biến sau: Chỉ thị tạo/hủy định nghĩa macro (#define/#undef), Chỉ thị biên dịch có điều kiện (#if, #elif, #else, #endif, #ifdef, #ifndef)
										     Chỉ thị bao hàm tệp (#include), Chỉ thị thông báo lỗi (#error)
    Cú pháp #if, #elif, và #endif là gì?
	Đây là các directive của preprocessor (chỉ thị tiền xử lý) trong ngôn ngữ C/C++. Preprocessor là bước đầu tiên khi biên dịch code, nó xử lý các dòng bắt đầu bằng # (như #include, #define, #if, v.v.) trước khi biên dịch thực sự.
	#if, #elif, và #endif được dùng để kiểm tra điều kiện và chọn lọc code sẽ được biên dịch dựa trên giá trị của các hằng số (thường được định nghĩa bằng #define).
    */
    #if PREDEFINED_INPUT // Nếu dùng thời gian báo thức định sẵn
        set_time.Hour = 14;
        set_time.Minute = 31;
        Convert_Time_To_Seconds(current_time, set_time, &current_time_to_secs, &set_time_to_secs, &lower_bound_secs, &upper_bound_secs);
    #elif USER_INPUT // Nếu cho phép người dùng nhập thời gian
        HAL_Delay(10);
        HAL_UART_Transmit(&huart1, (uint8_t*)"ALARM_PROJECT\r\n", strlen("ALARM_PROJECT\r\n"), UART_TIMEOUT);
        HAL_Delay(10);
        HAL_UART_Transmit(&huart1, (uint8_t*)"Press [START]\r\n", strlen("Press [START]\r\n"), UART_TIMEOUT);
        HAL_UART_Receive_IT(&huart1, &rxData, 1); // Bật ngắt nhận UART
    #endif

    sprintf(sms_message, "You need to take your medicine at %02d:%02d", set_time.Hour, set_time.Minute);
    box_mode = 0; // Đặt trạng thái hộp là đóng
    already_warned_mp3 = 0; // Reset cờ phát nhạc
    send_sms_now = 0; // Reset cờ gửi SMS
    already_warned_sms = 0; // Reset cờ gửi SMS
    alarm_completed = 0; // Reset cờ báo thức hoàn tất

    while (1) { // Vòng lặp chính
        sht4x_measure_blocking_read(&temp, &humi); // Đọc nhiệt độ và độ ẩm từ SHT4x
        if (flag_update_time) { // Nếu có cờ cập nhật thời gian
            flag_update_time = 0; // Xóa cờ
            current_time = DS1307_GetTime(); // Lấy thời gian hiện tại
            Display_On_Oled(current_time, set_time, temp, humi); // Hiển thị lên OLED
        }
        if (send_sms_now) { // Nếu có cờ gửi SMS
            SIM_SendSMS_Update(&sim_ctx, 1); // Gửi SMS
            send_sms_now = 0; // Reset cờ
        }
        else {
            SIM_SendSMS_Update(&sim_ctx, 0); // Cập nhật trạng thái module SIM
        }
        DF_Update(&df_ctx); // Cập nhật trạng thái DFPlayer
    }
}

/**
  * @brief Cấu hình xung nhịp hệ thống
  * @retval None
  */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI; // Sử dụng dao động nội HSI
    RCC_OscInitStruct.HSIState = RCC_HSI_ON; // Bật HSI
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT; // Giá trị hiệu chỉnh mặc định
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON; // Bật PLL
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2; // Nguồn PLL là HSI/2
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL2; // Nhân tần số PLL lên 2
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK; // Sử dụng PLL làm nguồn xung nhịp hệ thống
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1; // Không chia tần AHB
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2; // Chia tần APB1
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1; // Không chia tần APB2
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief Khởi tạo giao thức I2C1
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void) {
    hi2c1.Instance = I2C1; // Sử dụng I2C1
    hi2c1.Init.ClockSpeed = 400000; // Tốc độ I2C 400kHz
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2; // Chu kỳ nhiệm vụ mặc định
    hi2c1.Init.OwnAddress1 = 0; // Địa chỉ của thiết bị (không dùng)
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT; // Chế độ địa chỉ 7-bit
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE; // Tắt chế độ địa chỉ kép
    hi2c1.Init.OwnAddress2 = 0; // Địa chỉ thứ hai (không dùng)
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE; // Tắt chế độ gọi chung
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE; // Bật chế độ kéo dài xung
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief Khởi tạo Timer 1
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void) {
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    htim1.Instance = TIM1; // Sử dụng Timer 1
    htim1.Init.Prescaler = 7; // Bộ chia tần
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP; // Đếm tăng
    htim1.Init.Period = 999; // Chu kỳ đếm
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1; // Không chia tần
    htim1.Init.RepetitionCounter = 0; // Không lặp lại
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE; // Tắt preload
    if (HAL_TIM_Base_Init(&htim1) != HAL_OK) {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL; // Nguồn xung nhịp nội
    if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET; // Reset trigger
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE; // Tắt chế độ Master-Slave
    if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief Khởi tạo Timer 3
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void) {
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    htim3.Instance = TIM3; // Sử dụng Timer 3
    htim3.Init.Prescaler = 7999; // Bộ chia tần
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP; // Đếm tăng
    htim3.Init.Period = 999; // Chu kỳ đếm
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1; // Không chia tần
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE; // Tắt preload
    if (HAL_TIM_Base_Init(&htim3) != HAL_OK) {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL; // Nguồn xung nhịp nội
    if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET; // Reset trigger
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE; // Tắt chế độ Master-Slave
    if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief Khởi tạo USART1 (giao tiếp với HC-05)
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void) {
    huart1.Instance = USART1; // Sử dụng USART1
    huart1.Init.BaudRate = 9600; // Tốc độ baud 9600
    huart1.Init.WordLength = UART_WORDLENGTH_8B; // Độ dài từ 8 bit
    huart1.Init.StopBits = UART_STOPBITS_1; // 1 bit dừng
    huart1.Init.Parity = UART_PARITY_NONE; // Không kiểm tra chẵn lẻ
    huart1.Init.Mode = UART_MODE_TX_RX; // Chế độ gửi và nhận
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE; // Không điều khiển dòng
    huart1.Init.OverSampling = UART_OVERSAMPLING_16; // Lấy mẫu 16 lần
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief Khởi tạo USART2 (giao tiếp với DFPlayer)
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void) {
    huart2.Instance = USART2; // Sử dụng USART2
    huart2.Init.BaudRate = 9600; // Tốc độ baud 9600
    huart2.Init.WordLength = UART_WORDLENGTH_8B; // Độ dài từ 8 bit
    huart2.Init.StopBits = UART_STOPBITS_1; // 1 bit dừng
    huart2.Init.Parity = UART_PARITY_NONE; // Không kiểm tra chẵn lẻ
    huart2.Init.Mode = UART_MODE_TX_RX; // Chế độ gửi và nhận
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE; // Không điều khiển dòng
    huart2.Init.OverSampling = UART_OVERSAMPLING_16; // Lấy mẫu 16 lần
    if (HAL_UART_Init(&huart2) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief Khởi tạo USART3 (giao tiếp với module SIM)
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void) {
    huart3.Instance = USART3; // Sử dụng USART3
    huart3.Init.BaudRate = 115200; // Tốc độ baud 115200
    huart3.Init.WordLength = UART_WORDLENGTH_8B; // Độ dài từ 8 bit
    huart3.Init.StopBits = UART_STOPBITS_1; // 1 bit dừng
    huart3.Init.Parity = UART_PARITY_NONE; // Không kiểm tra chẵn lẻ
    huart3.Init.Mode = UART_MODE_TX_RX; // Chế độ gửi và nhận
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE; // Không điều khiển dòng
    huart3.Init.OverSampling = UART_OVERSAMPLING_16; // Lấy mẫu 16 lần
    if (HAL_UART_Init(&huart3) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief Khởi tạo GPIO
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOC_CLK_ENABLE(); // Bật xung nhịp cho cổng GPIOC
    __HAL_RCC_GPIOA_CLK_ENABLE(); // Bật xung nhịp cho cổng GPIOA
    __HAL_RCC_GPIOB_CLK_ENABLE(); // Bật xung nhịp cho cổng GPIOB
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); // Đặt chân PC13 về mức thấp
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET); // Đặt chân PB15 về mức thấp
    GPIO_InitStruct.Pin = GPIO_PIN_13; // Cấu hình chân PC13
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; // Chế độ đầu ra Push-Pull
    GPIO_InitStruct.Pull = GPIO_NOPULL; // Không kéo lên/kéo xuống
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; // Tốc độ thấp
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_PIN_14; // Cấu hình chân PC14
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING; // Chế độ ngắt, kích hoạt khi tín hiệu giảm
    GPIO_InitStruct.Pull = GPIO_PULLUP; // Kéo lên
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_PIN_15; // Cấu hình chân PB15
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; // Chế độ đầu ra Push-Pull
    GPIO_InitStruct.Pull = GPIO_PULLUP; // Kéo lên
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; // Tốc độ thấp
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0); // Đặt độ ưu tiên ngắt
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn); // Bật ngắt EXTI
}

/* USER CODE BEGIN 4 */
// Hàm chuyển đổi thời gian thành giây
void Convert_Time_To_Seconds(DS1307_TIME _current_time, ALARM_TIME _set_time, int *_current_time_to_secs, int *_set_time_to_secs, int *_lower_bound_secs, int *_upper_bound_secs) {
    *_current_time_to_secs = _current_time.Hour * 3600 + _current_time.Minute * 60 + _current_time.Second; // Chuyển thời gian hiện tại thành giây
    *_set_time_to_secs = _set_time.Hour * 3600 + _set_time.Minute * 60; // Chuyển thời gian báo thức thành giây
    *_lower_bound_secs = *_set_time_to_secs - (30 * 60); // Giới hạn dưới (set_time - 30 phút)
    *_upper_bound_secs = *_set_time_to_secs + (30 * 60); // Giới hạn trên (set_time + 30 phút)

    if (*_lower_bound_secs < 0) *_lower_bound_secs += 86400; // Xử lý khi giới hạn dưới âm
    if (*_upper_bound_secs >= 86400) *_upper_bound_secs -= 86400; // Xử lý khi giới hạn trên vượt quá 1 ngày
}

// Hàm hiển thị thông tin lên màn hình OLED
void Display_On_Oled(DS1307_TIME _current_time, ALARM_TIME _set_time, float _temp, float _humi) {
    char line1_oled[21];
    char line2_oled[21];
    char line3_oled[21];
    sprintf(line1_oled, "Tem:%.1f Hum:%.1f%%", _temp, _humi); // Dòng 1: Nhiệt độ và độ ẩm
    sprintf(line2_oled, "%s,%02d/%02d %02d:%02d:%02d", dayOfWeek[_current_time.DoW], _current_time.Date, _current_time.Month, _current_time.Hour, _current_time.Minute, _current_time.Second); // Dòng 2: Ngày giờ hiện tại
    sprintf(line3_oled, "Alarm_Time:[%02d:%02d]", _set_time.Hour, _set_time.Minute); // Dòng 3: Thời gian báo thức
    SSD1306_Fill(SSD1306_COLOR_BLACK); // Xóa màn hình
    SSD1306_GotoXY(0, 10); SSD1306_Puts(line1_oled, &Font_7x10, SSD1306_COLOR_WHITE); // Hiển thị dòng 1
    SSD1306_GotoXY(0, 30); SSD1306_Puts(line2_oled, &Font_7x10, SSD1306_COLOR_WHITE); // Hiển thị dòng 2
    SSD1306_GotoXY(0, 50); SSD1306_Puts(line3_oled, &Font_7x10, SSD1306_COLOR_WHITE); // Hiển thị dòng 3
    SSD1306_UpdateScreen(); // Cập nhật màn hình
}
/* USER CODE END 4 */

/**
  * @brief Xử lý lỗi
  * @retval None
  */
void Error_Handler(void) {
    __disable_irq(); // Tắt ngắt toàn cục
    while (1) { // Vòng lặp vô hạn khi xảy ra lỗi
    }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief Báo cáo lỗi assert
  * @param file: Tên file nguồn
  * @param line: Dòng xảy ra lỗi
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line) {
    // Người dùng có thể thêm code để báo cáo lỗi (ví dụ: in tên file và dòng lỗi)
}
#endif /* USE_FULL_ASSERT */
