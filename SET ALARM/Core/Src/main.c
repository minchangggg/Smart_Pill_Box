/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdint.h>

#include "delay.h"

#include "ssd1306.h"
#include "fonts.h"
#include "ds1307_for_stm32_hal.h"

#include "sht4x.h"
#include "sensirion_i2c_hal.h"

#include "DFPlayer.h"
#include "sim.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct ALARM_TIME{
	uint8_t Hour;   // Hour of the alarm (0-23)
	uint8_t Minute; // Minute of the alarm (0-59)
} ALARM_TIME;

/*SystemState => State machine để quản lý quá trình cài đặt thời gian báo thức qua HC-05*/
typedef enum {
    /*Trạng thái nghỉ*/
    IDLE,
    /*Các trạng thái để nhập giờ.*/
    SET_HOUR,
    SET_HOUR_INPUT,  // State to input hour value
    SET_HOUR_TENS,   // State to input second digit of hour
    /*Các trạng thái để nhập phút.*/
    SET_MIN,
    SET_MIN_INPUT,   // State to input minute value
    SET_MIN_TENS,    // State to input second digit of minute
    /*Xác nhận thời gian báo thức.*/
    CONFIRM
} SystemState;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define INIT_RTC_TIME 1     // Init RTC with default time (1: On, 0: Off)

#define PREDEFINED_INPUT 1  // Use predefined alarm time (1: On, 0: Off)

#define USER_INPUT 0        // Allow user to set alarm time (1: On, 0: Off)
#define UART_TIMEOUT 100    // Timeout for UART transmission in milliseconds
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
volatile uint32_t tick_ms = 0; // Counter for mili sec
volatile uint32_t tick_us = 0; // Counter for micro sec

