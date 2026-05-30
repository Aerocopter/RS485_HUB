#ifndef __BSP_SPI_H
#define __BSP_SPI_H

#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_spi.h"

/* SPI3 句柄声明，外部可访问（如果需要） */
extern SPI_HandleTypeDef hspi3;

/* 初始化 SPI3，模式3，主模式，软件NSS，CS=PI4 */
void DRV_SPI3_Init(void);

/* 片选控制宏，CS = PI4 */
#define DRV_SPI3_CS_LOW()   HAL_GPIO_WritePin(GPIOI, GPIO_PIN_4, GPIO_PIN_RESET)
#define DRV_SPI3_CS_HIGH()  HAL_GPIO_WritePin(GPIOI, GPIO_PIN_4, GPIO_PIN_SET)

/* 单字节全双工传输（不控制片选） */
uint8_t DRV_SPI3_TransmitReceive(uint8_t txData);

/* 多字节全双工传输（自动控制片选） */
void DRV_SPI3_TransmitReceiveBuffer(uint8_t *txBuf, uint8_t *rxBuf, uint16_t len);

/* 便捷函数：读取16位寄存器（自动片选） */
uint16_t DRV_SPI3_ReadReg16(uint8_t regAddr);

/* 仅发送多字节（自动控制片选） */
void DRV_SPI3_SendBuffer(uint8_t *txBuf, uint16_t len);

/* 仅接收多字节（自动控制片选） */
void DRV_SPI3_ReceiveBuffer(uint8_t *rxBuf, uint16_t len);

/* 便捷函数：写入8位寄存器（自动片选） */
void DRV_SPI3_WriteReg8(uint8_t regAddr, uint8_t data);

#endif
