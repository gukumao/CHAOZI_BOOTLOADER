#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include "main.h"

typedef void (*load)(void); // 跳转APP区函数指针

void bootloader_brance(void);

#endif

