/**
 * @file    bootloader.c
 * @brief   STM32 UART IAP (In-Application Programming) Bootloader 实现文件
 * @details 该文件实现了基于串口的 Bootloader 功能，主要包括：
 *          1. 通过 Xmodem 协议下载固件到内部 Flash (APP区)。
 *          2. 支持将固件下载到外部 SPI Flash (W25Q64) 进行备份。
 *          3. 支持从外部 Flash 恢复或升级固件到内部 Flash。
 *          4. 管理 OTA 版本号及升级标志位 (保存在 EEPROM AT24Cxx 中)。
 *          5. 跳转至用户应用程序 (APP)。
 * 
 */

#include "bootloader.h"
#include "ota_uart.h"
#include "delay.h"
#include "myiic.h"
#include "24cxx.h"
#include "stmflash.h"
#include "norflash.h"
#include "main.h"

/** 
 * @brief 定义跳转函数指针类型 
 */
load load_a; 

/** 
 * @brief 外部引用的 UART 句柄，用于 OTA 通信 
 */
extern UART_HandleTypeDef g_ota_uart_handle;

/* 内部函数声明 */
static void bootloader_info(void);

/**
 * @brief  Bootloader 串口数据处理状态机
 * @details 该函数是 Bootloader 的核心处理逻辑，通常在串口接收中断或轮询中调用。
 *          它根据 `boot_state_flag` 的状态处理不同的任务：
 *          - 空闲状态：解析菜单命令 ('1'~'7')。
 *          - IAP_XMODEMD_FLAG：处理 Xmodem 数据包接收与 Flash 写入。
 *          - SET_VERSION_FLAG：解析并保存版本号字符串。
 *          - W25Q64_DL_FLAG：选择下载到外部 Flash 的块编号。
 *          - W25Q64_LOAD_FLAG：选择从外部 Flash 加载的块编号。
 * 
 * @param  data    指向接收到的数据缓冲区的指针
 * @param  datalen 接收到的数据长度（字节）
 * @return None
 */
