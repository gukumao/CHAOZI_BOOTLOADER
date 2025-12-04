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
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "ota_uart.h"
#include "delay.h"
#include "myiic.h"
#include "norflash.h"
#include "24cxx.h"
#include "stmflash.h"
#include "bootloader.h"
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
OTA_InfoCB OTA_Info;
updata_cb updataA;
uint32_t boot_state_flag;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  uint32_t i = 0;
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
  /* USER CODE BEGIN 2 */
  delay_init(72);
  ota_uart_init(921600);
  ota_uart_cb_init();
  iic_init();
  norflash_init();
  at24cxx_read_otaflag();
  bootloader_brance();
  /* USER CODE END 2 */
  
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // if ((boot_state_flag & UPDATA_A_FLAG) != 0) {
    //   printf("长度%d字节\r\n", OTA_Info.firlen[updataA.w25q64_block_num]);
    //   if (OTA_Info.firlen[updataA.w25q64_block_num] % 4 == 0) {
    //     for (i = 0; i < OTA_Info.firlen[updataA.w25q64_block_num] / F103RC_PAGE_SIZE; i++) {
    //       norflash_read(updataA.updatabuff, i * 2048 + updataA.w25q64_block_num * 64 * 1024, F103RC_PAGE_SIZE);
    //       stmflash_write(F103RC_A_SADDR + i * F103RC_PAGE_SIZE, (uint16_t *)updataA.updatabuff, F103RC_PAGE_SIZE);
    //     }

    //     if (OTA_Info.firlen[updataA.w25q64_block_num] % 1024 != 0) {
    //       norflash_read(updataA.updatabuff, i * 2048 + updataA.w25q64_block_num * 64 * 1024, OTA_Info.firlen[updataA.w25q64_block_num] % 1024);
    //       stmflash_write(F103RC_A_SADDR + i * F103RC_PAGE_SIZE, (uint16_t *)updataA.updatabuff, OTA_Info.firlen[updataA.w25q64_block_num] % 1024);
    //     }

    //     if (updataA.w25q64_block_num == 0) {
    //       OTA_Info.ota_flag = 0;
    //       at24cxx_write_otainfo();
    //     }
    //     NVIC_SystemReset();
    //   }
    //   else {
    //   printf("长度错误\r\n");
    //   boot_state_flag &= ~(UPDATA_A_FLAG);      
    //   } 
    // }
    
    // if (ota_uart_cb.URxDataOUT != ota_uart_cb.URxDataIN) {
    //   printf("本次接收的字节数为%d\r\n", ota_uart_cb.URxDataOUT->end - ota_uart_cb.URxDataOUT->start + 1);
    //   for (int i = 0; i < (ota_uart_cb.URxDataOUT->end - ota_uart_cb.URxDataOUT->start + 1); i++) {
    //     printf("%c", ota_uart_cb.URxDataOUT->start[i]);
    //   }
    //   printf("\r\n");
    //   ota_uart_cb.URxDataOUT++;
    //   if (ota_uart_cb.URxDataOUT == ota_uart_cb.URxDataEND) {
    //     ota_uart_cb.URxDataOUT = &ota_uart_cb.URxDataPtr[0];
    //   }
    // }

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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
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
}

/* USER CODE BEGIN 4 */

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