const char* dayOfWeek[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
DS1307_TIME current_time;  // Current time from DS1307
ALARM_TIME set_time = {0, 0}; // Alarm time set, initialized to 00:00;       // Alarm time set

DFPlayer_Context df_ctx;
SMS_Context sim_ctx;

int current_time_to_mins = 0;
int set_time_to_mins = 0;
int lower_bound_mins = 0; // settime-30mins
int upper_bound_mins = 0; // settime+30mins

float temp = 0.0f;
float humi = 0.0f;

volatile uint8_t flag_update_time = 0; 			// Flag to update time every second

volatile uint8_t current_box_cnt = 0;  			// debug only: count how many times button pressed
volatile uint8_t box_mode = 0;         			// 0: Box closed, 1: Box opened
volatile uint8_t alarm_completed = 0;           // Flag to track if alarm has been fully handled

uint32_t tick = 0;              				// Debug flag for timer events

volatile uint8_t already_warned_mp3 = 0; 		// Ensure MP3 is played only once

volatile uint8_t already_warned_sms = 0; 		// Ensure SMS is sent only once
volatile uint8_t send_sms_now = 0;              // Flag to trigger SMS sending
char sms_message[50];  							// Buffer to store SMS message content

uint8_t rxData;       							// Buffer to receive data from HC-05
SystemState current_state = IDLE;               // Current state of the system
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_TIM1_Init(void);
/* USER CODE BEGIN PFP */
// Convert time structures to minutes
void Convert_Time_To_Mins(DS1307_TIME _current_time, ALARM_TIME _set_time, int *_current_time_to_mins, int *_set_time_to_mins, int *_lower_bound_mins, int *_upper_bound_mins);
// Display time, temperature, humidity, and alarm on OLED.
void Display_On_Oled(DS1307_TIME _current_time, ALARM_TIME _set_time, float _temp, float _humi);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        switch (current_state)
        {
            case IDLE:
	    /*
            Điều kiện: Chờ ký tự S (START).
	    Hành động:
		Reset các biến set_time, set_time_to_mins, lower_bound_mins, upper_bound_mins về 0.
		Chuyển sang SET_HOUR.
		Gửi thông báo qua UART1: "PRESS [SET HOUR]\r\n".
	    Mục đích: Khởi động quy trình cài đặt khi nhận lệnh bắt đầu.
	    */
                if (rxData == 'S') // 'S' for START
                {
                    // Reset set_time and bounds when starting new input
                    set_time.Hour = 0;
                    set_time.Minute = 0;
                    set_time_to_mins = 0;
                    lower_bound_mins = 0;
                    upper_bound_mins = 0;
                    current_state = SET_HOUR;
                    HAL_UART_Transmit(&huart1, (uint8_t*)"PRESS [SET HOUR]\r\n", strlen("PRESS [SET HOUR]\r\n"), UART_TIMEOUT);
                }
                break;

            case SET_HOUR:
	    /*
            Điều kiện: Chờ ký tự H (SET HOUR).
            Hành động:
		Gửi thông báo: "Enter H (0-23)\r\n".
		Chuyển sang SET_HOUR_INPUT.
            Mục đích: Yêu cầu người dùng nhập giờ (0-23).
            */
                if (rxData == 'H') // 'H' for SET HOUR
                {
                    HAL_UART_Transmit(&huart1, (uint8_t*)"Enter H (0-23)\r\n", strlen("Enter H (0-23)\r\n"), UART_TIMEOUT);
                    current_state = SET_HOUR_INPUT;
                }
                break;

            case SET_HOUR_INPUT:
	    /*
     	    Điều kiện: Nhận ký tự đầu tiên của giờ (0-2).
	    Hành động:
		Chuyển ký tự ASCII thành số nguyên (rxData - '0').
		Lưu vào set_time.Hour (chỉ chữ số đầu tiên).
		Chuyển sang SET_HOUR_TENS.
		Gọi HAL_UART_Receive_IT để nhận chữ số thứ hai.
   	    Mục đích: Nhận chữ số hàng chục của giờ (0-2).
            */
                if (rxData >= '0' && rxData <= '2') // First digit of hour (0-2)
                {
                    set_time.Hour = rxData - '0'; // Convert ASCII to integer
                    current_state = SET_HOUR_TENS; // Always wait for second digit
                    HAL_UART_Receive_IT(&huart1, &rxData, 1); // Wait for next digit
                }
                break;

            case SET_HOUR_TENS:
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
                if (set_time.Hour == 2 && rxData >= '0' && rxData <= '3') // Second digit for 20-23
                {
                    set_time.Hour = (set_time.Hour * 10) + (rxData - '0');
                    current_state = SET_MIN;
                    HAL_UART_Transmit(&huart1, (uint8_t*)"PRESS [SET MIN]\r\n", strlen("PRESS [SET MIN]\r\n"), UART_TIMEOUT);
                }
                else if (set_time.Hour < 2 && rxData >= '0' && rxData <= '9') // Second digit for 00-19
                {
                    set_time.Hour = (set_time.Hour * 10) + (rxData - '0');
                    current_state = SET_MIN;
                    HAL_UART_Transmit(&huart1, (uint8_t*)"PRESS [SET MIN]\r\n", strlen("PRESS [SET MIN]\r\n"), UART_TIMEOUT);
                }
                break;

            case SET_MIN:
	    /*
    	    Điều kiện: Chờ ký tự M (SET MIN).
	    Hành động:
		Gửi thông báo: "Enter M (0-59)\r\n".
		Chuyển sang SET_MIN_INPUT.
	    Mục đích: Yêu cầu người dùng nhập phút (0-59).
            */
                if (rxData == 'M') // 'M' for SET MIN
                {
                    HAL_UART_Transmit(&huart1, (uint8_t*)"Enter M (0-59)\r\n", strlen("Enter M (0-59)\r\n"), UART_TIMEOUT);
                    current_state = SET_MIN_INPUT;
                }
                break;

            case SET_MIN_INPUT:
	    /*
	    Điều kiện: Nhận ký tự đầu tiên của phút (0-5).
	    Hành động:
		Chuyển ký tự ASCII thành số nguyên.
		Lưu vào set_time.Minute (chỉ chữ số đầu tiên).
		Chuyển sang SET_MIN_TENS.
		Gọi HAL_UART_Receive_IT để nhận chữ số thứ hai.
	    Mục đích: Nhận chữ số hàng chục của phút (0-5).
            */
                if (rxData >= '0' && rxData <= '5') // First digit of minute (0-5)
                {
                    set_time.Minute = rxData - '0'; // Convert ASCII to integer
                    HAL_UART_Receive_IT(&huart1, &rxData, 1); // Wait for next digit
                    current_state = SET_MIN_TENS;
                }
                break;

            case SET_MIN_TENS:
	    /*
	    Điều kiện: Nhận chữ số thứ hai của phút (0-9).
	    Hành động:
		Ghép chữ số thứ hai vào set_time.Minute (ví dụ: 5 + 9 = 59).
		Chuyển sang CONFIRM.
		Gửi thông báo: "Press [CONFIRM]\r\n".
    	    Mục đích: Hoàn tất nhập phút.
            */
                if (rxData >= '0' && rxData <= '9') // Second digit for 00-59
                {
                    set_time.Minute = (set_time.Minute * 10) + (rxData - '0');
                    current_state = CONFIRM;
                    HAL_UART_Transmit(&huart1, (uint8_t*)"Press [CONFIRM]\r\n", strlen("Press [CONFIRM]\r\n"), UART_TIMEOUT);
                }
                break;

            case CONFIRM:
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
                if (rxData == 'C') // 'C' for CONFIRM
                {
                    char confirm_msg[20];
                    sprintf(confirm_msg, "Set to %02d:%02d\r\n", set_time.Hour, set_time.Minute);
                    HAL_UART_Transmit(&huart1, (uint8_t*)confirm_msg, strlen(confirm_msg), UART_TIMEOUT);
                    current_state = IDLE; // Reset to initial state
                    HAL_UART_Transmit(&huart1, (uint8_t*)"DONE\r\n", strlen("DONE\r\n"), UART_TIMEOUT);
                    // Update SMS message with the new alarm time
                    sprintf(sms_message, "You need to take your medicine at %02d:%02d", set_time.Hour, set_time.Minute);
                    // Recalculate bounds after setting new alarm time
                    Convert_Time_To_Mins(current_time, set_time, &current_time_to_mins, &set_time_to_mins, &lower_bound_mins, &upper_bound_mins);
                }
                break;

            default:
                break;
        }

        // Prepare for next reception
        HAL_UART_Receive_IT(&huart1, &rxData, 1);
    }
}

