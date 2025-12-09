#include "ota_uart.h"
#include <stddef.h>
// --- 全局变量定义 ---
DMA_HandleTypeDef g_ota_uart_dma_handle; // DMA句柄
UART_HandleTypeDef g_ota_uart_handle;    // UART句柄
volatile UCB_CB ota_uart_cb;                      // 接收控制块（管理接收逻辑的核心结构体）
volatile uint8_t ota_rxbuff[OTA_RX_SIZE];         // 物理接收缓冲区 (假设 OTA_RX_SIZE 为 2048)

/**
 * @brief printf串口重定向
 * @param  fd 
 * @param  ptr
 * @param  len
 * @return int 
*/
int _write(int fd, char *ptr, int len)  
{  
  HAL_UART_Transmit(&g_ota_uart_handle, (uint8_t*)ptr, len, 0xFFFF);
  return len;
  
}

/**
 * @brief 串口硬件初始化
 * @param bandrate 波特率
 */
void ota_uart_init(uint32_t bandrate)
{
    // 1. 开启外设时钟
    OTA_UART_CLK_ENABLE();          // 使能串口时钟
    OTA_UART_RX_GPIO_CLK_ENABLE();  // 使能RX引脚GPIO时钟
    OTA_UART_TX_GPIO_CLK_ENABLE();  // 使能TX引脚GPIO时钟
    OTA_UART_DMA_CLK_ENABLE();      // 使能DMA时钟

    // 2. GPIO 初始化
    GPIO_InitTypeDef gpio_initstruct = {
        .Pin = OTA_UART_TX_PIN,
        .Mode = GPIO_MODE_AF_PP,       // 复用推挽输出
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_HIGH
    };
    HAL_GPIO_Init(OTA_UART_TX_PORT, &gpio_initstruct);

    gpio_initstruct.Pin = OTA_UART_RX_PIN;
    gpio_initstruct.Mode = GPIO_MODE_AF_INPUT; // 复用输入
    HAL_GPIO_Init(OTA_UART_RX_PORT, &gpio_initstruct);

    // 3. NVIC 中断控制器配置
    // 配置串口中断优先级并使能
    HAL_NVIC_SetPriority(OTA_UART_IRQn, 2,  0);
    HAL_NVIC_EnableIRQ(OTA_UART_IRQn);
    

    // 4. 串口参数初始化
    g_ota_uart_handle.Instance = OTA_UART;
    g_ota_uart_handle.Init.Mode = UART_MODE_TX_RX;
    g_ota_uart_handle.Init.StopBits = UART_STOPBITS_1;
    g_ota_uart_handle.Init.WordLength = UART_WORDLENGTH_8B;
    g_ota_uart_handle.Init.BaudRate = bandrate;
    g_ota_uart_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    g_ota_uart_handle.Init.Parity = UART_PARITY_NONE;
    HAL_UART_Init(&g_ota_uart_handle);
    // 【关键】使能串口空闲中断 (IDLE Interrupt)
    // 当串口收到数据并停止发送超过一个字节时间后触发，用于判断一帧结束
    __HAL_UART_ENABLE_IT(&g_ota_uart_handle, UART_IT_IDLE);

    // 5. DMA 初始化
    g_ota_uart_dma_handle.Instance = OTA_UART_DMA;
    g_ota_uart_dma_handle.Init.Direction = DMA_PERIPH_TO_MEMORY; // 方向：外设->内存
    g_ota_uart_dma_handle.Init.PeriphInc = DMA_PINC_DISABLE;     // 外设地址不增
    g_ota_uart_dma_handle.Init.MemInc = DMA_MINC_ENABLE;         // 内存地址自增
    g_ota_uart_dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    g_ota_uart_dma_handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    g_ota_uart_dma_handle.Init.Mode = DMA_NORMAL;                // 普通模式 (非循环，需手动重置)
    g_ota_uart_dma_handle.Init.Priority = DMA_PRIORITY_MEDIUM;
    HAL_DMA_Init(&g_ota_uart_dma_handle);
    
    // 关联 UART 句柄和 DMA 句柄
    __HAL_LINKDMA(&g_ota_uart_handle, hdmarx, g_ota_uart_dma_handle);
    
    // 6. 开启首次 DMA 接收
    HAL_UART_Receive_DMA(&g_ota_uart_handle,  (uint8_t *)ota_rxbuff, OTA_RX_MAX + 1);
}

