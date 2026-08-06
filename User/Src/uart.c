#include "uart.h"
#include "ring_buffer.h"
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
  if (LL_USART_IsActiveFlag_TXE(TARGET_UART)) {
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
  if (LL_USART_IsActiveFlag_RXNE(TARGET_UART)) {
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
}