// External interrupt callback for button press on PC14.
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_PIN)
{
    if (GPIO_PIN == GPIO_PIN_14)
    {
//    	HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_15); // Test button
		current_box_cnt++; // // Increment button press counter for debugging
		for(int i = 0; i < 100000; i++); // Simple debounce delay

		// Wait for button release with timeout
		int timeout = 0;
		while(!HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_14) && (timeout < 50000)) // Đợi đến khi thả nút nhấn trong khoảng thời gian nhất định
		{
			timeout++;
		}
		EXTI->PR |= GPIO_PIN_14; // Clear interrupt pending bit

		// Check if current time is within valid window [ timeset-30mins ; timeset+30mins )
		uint8_t is_valid_press_button = (lower_bound_mins <= upper_bound_mins)
			? (current_time_to_mins >= lower_bound_mins && current_time_to_mins <= upper_bound_mins)
			: (current_time_to_mins >= lower_bound_mins || current_time_to_mins <= upper_bound_mins);
	    // Check if within music window [ set_time ; set_time+30mins )
	    uint8_t in_window_mp3 = (set_time_to_mins < upper_bound_mins)
		    ? (current_time_to_mins >= set_time_to_mins && current_time_to_mins <= upper_bound_mins)
		    : (current_time_to_mins >= set_time_to_mins || current_time_to_mins <= upper_bound_mins);

		if (is_valid_press_button)
		{
			box_mode = 1;  // Box opened
			alarm_completed = 1;  // Mark alarm as completed
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);

			// Pause music only if within music window and music is playing
//			if (in_window_mp3 && df_ctx.state == DF_IDLE && df_ctx.is_playing)
			if (in_window_mp3 && df_ctx.is_playing)
			{
				DF_Pause(&df_ctx); // Pause the music playback
				already_warned_mp3 = 1; // Ensure music doesn't play again in this cycle
			}
		}
		else
		{
			box_mode = 0; // Box remains closed outside the time window
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
		}
    }
}