/**
 * @brief 接收管理控制块初始化
 */
void ota_uart_cb_init(void)
{
    // 初始化指针队列的头尾指针
    ota_uart_cb.URxDataIN = (UCB_URXBuffptr *)&ota_uart_cb.URxDataPtr[0];    // 入队指针
    ota_uart_cb.URxDataOUT = (UCB_URXBuffptr *)&ota_uart_cb.URxDataPtr[0];   // 出队指针（代码中未展示出队逻辑）
    ota_uart_cb.URxDataEND = (UCB_URXBuffptr *)&ota_uart_cb.URxDataPtr[NUM - 1]; // 队列边界指针
    
        // 初始化数据存放的起始位置
    ota_uart_cb.URxDataIN->start = (uint8_t *)ota_rxbuff;    
    ota_uart_cb.URxcounter = 0; // 当前缓冲区的写偏移量
}

/**
 * @brief 串口中断服务函数
 * 核心逻辑：处理空闲中断，切分数据包
 */
void OTA_UART_IRQHandler(void)
{
    // 检查是否是空闲中断 (IDLE Flag)
    if (__HAL_UART_GET_FLAG(&g_ota_uart_handle, UART_FLAG_IDLE) != RESET) {
        
        // 1. 清除空闲中断标志位 (先读SR再读DR，HAL库宏封装了)
        __HAL_UART_CLEAR_IDLEFLAG(&g_ota_uart_handle);
        
        HAL_UART_DMAStop(&g_ota_uart_handle);

        // 2. 计算当前接收到的数据位置
        // 公式：当前偏移量 += (设定的DMA总长 - DMA剩余长度)
        ota_uart_cb.URxcounter += (OTA_RX_MAX + 1) - __HAL_DMA_GET_COUNTER(&g_ota_uart_dma_handle);
        
        // 3. 记录当前数据包的结束地址
        // end 指向的是本包数据的最后一个字节
        ota_uart_cb.URxDataIN->end = (uint8_t *)&ota_rxbuff[ota_uart_cb.URxcounter - 1];
        
        // 4. 指针队列入队：移动 IN 指针到下一个槽位
        ota_uart_cb.URxDataIN++;

        // 5. 队列回绕处理 (Ring Buffer Logic for Metadata)
        if (ota_uart_cb.URxDataIN == ota_uart_cb.URxDataEND) { 
            ota_uart_cb.URxDataIN = (UCB_URXBuffptr *)&ota_uart_cb.URxDataPtr[0]; 
        }

        // 6. 物理缓冲区空间管理
        // 判断剩余空间是否足够存放下一个最大包 (OTA_RX_MAX)M
        if (OTA_RX_SIZE - ota_uart_cb.URxcounter >= OTA_RX_MAX) {
            // 空间足够：下一个包接着当前位置存
            ota_uart_cb.URxDataIN->start = (uint8_t *)&ota_rxbuff[ota_uart_cb.URxcounter];
        }
        else {
            // 空间不足：从头开始存 (Rewind)
            ota_uart_cb.URxDataIN->start = (uint8_t *)ota_rxbuff;
            ota_uart_cb.URxcounter = 0;
        }


        // 7. 重新开启 DMA 接收
        HAL_UART_Receive_DMA(&g_ota_uart_handle, (uint8_t *)ota_uart_cb.URxDataIN->start,  OTA_RX_MAX + 1); 
    }
}