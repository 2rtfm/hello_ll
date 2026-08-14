#include "i2c_hw.h"
#include "stm32f103x6.h"
#include "stm32f1xx_ll_dma.h"

static I2C_Ctx I2C1_Ctx = {.i2c = I2C1,
                           .tx_dma = DMA1,
                           .rx_dma = DMA1,
                           .tx_dma_channel = LL_DMA_CHANNEL_6,
                           .rx_dma_channel = LL_DMA_CHANNEL_7};
I2C_Ctx *const cI2C1 = &I2C1_Ctx;
