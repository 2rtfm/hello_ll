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
#include "dma.h"
#include "gpio.h"
#include "i2c.h"
#include "rtc.h"
#include "spi.h"
#include "stm32f103x6.h"
#include "stm32f1xx_ll_tim.h"
#include "tim.h"
#include "usart.h"
#include "usb.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "aht20.h"
#include "i2c_ll.h"
#include "keyled.h"
#include "uart.h"
#include "us_delay.h"
#include <stdint.h>
#include <string.h>
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
uint8_t recv_data[50];
char hum[7];
char temp[7];
char msg[50];
uint8_t input_flag;
AHT20_Status aht20_status;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void UART_Handle_Recv(UART_Ctx *ctx);
void UART_Handle_Recv_IDLE(UART_Ctx *ctx, uint8_t size);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void UART_Handle_Recv_IT(UART_Ctx *ctx) {
  LED_Toggle();
  UART_Transmit_RecvData_IT(ctx, recv_data, 1);
  input_flag = recv_data[0];
  UART_SendData_IT(ctx, recv_data, 1);
  UART_RecvData_IT(ctx, 1);
};

void UART_Handle_Recv_DMA(UART_Ctx *ctx) {
  LED_Toggle();
  UART_SendData_DMA(ctx, recv_data, 2);
  UART_RecvData_DMA(ctx, recv_data, 2);
}

void UART_Handle_Recv_IDLE(UART_Ctx *ctx, uint8_t size) {
  LED_Toggle();
  input_flag = recv_data[0];
  UART_SendData_DMA(ctx, recv_data, size);
  UART_RecvData_IDLE(ctx, recv_data, 50);
}

void I2C_Error_Callback(I2C_Ctx *ctx, I2C_LL_Error error) {
  if (error.I2C_ERROR_BERR) {
    /* 总线错误复位后重新初始化 */
    MX_I2C1_Init();
    strcpy(msg, "Error: BERR\n");
  }
  if (error.I2C_ERROR_AF) {
    strcpy(msg, "Error: AF\n");
  }
  if (error.I2C_ERROR_ARLO) {
    strcpy(msg, "Error: ARLO\n");
  }
  if (error.I2C_ERROR_OVR) {
    strcpy(msg, "Error: OVR\n");
  }
  UART_SendData_DMA(UART1, (uint8_t *)msg, strlen(msg));
  ctx->status = I2C_STATUS_IDLE;
}

void I2C_MasterTx_Callback(I2C_Ctx *ctx) {
  if (ctx->addr == AHT20_ADDR) {
    if (aht20_status == AHT20_STATUS_SENDING_MESURE) {
      aht20_status = AHT20_STATUS_SENDING_COMPLETE;
    }
  }
}

void I2C_MasterRx_Callback(I2C_Ctx *ctx) {
  if (ctx->addr == AHT20_ADDR) {
    if (aht20_status == AHT20_STATUS_READING_MESURE) {
      aht20_status = AHT20_STATUS_READING_COMPLETE;
    }
  }
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */
  DWT_Init();
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_USB_PCD_Init();
  MX_RTC_Init();
  MX_USART2_UART_Init();
  MX_TIM1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  AHT20_Init();
  LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH1);
  LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH2);
  LL_TIM_EnableIT_CC1(TIM1);
  LL_TIM_EnableIT_CC2(TIM1);
  LL_TIM_EnableCounter(TIM1);
  LL_TIM_CC_EnableChannel(TIM3, LL_TIM_CHANNEL_CH1);
  LL_TIM_EnableCounter(TIM3);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  // UART_RecvData_DMA(recv_data, 2);
  UART_RecvData_IDLE(UART1, recv_data, 50);
  UART_RecvData_IT(UART2, 1);
  // UART_RecvData_IDLE(UART2, recv_data, 50);
  while (1) {
    // if (ScanKey()) {
    //   LED_Toggle();
    //   if (LED_GetState()) {
    //     UART_SendData_DMA(UART1, (uint8_t *)"Off\n", 4);
    //     UART_SendData_IT(UART2, (uint8_t *)"Off\n", 4);
    //   } else {
    //     UART_SendData_DMA(UART1, (uint8_t *)"On\n", 3);
    //     UART_SendData_IT(UART2, (uint8_t *)"On\n", 3);
    //   }
    // }
    if (input_flag == 'r') {
      if (aht20_status == AHT20_STATUS_IDLE) {
        AHT20_Measure_IT();
        aht20_status = AHT20_STATUS_SENDING_MESURE;
      }
    }
    if (aht20_status == AHT20_STATUS_SENDING_COMPLETE) {
      LL_mDelay(80);
      AHT20_Recv_IT();
      aht20_status = AHT20_STATUS_READING_MESURE;
    }
    if (aht20_status == AHT20_STATUS_READING_COMPLETE) {
      AHT20_Format(temp, hum);
      aht20_status = AHT20_STATUS_IDLE;
      strcpy(msg, "\nTemp: ");
      strcat(msg, temp);
      strcat(msg, "°C\nHum:  ");
      strcat(msg, hum);
      strcat(msg, " %\n");
      UART_SendData_DMA(UART1, (uint8_t *)msg, strlen(msg));
      UART_SendData_IT(UART2, (uint8_t *)msg, strlen(msg));
      LL_mDelay(20);
    }
    if (input_flag == 't') {
      UART_SendU32Bin(UART1, LL_TIM_GetCounter(TIM1));
      UART_SendByte(UART1, '\n');
    }
    if (input_flag == 'e') {
      LL_mDelay(10);
      LL_GPIO_SetOutputPin(TRIG_GPIO_Port, TRIG_Pin);
      delay_us(50);
      LL_GPIO_ResetOutputPin(TRIG_GPIO_Port, TRIG_Pin);
      LL_TIM_SetCounter(TIM1, 0);
    }
    if (input_flag == 'u') {
      LL_TIM_OC_SetCompareCH1(TIM3, LL_TIM_OC_GetCompareCH1(TIM3) + 10);
    }
    if (input_flag == 'd') {
      LL_TIM_OC_SetCompareCH1(TIM3, LL_TIM_OC_GetCompareCH1(TIM3) - 10);
    }
    input_flag = 0;
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
  while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_2) {
  }
  LL_RCC_HSE_Enable();

  /* Wait till HSE is ready */
  while (LL_RCC_HSE_IsReady() != 1) {
  }
  LL_PWR_EnableBkUpAccess();
  if (LL_RCC_GetRTCClockSource() != LL_RCC_RTC_CLKSOURCE_LSE) {
    LL_RCC_ForceBackupDomainReset();
    LL_RCC_ReleaseBackupDomainReset();
  }
  LL_RCC_LSE_Enable();

  /* Wait till LSE is ready */
  while (LL_RCC_LSE_IsReady() != 1) {
  }
  if (LL_RCC_GetRTCClockSource() != LL_RCC_RTC_CLKSOURCE_LSE) {
    LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);
  }
  LL_RCC_EnableRTC();
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE_DIV_1, LL_RCC_PLL_MUL_9);
  LL_RCC_PLL_Enable();

  /* Wait till PLL is ready */
  while (LL_RCC_PLL_IsReady() != 1) {
  }
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

  /* Wait till System clock is ready */
  while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {
  }
  LL_SetSystemCoreClock(72000000);

  /* Update the time base */
  if (HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK) {
    Error_Handler();
  }
  LL_RCC_SetUSBClockSource(LL_RCC_USB_CLKSOURCE_PLL_DIV_1_5);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1) {
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
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
