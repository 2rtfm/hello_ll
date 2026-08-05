#ifndef __UART_H__
#define __UART_H__

#include "main.h"

#ifndef TX_BUF_SIZE
#define TX_BUF_SIZE 8
#endif

#ifndef RX_BUF_SIZE
#define RX_BUF_SIZE 8
#endif

void UART_SendByte(uint8_t data);
void UART_SendString(const char *str);
void UART_SendHex(uint8_t hex);
void UART_SendBin(uint8_t bin);

__WEAK void UART_Handle_Recv(void);
void UART_SendData_IT(uint8_t *data, uint8_t size);
void UART_RecvData_IT(uint8_t *data, uint8_t size);
void UART_Handle_IT(void);

#endif
