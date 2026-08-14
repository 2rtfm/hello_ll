#ifndef __I2C_HW_H__
#define __I2C_HW_H__

#include "stm32f103x6.h"
#include <stdint.h>

typedef enum {
  I2C_LL_STATUS_IDLE,
  I2C_LL_STATUS_WAIT_SB_TX,
  I2C_LL_STATUS_WAIT_ADDR_TX,
  I2C_LL_STATUS_WAIT_TXE,
  I2C_LL_STATUS_WAIT_BTF_TX,
  I2C_LL_STATUS_WAIT_SB_RX,
  I2C_LL_STATUS_WAIT_ADDR_RX,
  I2C_LL_STATUS_WAIT_RXNE,
  I2C_LL_STATUS_WAIT_BTF_L2_RX,
  I2C_LL_STATUS_WAIT_BTF_L3_RX,
  I2C_LL_STATUS_ERROR
} I2C_LL_Status;

typedef struct {
  I2C_TypeDef *i2c;
  uint8_t addr;
  uint8_t *buf;
  uint8_t left;
  I2C_LL_Status status;
  DMA_TypeDef *tx_dma, *rx_dma;
  uint32_t tx_dma_channel, rx_dma_channel;
} I2C_Ctx;

extern I2C_Ctx *const cI2C1;

#endif