void bootloader_event(uint8_t *data, uint16_t datalen)
{
    int temp;

    // --- 状态：空闲模式 (等待菜单指令) ---
    if (boot_state_flag == 0)
    {
        if (datalen == 1){
            switch (data[0]) {
                // [1] 选择擦除 A 区程序
                case '1' : {
                    printf("擦除A区\r\n");
                    // 注意：这里硬编码了擦除100页(200KB)，建议根据实际芯片容量调整
                    stmflash_erase(F103RC_A_SADDR, 100); 
                    break;
                }
                // [2] 串口 IAP 下载 (Xmodem)
                case '2' : {
                    printf("通过Xmodem协议:串口IAP下载程序,请使用bin格式文件\r\n");
                    // 设置 Xmodem 控制与数据标志位
                    boot_state_flag |= (IAP_XMODEMC_FLAG | IAP_XMODEMD_FLAG);
                    updataA.xmodemTimer = 0;
                    updataA.xmodemNB = 0;
                    break;
                }
                // [3] 设置版本号
                case '3' : {
                    printf("设置版本号\r\n");
                    boot_state_flag |= SET_VERSION_FLAG;
                    break;
                }
                // [4] 查询版本号
                case '4' : {
                    at24cxx_read_otaflag();
                    printf("当前版本号:%s\r\n", OTA_Info.ota_ver);
                    bootloader_info();
                    break;
                }
                // [5] 向外部 Flash 下载程序
                case '5' : {
                    printf("向外部flash下载程序,输入要使用的编号(1~9)\r\n");
                    boot_state_flag |= W25Q64_DL_FLAG;
                    break;
                }
                // [6] 使用外部 Flash 程序恢复/升级
                case '6' : {
                    printf("使用外部flash内的程序,输入要使用的编号(1~9)\r\n");
                    boot_state_flag |= W25Q64_LOAD_FLAG;
                    break;
                }
                // [7] 重启系统
                case '7' : {
                    printf("重启.....");
                    delay_ms(10);
                    NVIC_SystemReset();
                    break;
                }
                default : break;
            }
        }
    }
    // --- 状态：Xmodem 数据传输模式 ---
    else if (boot_state_flag & IAP_XMODEMD_FLAG) {
        // 处理 SOH 数据包 (133字节: 头1 + 包号1 + 反码1 + 数据128 + CRC2)
        if ((datalen == 133) && (data[0] == 0x01)) {
            boot_state_flag &= ~(IAP_XMODEMC_FLAG); // 收到数据，停止发送 'C' 请求
            
            // 计算 CRC 校验
            updataA.xmodemcrc = xmodem_crc16(&data[3], 128);
            
            // 校验通过 (高位在先)
            if (updataA.xmodemcrc == (data[131] * 256 + data[132])) {
                updataA.xmodemNB++; // 包计数增加
                
                // 将数据存入临时 Buffer，计算在 Page 中的偏移
                // 注意：F103RC_PAGE_SIZE 通常为 2048 字节
                memcpy(&updataA.updatabuff[(updataA.xmodemNB - 1) % (F103RC_PAGE_SIZE / 128) * 128], &data[3], 128);
                
                // 凑满一页数据，执行一次写入
                if(updataA.xmodemNB % (F103RC_PAGE_SIZE / 128) == 0) {
                    if (boot_state_flag & W25Q64_XMODEM_FLAG) {
                        // 写入外部 Flash (W25Q64)
                        norflash_write(updataA.updatabuff, 
                            (updataA.w25q64_block_num * 64 * 1024) + ((updataA.xmodemNB / (F103RC_PAGE_SIZE / 128) - 1)  * F103RC_PAGE_SIZE), 
                            F103RC_PAGE_SIZE);
                    }
                    else {
                        // 写入内部 Flash
                        stmflash_write(F103RC_A_SADDR + (updataA.xmodemNB / (F103RC_PAGE_SIZE / 128) - 1) * F103RC_PAGE_SIZE, 
                            (uint16_t *)updataA.updatabuff, 
                            F103RC_PAGE_SIZE / 2);
                    }
                }
                printf("\x06\r\n"); // 发送 ACK
            }
            else {
                printf("\x15\r\n"); // 校验失败，发送 NAK
            }
        }

        // 处理 EOT 结束信号 (0x04)
        if ((datalen == 1) && (data[0] == 0x04)) {
            printf("\x06\r\n"); // 发送 ACK
            
            // 处理不足一页的剩余数据
            if(updataA.xmodemNB % (F103RC_PAGE_SIZE / 128) != 0) {
                if (boot_state_flag & W25Q64_XMODEM_FLAG)
                {
                    norflash_write(updataA.updatabuff, 
                        (updataA.w25q64_block_num * 64 * 1024) + (updataA.xmodemNB / (F103RC_PAGE_SIZE / 128)  * F103RC_PAGE_SIZE), 
                        F103RC_PAGE_SIZE);
                }
                else
                {
                    stmflash_write(F103RC_A_SADDR + (updataA.xmodemNB / (F103RC_PAGE_SIZE / 128)) * F103RC_PAGE_SIZE, 
                        (uint16_t *)updataA.updatabuff, 
                        updataA.xmodemNB % (F103RC_PAGE_SIZE / 128) * 128 / 2);
                }
            }
            
            // 传输结束，清除标志位并执行后续操作
            boot_state_flag &= ~(IAP_XMODEMD_FLAG);
            
            if (boot_state_flag & W25Q64_XMODEM_FLAG) {
                // 如果是下载到外部 Flash，记录长度信息到 EEPROM
                boot_state_flag &= ~(W25Q64_XMODEM_FLAG);
                OTA_Info.firlen[updataA.w25q64_block_num] = updataA.xmodemNB * 128;
                at24cxx_write_otainfo();
                delay_ms(100);
                bootloader_info();
            }
            else {
                // 如果是直接更新 APP，完成后重启
                delay_ms(10);
                NVIC_SystemReset();
            }
        }
    }
    // --- 状态：设置版本号 ---
    else if (boot_state_flag & SET_VERSION_FLAG) {
        if (datalen == 26) {
            // 解析版本字符串格式: VER-x.x.x-y/m/d-h:m
            if (sscanf((char *)data, "VER-%d.%d.%d-%d/%d/%d-%d:%d", &temp, &temp, &temp, &temp, &temp, &temp, &temp, &temp ) == 8) {
                memset(OTA_Info.ota_ver, 0, 32);
                memcpy(OTA_Info.ota_ver, data, 26);
                at24cxx_write_otainfo();
                printf("版本号设置成功:%s\r\n", OTA_Info.ota_ver);
                boot_state_flag &= ~(SET_VERSION_FLAG);
                bootloader_info();
            }
            else {
                printf("版本号格式错误,请重新设置\r\n");
            }
        }
        else {
            printf("版本号长度错误,请重新设置\r\n");
        }
    }
    // --- 状态：准备下载到外部 Flash (选择块) ---
    else if (boot_state_flag & W25Q64_DL_FLAG) {
        if (datalen == 1) {
            if (data[0] >= '1' && data[0] <= '9') {
                updataA.w25q64_block_num = data[0] - '0';
                // 状态转移：进入 Xmodem 接收 + 外部 Flash 写模式
                boot_state_flag |= (IAP_XMODEMC_FLAG | IAP_XMODEMD_FLAG | W25Q64_XMODEM_FLAG);
                updataA.xmodemTimer = 0;
                updataA.xmodemNB = 0;
                OTA_Info.firlen[updataA.w25q64_block_num] = 0;
                printf("通过Xmodem协议:向外部flash第%d块下载程序,请使用bin格式文件\r\n", updataA.w25q64_block_num);
                boot_state_flag &= ~(W25Q64_DL_FLAG);
            }
            else {
                printf("编号错误\r\n");
            }
        }
        else {
            printf("数据长度错误\r\n");
        }
    }
    // --- 状态：准备从外部 Flash 加载 (选择块) ---
    else if (boot_state_flag & W25Q64_LOAD_FLAG) {
        if (datalen == 1) {
            if (data[0] >= '1' && data[0] <= '9') {
                updataA.w25q64_block_num = data[0] - '0';
                // 状态转移：设置更新标志位，主循环将检测此标志进行搬运
                boot_state_flag |= UPDATA_A_FLAG;
                boot_state_flag &= ~(W25Q64_LOAD_FLAG);
            }
            else {
                printf("编号错误\r\n");
            }
        }
        else {
            printf("数据长度错误\r\n");
        }
    }
}


