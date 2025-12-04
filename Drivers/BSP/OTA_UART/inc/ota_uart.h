/**
 * @file ota_uart.h
 * @brief OTA串口驱动
 * @author gukumao (gukumao@gmail.com)
 * @version 1.0.1
 * @date 2025-11-21
 * 
 * 
*/
#ifndef OTA_UART_H
#define OTA_UART_H

#include "main.h"

// 串口缓冲区相关定义
#define OTA_RX_SIZE 2048
#define OTA_RX_MAX 256
#define NUM 10

// 外设底层定义
#define OTA_UART_CLK_ENABLE() __HAL_RCC_USART1_CLK_ENABLE()
#define OTA_UART_TX_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()
#define OTA_UART_RX_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()
#define OTA_UART_DMA_CLK_ENABLE() __HAL_RCC_DMA1_CLK_ENABLE()

#define OTA_UART_TX_PIN GPIO_PIN_9
#define OTA_UART_RX_PIN GPIO_PIN_10
#define OTA_UART_TX_PORT GPIOA
#define OTA_UART_RX_PORT GPIOA

#define OTA_UART USART1
#define OTA_UART_IRQn USART1_IRQn
#define OTA_UART_IRQHandler USART1_IRQHandler

#define OTA_UART_DMA DMA1_Channel5
#define OTA_UART_DMA_IRQn DMA1_Channel5_IRQn

/**
 * @brief 缓冲区管理块指针对
*/
typedef struct{
    uint8_t *start;
    uint8_t *end;
}UCB_URXBuffptr;

/**
 * @brief 缓冲区管理结构体
*/
typedef struct
{
    uint16_t URxcounter;
    UCB_URXBuffptr URxDataPtr[NUM];
    UCB_URXBuffptr *URxDataIN;
    UCB_URXBuffptr *URxDataOUT;
    UCB_URXBuffptr *URxDataEND;
}UCB_CB; 

extern UCB_CB ota_uart_cb;                      // 接收控制块（管理接收逻辑的核心结构体）
extern uint8_t ota_rxbuff[OTA_RX_SIZE];         // 物理接收缓冲区

// 外部接口函数
void ota_uart_init(uint32_t bandrate);
void ota_uart_cb_init(void);

#endif // !OTA_UART_H