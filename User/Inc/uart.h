#ifndef __UART_H__
#define __UART_H__

#include "main.h"

#ifndef TX_BUF_SIZE
#define TX_BUF_SIZE 32
#endif

#ifndef RX_BUF_SIZE
#define RX_BUF_SIZE 32
#endif

void UART_SendByte(uint8_t data);
void UART_SendData(const uint8_t *data, uint8_t size);
void UART_SendString(const char *str);
void UART_SendHex(uint8_t hex);
void UART_SendBin(uint8_t bin);

__WEAK void UART_Handle_Recv(void);
void UART_SendByte_IT(uint8_t data);
void UART_SendData_IT(const uint8_t *data, uint8_t size);
void UART_SendString_IT(const char *str);
void UART_SendHex_IT(uint8_t hex);
void UART_SendBin_IT(uint8_t bin);
void UART_RecvData_IT(uint8_t size);
void UART_Transmit_RecvData_IT(uint8_t *data, uint8_t size);
void UART_Handle_IT(void);

void UART_SendData_DMA(const uint8_t *data, uint8_t size);
void UART_Handle_DMA_TX(void);
void UART_RecvData_DMA(uint8_t *data, uint8_t size);
void UART_Handle_DMA_RX(void);

__WEAK void UART_Handle_Recv_IDLE(uint8_t size);
void UART_RecvData_IDLE(uint8_t *data, uint8_t max_size);

#endif