/**
 * @brief  打印 Bootloader 命令行菜单
 * @note   通过串口输出支持的指令列表
 * @return None
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
 * @brief  Bootloader 进入检测
 * @details 在系统启动时等待指定时间，检测用户是否输入 'w' 字符以进入 Bootloader 命令行模式。
 * 
 * @param  timeout 等待超时时间 (单位：秒)
 * @retval 1 检测到 'w'，进入 Bootloader
 * @retval 0 超时未检测到，继续执行后续逻辑 (如跳转 APP)
 */
uint8_t bootloader_enter(uint8_t timeout)
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
 * @brief  跳转到用户 APP 应用程序
 * @details 检查指定地址的栈顶指针是否合法，如果合法则复位 MSP 并跳转。
 * 
 * @param  addr APP 程序的起始地址 (如 F103RC_A_SADDR)
 * @return None (如果跳转成功，不会返回)
 * @note   建议在跳转前关闭全局中断 (__disable_irq()) 以防止 HardFault。
 */
static void load_app(uint32_t addr)
{
    // 判断程序起始地址是否有合法的堆栈指针地址 (检查是否在 RAM 范围内：0x20000000)
    // 0X2FFE0000 掩码适用于 64KB~128KB RAM 的 F103RC/ZE 等型号
    if (((*(__IO uint32_t *)addr) & 0X2FFE0000) == 0x20000000) {
        load_a = (load)(*(__IO uint32_t *)(addr + 4)); // 获取 APP 区复位中断向量地址
         __set_MSP(*(__IO uint32_t *)addr);            // 初始化堆栈指针 (MSP)
        load_a();                                      // 跳转至 APP
    }
    else {
        printf("跳转A分区失败\r\n");
    }
}

/**
 * @brief  Bootloader 主分支逻辑
 * @details 该函数在 main 函数中调用。流程如下：
 *          1. 调用 bootloader_enter 等待用户按键。
 *          2. 如果未按键且检测到 OTA 标志位，准备自动更新。
 *          3. 如果未按键且无 OTA 任务，直接跳转 APP。
 *          4. 如果按键进入，显示菜单。
 * 
 * @note   如果设置了 UPDATA_A_FLAG，具体的固件搬运逻辑需要在主循环或其他地方执行。
 * @return None
 */
void bootloader_brance(void)
{
    // 如果用户未输入w (超时返回 0)
    if (bootloader_enter(5) == 0) {
        // 检查 EEPROM 中的 OTA 标志位
        if (OTA_Info.ota_flag == OTA_SET_FLAG) {
            printf("OTA升级中...");
            printf("\r\n");
            // 设置标志位，通知主循环进行固件搬运 (从 W25Q64 到 内部Flash)
            boot_state_flag |= UPDATA_A_FLAG;
            updataA.w25q64_block_num = 0;
        }
        // 否则直接跳转 APP 区
        else {
            printf("跳转APP程序...\r\n");
            load_app(F103RC_A_SADDR);
        }
    }

    printf("进入BootLoader命令行\r\n");
    bootloader_info();
}

/**
 * @brief  计算 XMODEM 协议的 CRC16 校验值
 * @details 使用标准 CRC-16-CCITT 多项式 (0x1021)。
 * 
 * @param  pdata 指向要计算的数据缓冲区的指针
 * @param  len   数据长度
 * @return uint16_t 计算出的 CRC16 值
 */
uint16_t xmodem_crc16(uint8_t *pdata, uint32_t len)
{
    uint8_t i;
    uint16_t crcinit = 0x0000;
    uint16_t crcpoly = 0x1021;

    while (len--) {
        crcinit  = crcinit ^ (*pdata++ << 8);
        for (i = 0; i < 8; i++) {
            if (crcinit & 0x8000) {
                crcinit = (crcinit << 1) ^ crcpoly;
            }
            else {
                crcinit <<= 1;
            }
        }
    }
    return crcinit;
}