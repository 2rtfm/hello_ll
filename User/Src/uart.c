#include "uart.h"
#include "ring_buffer.h"
#include "stm32f103x6.h"
#include "stm32f1xx_ll_dma.h"
#include "stm32f1xx_ll_usart.h"
#include "uart_hw.h"

static const char hexTable[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

void UART_SendByte(UART_Ctx *ctx, uint8_t data) {
  while (!LL_USART_IsActiveFlag_TXE(ctx->uart))
    ;
  LL_USART_TransmitData8(ctx->uart, data);
}

void UART_SendData(UART_Ctx *ctx, const uint8_t *data, uint8_t size) {
  do {
    UART_SendByte(ctx, *data++);
  } while (--size);
}

void UART_SendString(UART_Ctx *ctx, const char *str) {
  while (*str != '\0') {
    UART_SendByte(ctx, *str++);
  }
}

void UART_SendHex(UART_Ctx *ctx, uint8_t hex) {
  UART_SendByte(ctx, hexTable[hex >> 0x4]);
  UART_SendByte(ctx, hexTable[hex & 0xF]);
}

void UART_SendBin(UART_Ctx *ctx, uint8_t bin) {
  uint8_t i = 8;
  do {
    UART_SendByte(ctx, bin & 0x80 ? '1' : '0');
    bin <<= 1;
  } while (--i);
}

void UART_SendU32Dec(UART_Ctx *ctx, uint32_t dec) {
  char buf[11] = "         0";
  uint8_t i = 9;
  while (dec > 0) {
    buf[i--] = '0' + dec % 10;
    dec /= 10;
  }
  UART_SendString(ctx, buf);
}

void UART_SendU32Bin(UART_Ctx *ctx, uint32_t bin) {
  uint8_t i = 3;
  do {
    UART_SendBin(ctx, bin >> (i << 3) & 0xFF);
    UART_SendByte(ctx, ' ');
  } while (i--);
}

void UART_SendU32Hex(UART_Ctx *ctx, uint32_t hex) {
  uint8_t i = 3;
  do {
    UART_SendHex(ctx, hex >> (i << 3) & 0xFF);
  } while (i--);
}

void UART_SendByte_IT(UART_Ctx *ctx, uint8_t data) {
  ctx->status = UART_STATUS_BUSY;
  ring_buffer_push(ctx->tx_buf, data);
  ctx->tx_it_left++;
  LL_USART_EnableIT_TXE(ctx->uart);
}

void UART_SendData_IT(UART_Ctx *ctx, const uint8_t *data, uint8_t size) {
  ctx->status = UART_STATUS_BUSY;
  do {
    ring_buffer_push(ctx->tx_buf, *data++);
    ctx->tx_it_left++;
  } while (--size);
  LL_USART_EnableIT_TXE(ctx->uart);
}

void UART_SendString_IT(UART_Ctx *ctx, const char *str) {
  ctx->status = UART_STATUS_BUSY;
  while (*str != '\0') {
    ring_buffer_push(ctx->tx_buf, *str++);
    ctx->tx_it_left++;
  }
  LL_USART_EnableIT_TXE(ctx->uart);
}

void UART_SendHex_IT(UART_Ctx *ctx, uint8_t hex) {
  UART_SendByte_IT(ctx, hexTable[hex >> 0x4]);
  UART_SendByte_IT(ctx, hexTable[hex & 0xF]);
}

void UART_SendBin_IT(UART_Ctx *ctx, uint8_t bin) {
  uint8_t i = 8;
  do {
    UART_SendByte_IT(ctx, bin & 0x80 ? '1' : '0');
    bin <<= 1;
  } while (--i);
}

__WEAK void UART_Handle_Recv_IT(UART_Ctx *ctx) {}
__WEAK void UART_Handle_Recv_DMA(UART_Ctx *ctx) {}

void UART_RecvData_IT(UART_Ctx *ctx, uint8_t size) {
  ctx->status = UART_STATUS_BUSY;
  ctx->rx_it_left += size;
  LL_USART_EnableIT_RXNE(ctx->uart);
}

uint8_t UART_Sizeof_RecvData_IT(UART_Ctx *ctx) {
  return ring_buffer_get_count(ctx->rx_buf);
}

void UART_Transmit_RecvData_IT(UART_Ctx *ctx, uint8_t *data, uint8_t size) {
  do {
    ring_buffer_pop(ctx->rx_buf, data++);
  } while (--size);
}

void UART_Handle_IT(UART_Ctx *ctx) {
  if (LL_USART_IsActiveFlag_TXE(ctx->uart) &&
      LL_USART_IsEnabledIT_TXE(ctx->uart)) {
    if (ctx->tx_it_left > 0) {
      uint8_t data;
      ring_buffer_pop(ctx->tx_buf, &data);
      LL_USART_TransmitData8(ctx->uart, data);
      if (--ctx->tx_it_left == 0) {
        LL_USART_DisableIT_TXE(ctx->uart);
        ctx->status = UART_STATUS_IDLE;
      }
    } else {
      LL_USART_DisableIT_TXE(ctx->uart);
      ctx->status = UART_STATUS_IDLE;
    }
  }
  if (LL_USART_IsActiveFlag_RXNE(ctx->uart) &&
      LL_USART_IsEnabledIT_RXNE(ctx->uart)) {
    if (ctx->rx_it_left > 0) {
      ring_buffer_push(ctx->rx_buf, LL_USART_ReceiveData8(ctx->uart));
      if (--ctx->rx_it_left == 0) {
        LL_USART_DisableIT_RXNE(ctx->uart);
        UART_Handle_Recv_IT(ctx);
        ctx->status = UART_STATUS_IDLE;
      }
    } else {
      LL_USART_DisableIT_RXNE(ctx->uart);
      ctx->status = UART_STATUS_IDLE;
    }
  }
  if (LL_USART_IsActiveFlag_IDLE(ctx->uart) &&
      LL_USART_IsEnabledIT_IDLE(ctx->uart)) {
    LL_DMA_DisableChannel(ctx->rx_dma, ctx->rx_dma_channel);
    uint16_t remain = LL_DMA_GetDataLength(ctx->rx_dma, ctx->rx_dma_channel);
    uint16_t size = ctx->rx_idle_max_size - remain;
    LL_USART_ClearFlag_IDLE(ctx->uart);
    UART_Handle_Recv_IDLE(ctx, size);
  }
}

static uint32_t UART_DMA_IsActiveFlag_TC(DMA_TypeDef *dma, uint32_t channel) {
  switch (channel) {
  case LL_DMA_CHANNEL_4:
    return LL_DMA_IsActiveFlag_TC4(dma);
  case LL_DMA_CHANNEL_5:
    return LL_DMA_IsActiveFlag_TC5(dma);
  case LL_DMA_CHANNEL_6:
    return LL_DMA_IsActiveFlag_TC6(dma);
  case LL_DMA_CHANNEL_7:
    return LL_DMA_IsActiveFlag_TC7(dma);
  default:
    return 0;
  }
}

static void UART_DMA_ClearFlag_TC(DMA_TypeDef *dma, uint32_t channel) {
  switch (channel) {
  case LL_DMA_CHANNEL_4:
    LL_DMA_ClearFlag_TC4(dma);
    break;
  case LL_DMA_CHANNEL_5:
    LL_DMA_ClearFlag_TC5(dma);
    break;
  case LL_DMA_CHANNEL_6:
    LL_DMA_ClearFlag_TC6(dma);
    break;
  case LL_DMA_CHANNEL_7:
    LL_DMA_ClearFlag_TC7(dma);
    break;
  default:
    break;
  }
}

void UART_SendData_DMA(UART_Ctx *ctx, const uint8_t *data, uint8_t size) {
  if (ctx->status != UART_STATUS_IDLE) {
    return;
  }
  LL_DMA_SetPeriphAddress(ctx->tx_dma, ctx->tx_dma_channel,
                          LL_USART_DMA_GetRegAddr(ctx->uart));
  LL_DMA_SetMemoryAddress(ctx->tx_dma, ctx->tx_dma_channel, (uint32_t)data);
  LL_DMA_SetDataLength(ctx->tx_dma, ctx->tx_dma_channel, size);
  LL_DMA_EnableIT_TC(ctx->tx_dma, ctx->tx_dma_channel);
  LL_USART_EnableDMAReq_TX(ctx->uart);
  LL_DMA_EnableChannel(ctx->tx_dma, ctx->tx_dma_channel);
  ctx->status = UART_STATUS_BUSY;
}

void UART_Handle_DMA_TX(UART_Ctx *ctx) {
  if (UART_DMA_IsActiveFlag_TC(ctx->tx_dma, ctx->tx_dma_channel)) {
    UART_DMA_ClearFlag_TC(ctx->tx_dma, ctx->tx_dma_channel);
    LL_USART_DisableDMAReq_TX(ctx->uart);
    LL_DMA_DisableChannel(ctx->tx_dma, ctx->tx_dma_channel);
    LL_DMA_DisableIT_TC(ctx->tx_dma, ctx->tx_dma_channel);
    ctx->status = UART_STATUS_IDLE;
  }
}

void UART_RecvData_DMA(UART_Ctx *ctx, uint8_t *data, uint8_t size) {
  if (ctx->status != UART_STATUS_IDLE) {
    return;
  }
  LL_DMA_SetPeriphAddress(ctx->rx_dma, ctx->rx_dma_channel,
                          LL_USART_DMA_GetRegAddr(ctx->uart));
  LL_DMA_SetMemoryAddress(ctx->rx_dma, ctx->rx_dma_channel, (uint32_t)data);
  LL_DMA_SetDataLength(ctx->rx_dma, ctx->rx_dma_channel, size);
  LL_DMA_EnableIT_TC(ctx->rx_dma, ctx->rx_dma_channel);
  LL_USART_EnableDMAReq_RX(ctx->uart);
  LL_DMA_EnableChannel(ctx->rx_dma, ctx->rx_dma_channel);
  ctx->status = UART_STATUS_BUSY;
}

void UART_Handle_DMA_RX(UART_Ctx *ctx) {
  if (UART_DMA_IsActiveFlag_TC(ctx->rx_dma, ctx->rx_dma_channel)) {
    UART_DMA_ClearFlag_TC(ctx->rx_dma, ctx->rx_dma_channel);
    LL_USART_DisableDMAReq_RX(ctx->uart);
    LL_DMA_DisableChannel(ctx->rx_dma, ctx->rx_dma_channel);
    LL_DMA_DisableIT_TC(ctx->rx_dma, ctx->rx_dma_channel);
    UART_Handle_Recv_DMA(ctx);
    ctx->status = UART_STATUS_IDLE;
  }
}

__WEAK void UART_Handle_Recv_IDLE(UART_Ctx *ctx, uint8_t size) {}

void UART_RecvData_IDLE(UART_Ctx *ctx, uint8_t *data, uint8_t max_size) {
  ctx->rx_idle_max_size = max_size;
  LL_DMA_SetPeriphAddress(ctx->rx_dma, ctx->rx_dma_channel,
                          LL_USART_DMA_GetRegAddr(ctx->uart));
  LL_DMA_SetMemoryAddress(ctx->rx_dma, ctx->rx_dma_channel, (uint32_t)data);
  LL_DMA_SetDataLength(ctx->rx_dma, ctx->rx_dma_channel, max_size);
  LL_USART_EnableDMAReq_RX(ctx->uart);
  LL_DMA_EnableChannel(ctx->rx_dma, ctx->rx_dma_channel);
  LL_USART_ClearFlag_IDLE(ctx->uart);
  LL_USART_EnableIT_IDLE(ctx->uart);
}
