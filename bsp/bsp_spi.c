#include "bsp_spi.h"

/* SPI3 句柄定义 */
SPI_HandleTypeDef hspi3;

/* SPI3 初始化 */
void DRV_SPI3_Init(void)
{
    /* 1. 使能 SPI3 和 GPIO 时钟 */
    __HAL_RCC_SPI3_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();   // SCK, MISO, MOSI 在 GPIOC
    __HAL_RCC_GPIOI_CLK_ENABLE();   // CS 在 GPIOI

    /* 2. 配置 SPI 引脚复用 */
    GPIO_InitTypeDef gpio = {0};

    // SCK  PC10, MISO PC11, MOSI PC12 (AF6)
    gpio.Pin = GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOC, &gpio);

    // CS 引脚 PI4 (作为普通 GPIO 输出)
    gpio.Pin = GPIO_PIN_4;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOI, &gpio);
    DRV_SPI3_CS_HIGH();  // 片选默认拉高

    /* 3. 配置 SPI 外设 */
    hspi3.Instance = SPI3;
    hspi3.Init.Mode = SPI_MODE_MASTER;
    hspi3.Init.Direction = SPI_DIRECTION_2LINES;
    hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi3.Init.CLKPolarity = SPI_POLARITY_HIGH;      // CPOL = 1
    hspi3.Init.CLKPhase = SPI_PHASE_2EDGE;           // CPHA = 1 (模式3)
    hspi3.Init.NSS = SPI_NSS_SOFT;                    // 软件 NSS
    hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi3.Init.CRCPolynomial = 7;
    hspi3.Init.CRCLength = SPI_CRC_LENGTH_8BIT;
    hspi3.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;

    /* 设置波特率分频，使 SPI 时钟 ≤ 1MHz（根据实际 APB1 时钟调整） */
    uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();   // APB1 频率
    uint32_t br = 0;
    uint32_t target = 1000000;  // 1MHz
    while (br <= 7) {
        if ((pclk1 >> (br + 1)) <= target) break; // 分频系数 2^(br+1)
        br++;
    }
    if (br > 7) br = 7;  // 最大分频 256
    hspi3.Init.BaudRatePrescaler = br << 3;   // 对应 CR1 的 BR[2:0] 位

    if (HAL_SPI_Init(&hspi3) != HAL_OK) {
        return;  // 需要定义错误处理函数
    }
}

/* 单字节全双工传输（不控制片选） */
uint8_t DRV_SPI3_TransmitReceive(uint8_t txData)
{
    uint8_t rxData;
    DRV_SPI3_CS_LOW();
    HAL_SPI_TransmitReceive(&hspi3, &txData, &rxData, 1, HAL_MAX_DELAY);
    DRV_SPI3_CS_HIGH();
    return rxData;
}

/* 多字节全双工传输（自动控制片选） */
void DRV_SPI3_TransmitReceiveBuffer(uint8_t *txBuf, uint8_t *rxBuf, uint16_t len)
{
    DRV_SPI3_CS_LOW();
    HAL_SPI_TransmitReceive(&hspi3, txBuf, rxBuf, len, HAL_MAX_DELAY);
    DRV_SPI3_CS_HIGH();
}

/* 便捷函数：读取16位寄存器（自动片选） */
uint16_t DRV_SPI3_ReadReg16(uint8_t regAddr)
{
    uint8_t tx[2] = {regAddr, 0x00};
    uint8_t rx[2];
    DRV_SPI3_TransmitReceiveBuffer(tx, rx, 2);
    return (rx[0] << 8) | rx[1];
}

/* 仅发送多字节（自动控制片选） */
void DRV_SPI3_SendBuffer(uint8_t *txBuf, uint16_t len)
{
    DRV_SPI3_CS_LOW();
    HAL_SPI_Transmit(&hspi3, txBuf, len, HAL_MAX_DELAY);
    DRV_SPI3_CS_HIGH();
}

/* 仅接收多字节（自动控制片选） */
void DRV_SPI3_ReceiveBuffer(uint8_t *rxBuf, uint16_t len)
{
    DRV_SPI3_CS_LOW();
    HAL_SPI_Receive(&hspi3, rxBuf, len, HAL_MAX_DELAY);
    DRV_SPI3_CS_HIGH();
}

/* 便捷函数：写入8位寄存器（自动片选） */
void DRV_SPI3_WriteReg8(uint8_t regAddr, uint8_t data)
{
    uint8_t tx[2] = {0x80 | regAddr, data};  // 写命令通常最高位置1
    DRV_SPI3_SendBuffer(tx, 2);
}
