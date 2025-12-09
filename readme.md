# 本项目基于UP主超子说物联网的Bootloader程序设计(上篇章)实现

## 1. 开发环境

-   **开发板**: 正点原子STM32F103MINI开发板
-   **微控制器**: STM32F103RCT6
-   **构建系统**: CUBEMX,CMake,ninja
-   **调试工具**: oepnocd,pyocd
-   **IDE**: VS Code + CMake Tools 扩展。

## 2. 工程模块介绍

### Core
里面放置了延时函数的实现。

### Drivers
包含了所有外设驱动、CMSIS 标准库以及HAL库。

#### BSP (Board Support Package)
-   **24CXX**: I2C EEPROM (如AT24C02) 驱动，用于存储配置。
-   **bootloader**: 启动加载程序相关代码，负责固件更新的引导。
-   **IIC**: 软件I2C通信驱动。
-   **NORFLASH**: 外部NOR Flash存储器驱动，用于存储新的固件。
-   **OTA_UART**: 用于OTA更新的UART通信驱动，负责接收新的固件数据。
-   **SPI**: SPI通信驱动。
-   **STMFLASH**: STM32内部Flash操作驱动，用于擦除、写入和读取内部Flash。

#### STM32F1xx_HAL_Driver
STMicroelectronics 提供的STM32F1系列硬件抽象层 (HAL) 库，简化了底层硬件操作。

## 3. 工程功能实现

本项目旨在实现一个通过串口 (UART) 进行固件更新的 OTA (Over-The-Air) 系统，并提供一个交互式的命令行界面进行管理。主要功能包括：

-   **固件远程更新**: 允许通过 UART 接口接收新的应用程序固件，并将其写入到指定的存储区域（内部 Flash 或外部 SPI Flash）。
-   **启动加载程序**: 在设备启动时，bootloader 会检查是否有新的固件需要更新（通过 EEPROM 标志位判断），并管理应用程序的启动。如果检测到 OTA 标志，将自动进行固件搬运；否则，直接跳转到应用程序。
-   **版本管理**: 支持查询和设置固件版本号。

### Bootloader 命令行功能 (通过串口输入指令)

在设备启动时，如果在指定时间内（例如5秒内）通过串口输入 `'w'` 字符，即可进入 Bootloader 命令行模式。在命令行模式下，可以通过输入数字指令来执行以下功能：

-   **`1`：擦除 A 区程序**: 擦除内部 Flash 中的应用程序区域。
-   **`2`：串口 IAP 下载 A 区程序**: 通过 Xmodem 协议从串口下载 `bin` 格式的固件到内部 Flash 的应用程序区域。
-   **`3`：设置 OTA 版本号**: 设置固件版本号，格式为 `VER-x.x.x-y/m/d-h:m`。
-   **`4`：查询 OTA 版本号**: 查询当前存储的固件版本号。
-   **`5`：向外部 Flash 下载程序**: 通过 Xmodem 协议从串口下载 `bin` 格式的固件到外部 SPI Flash。需要输入要使用的存储块编号 (1~9)。
-   **`6`：使用外部 Flash 内程序**: 从外部 SPI Flash 中选择一个存储块的固件，并将其恢复/升级到内部 Flash 的应用程序区域。需要输入要使用的存储块编号 (1~9)。

## 4. 烧录方法


1.  **烧录程序 (使用 OpenOCD)**:
    在完成项目编译后，可以通过以下命令将固件烧录到开发板中。请确保已正确安装 OpenOCD，并连接了 ST-Link 调试器,如果烧录器不同,需要自行修改Cmakelists。
    ```bash
    make flash
    ```
    或用Cmake Tools拓展在项目大纲生成flash目标
    此命令会执行主Cmakelists文件内的`openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "program build/OTA.elf verify reset" -c shutdown`，将 `build/OTA.elf` 文件烧录到目标芯片。
