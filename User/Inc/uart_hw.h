#ifndef __UART_HW_H__
#define __UART_HW_H__

#include "ring_buffer.h"

#ifndef TX_BUF_SIZE
#define TX_BUF_SIZE 32
#endif

#ifndef RX_BUF_SIZE
#define RX_BUF_SIZE 32
#endif

typedef struct {
  USART_TypeDef *uart;
  ring_buffer_t *tx_buf, *rx_buf;
  uint8_t tx_it_left, rx_it_left;
  uint8_t rx_idle_max_size;
  DMA_TypeDef *tx_dma, *rx_dma;
  uint32_t tx_dma_channel, rx_dma_channel;
} UART_Ctx;

extern UART_Ctx *const UART1;
extern UART_Ctx *const UART2;

#endif
