#include "drv_g354.h"
#include "bsp_spi.h"   // 包含 SPI 驱动函数

/* 私有函数声明 */
static uint8_t G354_SelectWindow(uint8_t window);
static uint16_t G354_ReadRegWithWindow(uint8_t reg, uint8_t window);
static void G354_WriteRegWithWindow(uint8_t reg, uint8_t window, uint8_t data);

/* 等待超时辅助函数 (毫秒) */
static uint8_t WaitForFlagClear(volatile uint8_t *flag, uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    while (*flag) {
        if (HAL_GetTick() - start > timeout_ms) return 0;
    }
    return 1;
}

/* 选择窗口 (直接使用底层 SPI 写) */
static uint8_t G354_SelectWindow(uint8_t window)
{
    // WIN_CTRL 地址 0x7E，在任何窗口下均可访问
    DRV_SPI3_WriteReg8(G354_REG_WIN_CTRL, window);
    return 1;
}

/* 带窗口的读16位寄存器 */
static uint16_t G354_ReadRegWithWindow(uint8_t reg, uint8_t window)
{
    G354_SelectWindow(window);
    return DRV_SPI3_ReadReg16(reg);
}

/* 带窗口的写8位寄存器 */
static void G354_WriteRegWithWindow(uint8_t reg, uint8_t window, uint8_t data)
{
    G354_SelectWindow(window);
    DRV_SPI3_WriteReg8(reg, data);
}

/* 初始化 */
uint8_t G354_Init(G354_Handle_t *dev)
{
    uint16_t prod_id[4];
    uint32_t timeout = HAL_GetTick() + 1000;

    if (!dev || !dev->hspi) return 0;

    /* 等待启动完成 (NOT_READY 位清零) */
    while (HAL_GetTick() < timeout) {
        uint16_t glob = G354_ReadRegWithWindow(G354_REG_GLOB_CMD, 1);
        if (!(glob & 0x0400)) break;  // bit10 = NOT_READY
        HAL_Delay(10);
    }
    if (HAL_GetTick() >= timeout) return 0;

    /* 检查硬件错误 */
    uint16_t diag = G354_ReadRegWithWindow(G354_REG_DIAG_STAT, 0);
    if ((diag & 0x0060) != 0) return 0;  // HARD_ERR 非零

    /* 读取产品 ID 验证通信 */
    prod_id[0] = G354_ReadRegWithWindow(G354_REG_PROD_ID1, 1);
    prod_id[1] = G354_ReadRegWithWindow(G354_REG_PROD_ID2, 1);
    prod_id[2] = G354_ReadRegWithWindow(G354_REG_PROD_ID3, 1);
    prod_id[3] = G354_ReadRegWithWindow(G354_REG_PROD_ID4, 1);

    /* 验证是否为 "G354PDH0" (ASCII: 0x3347, 0x3435, 0x4450, 0x3048) */
    if (prod_id[0] != 0x3347 || prod_id[1] != 0x3435 ||
        prod_id[2] != 0x4450 || prod_id[3] != 0x3048) {
        return 0;
    }

    /* 默认配置：125SPS，移动平均 TAP=32，突发模式输出全部 16bit */
    G354_ConfigOutputRate(dev, G354_DOUT_125SPS);
    G354_ConfigFilter(dev, G354_FILTER_MA_TAP32);
    G354_ConfigBurst(dev, 0xF006, 0x0000);  // 输出 FLAG,TEMP,GYRO,ACCL,GPIO,COUNT 16bit

    dev->data_ready = 0;
    return 1;
}

/* 配置数据输出率 */
uint8_t G354_ConfigOutputRate(G354_Handle_t *dev, uint8_t rate)
{
    (void)dev;
    G354_WriteRegWithWindow(G354_REG_SMPL_CTRL + 1, 1, rate);  // SMPL_CTRL 高字节
    return 1;
}

/* 配置滤波器 */
uint8_t G354_ConfigFilter(G354_Handle_t *dev, uint8_t filter)
{
    (void)dev;
    G354_WriteRegWithWindow(G354_REG_FILTER_CTRL, 1, filter);  // 低字节

    /* 等待滤波器设置完成 (FILTER_STAT 位清零) */
    uint32_t timeout = HAL_GetTick() + 100;
    while (HAL_GetTick() < timeout) {
        uint16_t ctrl = G354_ReadRegWithWindow(G354_REG_FILTER_CTRL, 1);
        if (!(ctrl & 0x0020)) break;  // FILTER_STAT 位 (bit5)
        HAL_Delay(1);
    }
    return (HAL_GetTick() < timeout) ? 1 : 0;
}

