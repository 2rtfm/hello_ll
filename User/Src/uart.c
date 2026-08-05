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

static uint8_t *tx_it_data, tx_it_left;
static uint8_t *rx_it_data, rx_it_left;

void UART_SendByte(uint8_t data) {
  while (!LL_USART_IsActiveFlag_TXE(TARGET_UART))
    ;
  LL_USART_TransmitData8(TARGET_UART, data);
}

void UART_SendString(const char *str) {
  while (*str != '\0') {
    UART_SendByte(*str);
    str++;
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

void UART_SendData_IT(uint8_t *data, uint8_t size) {
  tx_it_data = data;
  tx_it_left = size;
  LL_USART_EnableIT_TXE(TARGET_UART);
}

__WEAK void UART_Handle_Recv(void) {}

void UART_RecvData_IT(uint8_t *data, uint8_t size) {
  rx_it_data = data;
  rx_it_left = size;
  LL_USART_EnableIT_RXNE(TARGET_UART);
}

void UART_Handle_IT(void) {
  if (LL_USART_IsActiveFlag_TXE(TARGET_UART) &&
      LL_USART_IsEnabledIT_TXE(TARGET_UART)) {
    if (tx_it_left > 0) {
      LL_USART_TransmitData8(TARGET_UART, *tx_it_data++);
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
      *rx_it_data++ = LL_USART_ReceiveData8(TARGET_UART);
      if (--rx_it_left == 0) {
        LL_USART_DisableIT_RXNE(TARGET_UART);
        UART_Handle_Recv();
      }
    } else {
      LL_USART_DisableIT_RXNE(TARGET_UART);
    }
  }
}
