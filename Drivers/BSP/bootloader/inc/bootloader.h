#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include "main.h"

typedef void (*load)(void); // 跳转APP区函数指针

void bootloader_brance(void);
void bootloader_event(uint8_t *data, uint16_t datalen);
uint16_t xmodem_crc16(uint8_t *pdata, uint32_t len);
#endif