/* 配置突发模式输出内容 */
uint8_t G354_ConfigBurst(G354_Handle_t *dev, uint16_t ctrl1, uint16_t ctrl2)
{
    (void)dev;
    G354_WriteRegWithWindow(G354_REG_BURST_CTRL1 + 1, 1, ctrl1 >> 8);
    G354_WriteRegWithWindow(G354_REG_BURST_CTRL1, 1, ctrl1 & 0xFF);
    G354_WriteRegWithWindow(G354_REG_BURST_CTRL2 + 1, 1, ctrl2 >> 8);
    G354_WriteRegWithWindow(G354_REG_BURST_CTRL2, 1, ctrl2 & 0xFF);
    return 1;
}

/* 进入采样模式 */
uint8_t G354_StartSampling(G354_Handle_t *dev)
{
    (void)dev;
    G354_WriteRegWithWindow(G354_REG_MODE_CTRL + 1, 0, 0x01);  // MODE_CMD = 01
    return 1;
}

/* 退出采样模式 */
uint8_t G354_StopSampling(G354_Handle_t *dev)
{
    (void)dev;
    G354_WriteRegWithWindow(G354_REG_MODE_CTRL + 1, 0, 0x02);  // MODE_CMD = 10
    return 1;
}

/* 突发模式读取数据 (按 16bit 输出配置) */
uint8_t G354_ReadBurst(G354_Handle_t *dev)
{
    uint8_t rx[20];  // 足够容纳 16bit 输出：FLAG(2)+TEMP(2)+GYRO3(6)+ACCL3(6)+GPIO(2)+COUNT(2) = 20字节
    uint8_t *p = rx;

    /* 发送 BURST 命令 (0x80 0x00) 并连续读取数据 */
    DRV_SPI3_CS_LOW();   // 手动拉低片选
    DRV_SPI3_TransmitReceive(0x80);   // 高字节
    DRV_SPI3_TransmitReceive(0x00);   // 低字节

    /* 连续读取 20 字节 (根据 BURST_CTRL 配置，这里假设 10 个字) */
    for (int i = 0; i < 20; i++) {
        rx[i] = DRV_SPI3_TransmitReceive(0x00);
    }
    DRV_SPI3_CS_HIGH();  // 拉高片选

    /* 解析数据 (大端) */
    p = rx;
    dev->data.flag   = (p[0] << 8) | p[1]; p += 2;
    dev->data.temp   = (p[0] << 8) | p[1]; p += 2;
    dev->data.gyro[0] = (p[0] << 8) | p[1]; p += 2;
    dev->data.gyro[1] = (p[0] << 8) | p[1]; p += 2;
    dev->data.gyro[2] = (p[0] << 8) | p[1]; p += 2;
    dev->data.accel[0] = (p[0] << 8) | p[1]; p += 2;
    dev->data.accel[1] = (p[0] << 8) | p[1]; p += 2;
    dev->data.accel[2] = (p[0] << 8) | p[1]; p += 2;
    /* GPIO 和 COUNT 可根据需要解析，这里省略，可根据实际需求添加 */
    // dev->gpio = (p[0] << 8) | p[1]; p += 2;
    // dev->count = (p[0] << 8) | p[1]; p += 2;

    dev->data_ready = 0;
    return 1;
}

/* 软件自检 */
uint8_t G354_SelfTest(G354_Handle_t *dev)
{
    (void)dev;
    G354_WriteRegWithWindow(G354_REG_MSC_CTRL + 1, 1, 0x04);  // SELF_TEST 位 (bit10)

    /* 等待自检完成 (SELF_TEST 位清零) */
    uint32_t timeout = HAL_GetTick() + 1000;
    while (HAL_GetTick() < timeout) {
        uint16_t ctrl = G354_ReadRegWithWindow(G354_REG_MSC_CTRL, 1);
        if (!(ctrl & 0x0400)) break;  // SELF_TEST 位
        HAL_Delay(10);
    }
    if (HAL_GetTick() >= timeout) return 0;

    /* 检查结果 */
    uint16_t diag = G354_ReadRegWithWindow(G354_REG_DIAG_STAT, 0);
    return ((diag & 0x7800) == 0);  // ST_ERR 位全零
}

/* 软件复位 */
uint8_t G354_SoftwareReset(G354_Handle_t *dev)
{
    (void)dev;
    G354_WriteRegWithWindow(G354_REG_GLOB_CMD + 1, 1, 0x80);  // SOFT_RST 位 (bit7)
    HAL_Delay(800);  // 等待复位完成
    return 1;
}

/* 通用读寄存器 (外部调用) */
uint16_t G354_ReadReg(uint8_t reg, uint8_t window)
{
    return G354_ReadRegWithWindow(reg, window);
}

/* 通用写寄存器 (外部调用) */
void G354_WriteReg(uint8_t reg, uint8_t window, uint8_t data)
{
    G354_WriteRegWithWindow(reg, window, data);
}

/* DRDY 中断处理函数 (需在 GPIO 中断中调用) */
void G354_DRDY_IRQHandler(G354_Handle_t *dev)
{
    if (dev) {
        dev->data_ready = 1;
    }
}
