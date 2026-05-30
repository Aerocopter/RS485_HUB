# RS485 HUB 

A clean base project for **STM32H753IIK6** rs485 hub development.  
Generated with STM32CubeMX, built with **GCC ARM toolchain** and **HAL library**, and verified with a simple LED blinking on PC15.  

## Features
- **MCU**: STM32H753IIK6 (Cortex-M7 @ 400MHz, 2MB Flash, 1MB RAM)
- **Clock**: 16MHz HSE → 400MHz system clock
- **LED**: PC15 toggling every 500ms (configurable)
- **Toolchain**: `arm-none-eabi-gcc` + Makefile
- **Debugging**: SWD preserved (PA13/PA14 untouched) – debugger stays connected after programming
- **Modular structure**: Ready to integrate BSP, sensor drivers, RTOS and algorithm modules

## Hardware Requirements
- STM32H753IIK6 board (custom or evaluation board)
- LED connected to **PC10** (with appropriate resistor)
- ST-Link programmer/debugger

## Software Requirements
- **ARM GCC** toolchain (`arm-none-eabi-gcc`)
- **GNU Make**
- **STM32CubeProgrammer** or `st-flash` for flashing

## Quick Start

1. **Clone the repository**
   ```bash
   git clone https://github.com/Aerocopter/RS485_HUB.git
   cd RS485_HUB
