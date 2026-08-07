#include "uart.h"
#include "ring_buffer.h"
#include "stm32f103x6.h"
#include "stm32f1xx_ll_dma.h"
#include "stm32f1xx_ll_usart.h"
#include <stdint.h>

#define TARGET_UART USART1

static const char hexTable[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

static uint8_t tx_buf_storage[TX_BUF_SIZE];
static ring_buffer_t tx_buf = {
    .storage = tx_buf_storage, .size = TX_BUF_SIZE, .head = 0, .tail = 0};

static uint8_t rx_buf_storage[RX_BUF_SIZE];
static ring_buffer_t rx_buf = {
    .storage = rx_buf_storage, .size = RX_BUF_SIZE, .head = 0, .tail = 0};

// static uint8_t *tx_it_data, tx_it_left;
// static uint8_t *rx_it_data, rx_it_left;
static uint8_t tx_it_left, rx_it_left;
static uint8_t rx_idle_max_size;

void UART_SendByte(uint8_t data) {
  while (!LL_USART_IsActiveFlag_TXE(TARGET_UART))
    ;
  LL_USART_TransmitData8(TARGET_UART, data);
}

void UART_SendData(const uint8_t *data, uint8_t size) {
  do {
    UART_SendByte(*data++);
  } while (--size);
}

void UART_SendString(const char *str) {
  while (*str != '\0') {
    UART_SendByte(*str++);
  }
}

void UART_SendHex(uint8_t hex) {
  UART_SendByte(hexTable[hex >> 0x4]);
  UART_SendByte(hexTable[hex & 0xF]);
}

void UART_SendBin(uint8_t bin) {
  uint8_t i = 8;
  do {
    UART_SendByte(bin & 0x80 ? '1' : '0');
    bin <<= 1;
  } while (--i);
}

void UART_SendByte_IT(uint8_t data) {
  ring_buffer_push(&tx_buf, data);
  tx_it_left++;
  LL_USART_EnableIT_TXE(TARGET_UART);
}

void UART_SendData_IT(const uint8_t *data, uint8_t size) {
  do {
    ring_buffer_push(&tx_buf, *data++);
    tx_it_left++;
  } while (--size);
  LL_USART_EnableIT_TXE(TARGET_UART);
}

void UART_SendString_IT(const char *str) {
  while (*str != '\0') {
    ring_buffer_push(&tx_buf, *str++);
    tx_it_left++;
  }
  LL_USART_EnableIT_TXE(TARGET_UART);
}

void UART_SendHex_IT(uint8_t hex) {
  UART_SendByte_IT(hexTable[hex >> 0x4]);
  UART_SendByte_IT(hexTable[hex & 0xF]);
}

void UART_SendBin_IT(uint8_t bin) {
  uint8_t i = 8;
  do {
    UART_SendByte_IT(bin & 0x80 ? '1' : '0');
    bin <<= 1;
  } while (--i);
}

__WEAK void UART_Handle_Recv(void) {}

void UART_RecvData_IT(uint8_t size) {
  rx_it_left += size;
  LL_USART_EnableIT_RXNE(TARGET_UART);
}

void UART_Transmit_RecvData_IT(uint8_t *data, uint8_t size) {
  do {
    ring_buffer_pop(&rx_buf, data++);
  } while (--size);
}

void UART_Handle_IT(void) {
  if (LL_USART_IsActiveFlag_TXE(TARGET_UART) &&
      LL_USART_IsEnabledIT_TXE(TARGET_UART)) {
    if (tx_it_left > 0) {
      uint8_t data;
      ring_buffer_pop(&tx_buf, &data);
      LL_USART_TransmitData8(TARGET_UART, data);
      if (--tx_it_left == 0) {
        LL_USART_DisableIT_TXE(TARGET_UART);
      }
    } else {
      LL_USART_DisableIT_TXE(TARGET_UART);
    }
  }
  if (LL_USART_IsActiveFlag_RXNE(TARGET_UART) &&
      LL_USART_IsEnabledIT_RXNE(TARGET_UART)) {
    if (rx_it_left > 0) {
      ring_buffer_push(&rx_buf, LL_USART_ReceiveData8(TARGET_UART));
      if (--rx_it_left == 0) {
        LL_USART_DisableIT_RXNE(TARGET_UART);
        UART_Handle_Recv();
      }
    } else {
      LL_USART_DisableIT_RXNE(TARGET_UART);
    }
  }
  if (LL_USART_IsActiveFlag_IDLE(TARGET_UART) &&
      LL_USART_IsEnabledIT_IDLE(TARGET_UART)) {
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_5);
    uint16_t remain = LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_5);
    uint16_t size = rx_idle_max_size - remain;
    LL_USART_ClearFlag_IDLE(USART1);
    UART_Handle_Recv_IDLE(size);
  }
}

void UART_SendData_DMA(const uint8_t *data, uint8_t size) {
  LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_4,
                          LL_USART_DMA_GetRegAddr(USART1));
  LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_4, (uint32_t)data);
  LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_4, size);
  LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_4);
  LL_USART_EnableDMAReq_TX(USART1);
  LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_4);
}

void UART_Handle_DMA_TX(void) {
  if (LL_DMA_IsActiveFlag_TC4(DMA1)) {
    LL_DMA_ClearFlag_TC4(DMA1);
    LL_USART_DisableDMAReq_TX(USART1);
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_4);
    LL_DMA_DisableIT_TC(DMA1, LL_DMA_CHANNEL_4);
  }
}

void UART_RecvData_DMA(uint8_t *data, uint8_t size) {
  LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_5,
                          LL_USART_DMA_GetRegAddr(USART1));
  LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_5, (uint32_t)data);
  LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_5, size);
  LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_5);
  LL_USART_EnableDMAReq_RX(USART1);
  LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_5);
}

void UART_Handle_DMA_RX(void) {
  if (LL_DMA_IsActiveFlag_TC5(DMA1)) {
    LL_DMA_ClearFlag_TC5(DMA1);
    LL_USART_DisableDMAReq_RX(USART1);
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_5);
    LL_DMA_DisableIT_TC(DMA1, LL_DMA_CHANNEL_5);
    UART_Handle_Recv();
  }
}

__WEAK void UART_Handle_Recv_IDLE(uint8_t size) {}

void UART_RecvData_IDLE(uint8_t *data, uint8_t max_size) {
  rx_idle_max_size = max_size;
  LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_5,
                          LL_USART_DMA_GetRegAddr(USART1));
  LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_5, (uint32_t)data);
  LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_5, max_size);
  LL_USART_EnableDMAReq_RX(USART1);
  LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_5);
  LL_USART_ClearFlag_IDLE(TARGET_UART);
  LL_USART_EnableIT_IDLE(TARGET_UART);
}
