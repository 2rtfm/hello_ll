#include "uart.h"

static const char hexTable[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

void UART_SendByte(uint8_t data) {
  while (!LL_USART_IsActiveFlag_TXE(USART1))
    ;
  LL_USART_TransmitData8(USART1, data);
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
