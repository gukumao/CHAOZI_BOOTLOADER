/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
#include "stdio.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
#define F103RC_FALSH_SADDR 0X08000000 // flash起始地址
#define F103RC_PAGE_SIZE 2048 // 页大小
#define F103RC_PAGE_NUM 128 // 页个数
#define F103RC_B_PAGE_NUM 10  // B区页个数
#define F103RC_A_PAGE_NUM (F103RC_PAGE_NUM - F103RC_B_PAGE_NUM) // A区页个数
#define F103RC_A_SPAGE F103RC_B_PAGE_NUM  // A区起始页标号
#define F103RC_A_SADDR (F103RC_FALSH_SADDR + F103RC_A_SPAGE * F103RC_PAGE_SIZE) // A区flash起始地址

#define UPDATA_A_FLAG 0X00000001 // 更新标志位
#define IAP_XMODEMC_FLAG 0X00000002 // IAP XMODEM标志位
#define IAP_XMODEMD_FLAG 0X00000004 // IAP XMODEM标志位
#define SET_VERSION_FLAG 0X00000008 // 设置版本标志位
#define W25Q64_DL_FLAG 0X00000010 // 外部flash下载标志位
#define W25Q64_XMODEM_FLAG 0X00000020 // 外部flashXMODEM标志位
#define W25Q64_LOAD_FLAG 0X00000040 // 外部flashXMODEM标志位

#define OTA_SET_FLAG 0XAABB1122 // OTA校验码

/**
 * @brief OTA信息结构体
*/
typedef struct 
{
    uint32_t ota_flag; // OTA标志位
    uint32_t firlen[11]; // OTA字节数
    uint8_t ota_ver[32];
}OTA_InfoCB;

/**
 * @brief 外部flash管理结构体
*/
typedef struct 
{
    uint8_t updatabuff[F103RC_PAGE_SIZE]; // 内部flash缓冲区
    uint32_t w25q64_block_num; // 外部flash块索引
    uint32_t xmodemTimer;
    uint32_t xmodemNB;
    uint32_t xmodemcrc;
}updata_cb;

#define OTA_INFOCB_SIZE sizeof(OTA_InfoCB)

extern OTA_InfoCB OTA_Info;
extern updata_cb updataA;
extern uint32_t boot_state_flag;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
