#include "bootloader.h"
#include "ota_uart.h"
#include "delay.h"
#include "myiic.h"
#include "24cxx.h"
#include "stmflash.h"
#include "main.h"
load load_a; // APP函数指针
extern UART_HandleTypeDef g_ota_uart_handle;

/**
 * @brief 打印命令行指令
*/
static void bootloader_info(void)
{
    printf("\r\n");
    printf("[1]擦除A区\r\n");
    printf("[2]串口IAP下载A区程序\r\n");
    printf("[3]设置OTA版本号\r\n");
    printf("[4]查询OTA版本号\r\n");
    printf("[5]向外部flash下载程序\r\n");
    printf("[6]使用外部flash内程序\r\n");
    printf("[7]重启\r\n");
}

/**
 * @brief 命令行进入程序
 * @param  timeout 开机等待秒数
 * @return uint8_t 
*/
static uint8_t bootloader_enter(uint8_t timeout)
{
    printf("请在%d秒内输入w进入bootloder命令行\r\n", timeout);
    // 等待用户输入w
    while (timeout--) {
        delay_ms(1000);
        if (ota_rxbuff[0] == 'w') {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief APP区跳转
 * @param  addr APP程序起始地址
*/
static void load_app(uint32_t addr)
{
    // 判断程序起始地址是否有堆栈指针地址
    if (((*(__IO uint32_t *)addr) & 0X2FFE0000) == 0x20000000) {
        load_a = (load)(*(__IO uint32_t *)(addr + 4)); // 获取APP区中断向量起始地址
         __set_MSP(*(__IO uint32_t *)addr); // 初始化堆栈指针
        load_a(); // 跳转APP
    }
}

/**
 * @brief OTA业务逻辑
*/
void bootloader_brance(void)
{
    // 如果用户未输入w
    if (bootloader_enter(5) == 0) {
        // 如果有OTA标志位
        if (OTA_Info.ota_flag == OTA_SET_FLAG) {
            printf("OTA升级中...");
            printf("\r\n");
            boot_state_flag |= UPDATA_A_FLAG;
            updataA.w25q64_block_num = 0;
        }
        // 否则直接跳转APP区
        else {
            printf("跳转APP程序...\r\n");
            load_app(F103RC_A_SADDR);
        }
    }
    // 否则进入命令行
    else {
        printf("进入BootLoader命令行\r\n");
        bootloader_info();
    }
}

