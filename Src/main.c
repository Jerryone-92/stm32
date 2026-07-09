/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "cmsis_os.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include <stdio.h>
#include <string.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "soft_i2c.h"
#include "oled.h"
#include "encoder.h"
#include "menu.h"
extern volatile uint32_t step_count;
#include "mpu6050.h"
#include "cmsis_os.h"
extern UART_HandleTypeDef huart1;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint8_t btn_pressed_flag = 0; // 按键标志，中断里设置
volatile uint32_t step_count = 0;
volatile int32_t debug_mag_sq = 0;   // 可保留用于调试，但非必须
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
 * @brief EXTI 中断回调，处理 EC11 按键按下事件
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_12) {
        if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) == GPIO_PIN_RESET) {
            btn_pressed_flag = 1;
        }
    }
}

// 任务函数声明
void Task_Display(void *argument);
void Task_MenuCtrl(void *argument);
void Task_MPU6050(void *argument);
void Task_Bluetooth(void *argument);

// 计步状态变量（仅本任务使用）
static uint8_t step_state = 0;         // 0: 等待高于阈值, 1: 等待低于阈值
static uint32_t last_step_tick = 0;    // 上一次计步的系统节拍
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

  MX_SPI1_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */

  OLED_Init();        // 初始化 OLED 屏幕

  Encoder_Init();     // 启动编码器
  Menu_Init();        // 菜单初始状态（从 RTC 读取时间
  Soft_I2C_Init();
  MPU6050_Init();

  xTaskCreate(Task_MPU6050, "MPU6050", 256, NULL, 3, NULL);
  xTaskCreate(Task_Display,   // 任务函数名
                 "Display",      // 任务名（用于调试）
                 256,            // 任务栈大小（words）
                 NULL,           // 任务参数
                 2,              // 任务优先级
                 NULL);          // 任务句柄（不需保存）

     xTaskCreate(Task_MenuCtrl,
                 "MenuCtrl",
                 256,
                 NULL,
                 1,              // 优先级略低于显示任务
                 NULL);

     xTaskCreate(Task_Bluetooth, "BT", 256, NULL, 1, NULL);   // 优先级1，与菜单同级

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/**
 * @brief 显示任务：每 50ms 调用一次 Menu_Display() 刷新屏幕
 */
void Task_Display(void *argument) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    for (;;) {
        Menu_Display();                         // 恢复这行
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50));
    }
}

/**
 * @brief 菜单交互任务：每 20ms 扫描一次编码器和按键，并调用状态机处理
 */
void Task_MenuCtrl(void *argument) {
    int8_t enc_diff;
    uint8_t btn;
    for (;;) {
        enc_diff = Encoder_GetDiff();
        btn = btn_pressed_flag;
        if (btn) btn_pressed_flag = 0; // 消费标志
        Menu_Process(enc_diff, btn);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
/**
 * @brief MPU6050 计步任务，每 20ms 执行一次
 */
void Task_MPU6050(void *argument) {
    int16_t ax, ay, az;
    int32_t base_sq = 0;
    int32_t threshold_high_sq, threshold_low_sq;
    const TickType_t MIN_STEP_INTERVAL = pdMS_TO_TICKS(250);
    uint8_t step_state = 0;
    uint32_t last_tick = 0;

    // 等待传感器稳定
    vTaskDelay(pdMS_TO_TICKS(500));

    // 采集静止基准（取20次平均）
    for (int i = 0; i < 20; i++) {
        if (MPU6050_ReadAccel(&ax, &ay, &az) == HAL_OK) {
            int32_t mag_sq = (int32_t)ax*ax + (int32_t)ay*ay + (int32_t)az*az;
            base_sq += mag_sq;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    base_sq /= 20;

    // 设置阈值：上升沿 > 基准 + 6%，下降沿 < 基准 + 1.5%
    threshold_high_sq = base_sq + (base_sq >> 4);   // base_sq * 1.0625
    threshold_low_sq  = base_sq + (base_sq >> 6);   // base_sq * 1.0156

    for (;;) {
        if (MPU6050_ReadAccel(&ax, &ay, &az) == HAL_OK) {
            int32_t mag_sq = (int32_t)ax*ax + (int32_t)ay*ay + (int32_t)az*az;
            debug_mag_sq = mag_sq;  // 可选，调试用

            if (mag_sq > threshold_high_sq && step_state == 0) {
                step_state = 1;
            }
            if (mag_sq < threshold_low_sq && step_state == 1) {
                if ((xTaskGetTickCount() - last_tick) >= MIN_STEP_INTERVAL) {
                    step_count++;
                    last_tick = xTaskGetTickCount();
                }
                step_state = 0;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void Task_Bluetooth(void *argument) {
    char msg[64];
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;

    for (;;) {
        // 读取时间
        HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

        // 构造字符串
        sprintf(msg, "Steps:%lu Time:%02d:%02d:%02d Date:20%02d-%02d-%02d\r\n",
                step_count,
                sTime.Hours, sTime.Minutes, sTime.Seconds,
                sDate.Year, sDate.Month, sDate.Date);

        // 通过串口发送（蓝牙会转发）
        HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 1000);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
#ifdef USE_FULL_ASSERT
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
