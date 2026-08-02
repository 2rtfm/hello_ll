#ifndef __UART_H__
#define __UART_H__

#include "main.h"

void UART_SendByte(uint8_t data);
void UART_SendString(const char *str);
void UART_SendHex(uint8_t hex);
void UART_SendBin(uint8_t bin);

#endif
