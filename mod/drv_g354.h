#ifndef __DRV_G354_H
#define __DRV_G354_H

#include "stm32h7xx_hal.h"
#include <stdint.h>

/* 寄存器地址 (窗口0) */
#define G354_REG_BURST        0x00
#define G354_REG_MODE_CTRL    0x02
#define G354_REG_DIAG_STAT    0x04
#define G354_REG_FLAG         0x06
#define G354_REG_GPIO         0x08
#define G354_REG_COUNT        0x0A
#define G354_REG_TEMP_HIGH    0x0E
#define G354_REG_TEMP_LOW     0x10
#define G354_REG_XGYRO_HIGH   0x12
#define G354_REG_XGYRO_LOW    0x14
#define G354_REG_YGYRO_HIGH   0x16
#define G354_REG_YGYRO_LOW    0x18
#define G354_REG_ZGYRO_HIGH   0x1A
#define G354_REG_ZGYRO_LOW    0x1C
#define G354_REG_XACCL_HIGH   0x1E
#define G354_REG_XACCL_LOW    0x20
#define G354_REG_YACCL_HIGH   0x22
#define G354_REG_YACCL_LOW    0x24
#define G354_REG_ZACCL_HIGH   0x26
#define G354_REG_ZACCL_LOW    0x28

/* 寄存器地址 (窗口1) */
#define G354_REG_SIG_CTRL     0x00
#define G354_REG_MSC_CTRL     0x02
#define G354_REG_SMPL_CTRL    0x04
#define G354_REG_FILTER_CTRL  0x06
#define G354_REG_UART_CTRL    0x08
#define G354_REG_GLOB_CMD     0x0A
#define G354_REG_BURST_CTRL1  0x0C
#define G354_REG_BURST_CTRL2  0x0E
#define G354_REG_POL_CTRL     0x10
#define G354_REG_PROD_ID1     0x6A
#define G354_REG_PROD_ID2     0x6C
#define G354_REG_PROD_ID3     0x6E
#define G354_REG_PROD_ID4     0x70
#define G354_REG_VERSION      0x72
#define G354_REG_SERIAL_NUM1  0x74
#define G354_REG_SERIAL_NUM2  0x76
#define G354_REG_SERIAL_NUM3  0x78
#define G354_REG_SERIAL_NUM4  0x7A
#define G354_REG_WIN_CTRL     0x7E

/* 数据输出率设置 (参考手册) */
#define G354_DOUT_2000SPS  0x00
#define G354_DOUT_1000SPS  0x01
#define G354_DOUT_500SPS   0x02
#define G354_DOUT_250SPS   0x03
#define G354_DOUT_125SPS   0x04
#define G354_DOUT_62_5SPS  0x05
#define G354_DOUT_31_25SPS 0x06
#define G354_DOUT_15_625SPS 0x07
#define G354_DOUT_400SPS   0x08
#define G354_DOUT_200SPS   0x09
#define G354_DOUT_100SPS   0x0A
#define G354_DOUT_80SPS    0x0B
#define G354_DOUT_50SPS    0x0C
#define G354_DOUT_40SPS    0x0D
#define G354_DOUT_25SPS    0x0E
#define G354_DOUT_20SPS    0x0F

/* 滤波器设置 (参考手册) */
#define G354_FILTER_MA_TAP0   0x00
#define G354_FILTER_MA_TAP2   0x01
#define G354_FILTER_MA_TAP4   0x02
#define G354_FILTER_MA_TAP8   0x03
#define G354_FILTER_MA_TAP16  0x04
#define G354_FILTER_MA_TAP32  0x05
#define G354_FILTER_MA_TAP64  0x06
#define G354_FILTER_MA_TAP128 0x07
/* FIR Kaiser 滤波器定义略，可按需添加 */

/* 数据结构：传感器数据 (16位输出) */
typedef struct {
    int16_t temp;       // 温度 (16位)
    int16_t gyro[3];    // 陀螺仪 X,Y,Z (16位)
    int16_t accel[3];   // 加速度计 X,Y,Z (16位)
    uint16_t count;     // 采样计数
    uint16_t flag;      // ND/EA 标志
} G354_Data_t;

/* 句柄结构 */
typedef struct {
    SPI_HandleTypeDef *hspi;          // SPI 句柄 (必须指向已初始化的 SPI3)
    volatile uint8_t data_ready;       // 数据就绪标志 (由 DRDY 中断置位)
    G354_Data_t data;                  // 最新数据
} G354_Handle_t;

/* 初始化与配置 */
uint8_t G354_Init(G354_Handle_t *dev);
uint8_t G354_SelfTest(G354_Handle_t *dev);
uint8_t G354_SoftwareReset(G354_Handle_t *dev);
uint8_t G354_ConfigOutputRate(G354_Handle_t *dev, uint8_t rate);
uint8_t G354_ConfigFilter(G354_Handle_t *dev, uint8_t filter);
uint8_t G354_ConfigBurst(G354_Handle_t *dev, uint16_t ctrl1, uint16_t ctrl2);
uint8_t G354_StartSampling(G354_Handle_t *dev);
uint8_t G354_StopSampling(G354_Handle_t *dev);

/* 数据读取 (正常模式读取单个寄存器，不常用) */
uint16_t G354_ReadReg(uint8_t reg, uint8_t window);
void G354_WriteReg(uint8_t reg, uint8_t window, uint8_t data);

/* 突发模式读取 (推荐，一次读取所有数据) */
uint8_t G354_ReadBurst(G354_Handle_t *dev);

/* DRDY 中断处理 (需在 GPIO 中断中调用) */
void G354_DRDY_IRQHandler(G354_Handle_t *dev);

#endif