// Timer interrupt callback (every 1 second)
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1)
    {
        tick_ms++;        // Increment millisecond counter
        tick_us += 1000;  // Increment microsecond counter (1000 µs = 1 ms)
    }
    if (htim->Instance == TIM3)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        flag_update_time = 1; // Set flag to update display and logic

        // Convert to minutes
        current_time = DS1307_GetTime();
        Convert_Time_To_Mins(current_time, set_time, &current_time_to_mins, &set_time_to_mins, &lower_bound_mins, &upper_bound_mins);

        // Only process alarm logic if not in the process of setting time
        if (current_state != IDLE)
        {
            return; // Skip alarm logic while setting time
        }

        // Check if current time is within valid window [set_time-30mins ; set_time+30mins + 3mins)
        uint8_t is_valid_alarm = (lower_bound_mins < (upper_bound_mins + 3) % 1440)
            ? (current_time_to_mins >= lower_bound_mins && current_time_to_mins <= (upper_bound_mins + 3) % 1440)
            : (current_time_to_mins >= lower_bound_mins || current_time_to_mins <= (upper_bound_mins + 3) % 1440);

        // Check if within music window [set_time ; set_time+30mins)
        uint8_t in_window_mp3 = (set_time_to_mins < upper_bound_mins)
            ? (current_time_to_mins >= set_time_to_mins && current_time_to_mins <= upper_bound_mins)
            : (current_time_to_mins >= set_time_to_mins || current_time_to_mins <= upper_bound_mins);

        if (is_valid_alarm)
        {
            if (box_mode || alarm_completed)
            {
                if (already_warned_mp3)
                {
                    DF_Pause(&df_ctx); // Ensure music is paused if box is open
                }
                return; // Box already opened, no need to play music or send SMS
            }
            else if (in_window_mp3 && !already_warned_mp3)
            {
                // Play music
                DF_PlayFromStart(&df_ctx);
                already_warned_mp3 = 1;
                tick = 1; // Debug flag
            }
            else if ((current_time_to_mins > upper_bound_mins) && !already_warned_sms)
            {
                // Send SMS only once
                DF_Pause(&df_ctx);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
                tick = 2; // Debug flag
                send_sms_now = 1;
                already_warned_sms = 1; // Set flag to prevent further SMS
            }
        }
        else
        {
            // Reset all states outside valid window
        	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
            box_mode = 0;           // Ensure box is closed
            already_warned_mp3 = 0; // Reset music flag
            send_sms_now = 0;       // Reset SMS trigger
            already_warned_sms = 0; // Reset SMS flag
            alarm_completed = 0;    // Reset alarm completion flag
        }
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  MX_TIM3_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start_IT(&htim3);

  // Initialize delay module using Timer 1
  delay_init();

  // Initialize DS1307, OLED, and SHT4x
  DS1307_Init(&hi2c1);        // Initialize RTC
  SSD1306_Init();             // Initialize OLED display
  sensirion_i2c_init();       // Initialize I2C HAL wrapper
  sht4x_init();               // Initialize SHT4x sensor

  // Initialize DFPlayer with volume 10
  DF_Init(&df_ctx, &huart2, 15);

  // Initialize SMS message with default alarm time
  sprintf(sms_message, "You need to take your medicine at %02d:%02d", set_time.Hour, set_time.Minute);
  SIM_SendSMS_Init(&sim_ctx, &huart3, "+84935120229", sms_message);

  // Set initial RTC time if needed
  #if INIT_RTC_TIME
	  DS1307_SetClockHalt(0);
	  DS1307_SetTimeZone(+7, 0);
	  DS1307_SetDate(04);
	  DS1307_SetMonth(05);
	  DS1307_SetYear(2025);
	  DS1307_SetDayOfWeek(0);
	  DS1307_SetHour(15);
	  DS1307_SetMinute(00);
	  DS1307_SetSecond(40);
  #endif

  // Set alarm time based on input mode
  #if PREDEFINED_INPUT
	  set_time.Hour = 14;
	  set_time.Minute = 31;
	  Convert_Time_To_Mins(current_time, set_time, &current_time_to_mins, &set_time_to_mins, &lower_bound_mins, &upper_bound_mins);
  #elif USER_INPUT
	  // Initial message to HC-05
	  HAL_Delay(10);
	  HAL_UART_Transmit(&huart1, (uint8_t*)"ALARM_PROJECT\r\n", strlen("ALARM_PROJECT\r\n"), UART_TIMEOUT);
	  HAL_Delay(10);
	  HAL_UART_Transmit(&huart1, (uint8_t*)"Press [START]\r\n", strlen("Press [START]\r\n"), UART_TIMEOUT);

	  // Enable UART receive interrupt
	  HAL_UART_Receive_IT(&huart1, &rxData, 1);
  #endif

  // Update SMS message with the set alarm time
  sprintf(sms_message, "You need to take your medicine at %02d:%02d", set_time.Hour, set_time.Minute);

  // Initialize system state
  box_mode = 0;           // Ensure box is closed
  already_warned_mp3 = 0; // Reset music warning flag
  send_sms_now = 0;       // Reset SMS trigger
  already_warned_sms = 0; // Reset SMS warning flag
  alarm_completed = 0;    // Reset alarm completion flag
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  // Read temperature and humidity from SHT4x sensor
	  sht4x_measure_blocking_read(&temp, &humi);

	  // Update display and alarm logic if time update is triggered
	  if (flag_update_time)
	  {
		  flag_update_time = 0; // Clear update flag
		  current_time = DS1307_GetTime(); // Fetch current time
		  Display_On_Oled(current_time, set_time, temp, humi); // Update OLED display
	  }

	  // Manage SMS state machine
	  if (send_sms_now)
	  {
		  SIM_SendSMS_Update(&sim_ctx, 1); // Trigger SMS sending
		  send_sms_now = 0; // Reset trigger immediately after starting SMS
	  }
	  else
	  {
		  SIM_SendSMS_Update(&sim_ctx, 0); // Continue SMS state machine without triggering
	  }

	  // Update DFPlayer state machine
	  DF_Update(&df_ctx);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 7;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 999;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 7999;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PC14 */
  GPIO_InitStruct.Pin = GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PB15 */
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void Convert_Time_To_Mins(DS1307_TIME _current_time, ALARM_TIME _set_time, int *_current_time_to_mins, int *_set_time_to_mins, int *_lower_bound_mins, int *_upper_bound_mins)
{
    // The permitted range: from (alarm - 30 minutes) to (alarm + 30 minutes)
    *_current_time_to_mins = _current_time.Hour * 60 + _current_time.Minute;
    *_set_time_to_mins = _set_time.Hour * 60 + _set_time.Minute;
    *_lower_bound_mins = *_set_time_to_mins - 30;
    *_upper_bound_mins = *_set_time_to_mins + 30;

    // Handle if lower_bound is negative: wrap around to the previous day
    if (*_lower_bound_mins < 0) *_lower_bound_mins += 1440; // 1440 minutes = 24 hours
    // Handle if upper_bound exceeds 23:59
    if (*_upper_bound_mins >= 1440) *_upper_bound_mins -= 1440;
}
void Display_On_Oled(DS1307_TIME _current_time, ALARM_TIME _set_time, float _temp, float _humi)
{
    char line1_oled[21];
    char line2_oled[21];
    char line3_oled[21];

    sprintf(line1_oled, "Tem:%.1f Hum:%.1f%%", _temp, _humi);
    sprintf(line2_oled, "%s,%02d/%02d %02d:%02d:%02d", dayOfWeek[_current_time.DoW], _current_time.Date, _current_time.Month, _current_time.Hour, _current_time.Minute, _current_time.Second); // "%04d", _current_time.Year
    sprintf(line3_oled, "Alarm_Time:[%02d:%02d]", _set_time.Hour, _set_time.Minute);

    SSD1306_Fill(SSD1306_COLOR_BLACK);
    SSD1306_GotoXY(0,10); SSD1306_Puts(line1_oled, &Font_7x10, SSD1306_COLOR_WHITE);
    SSD1306_GotoXY(0,30); SSD1306_Puts(line2_oled, &Font_7x10, SSD1306_COLOR_WHITE);
    SSD1306_GotoXY(0,50); SSD1306_Puts(line3_oled, &Font_7x10, SSD1306_COLOR_WHITE);
    SSD1306_UpdateScreen();
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
