#ifndef __UART_H__
#define __UART_H__

#include "main.h"
#include "ring_buffer.h"
#include "stm32f103x6.h"
#include "uart_hw.h"

void UART_SendByte(UART_Ctx *ctx, uint8_t data);
void UART_SendData(UART_Ctx *ctx, const uint8_t *data, uint8_t size);
void UART_SendString(UART_Ctx *ctx, const char *str);
void UART_SendHex(UART_Ctx *ctx, uint8_t hex);
void UART_SendBin(UART_Ctx *ctx, uint8_t bin);

__WEAK void UART_Handle_Recv(UART_Ctx *ctx);
void UART_SendByte_IT(UART_Ctx *ctx, uint8_t data);
void UART_SendData_IT(UART_Ctx *ctx, const uint8_t *data, uint8_t size);
void UART_SendString_IT(UART_Ctx *ctx, const char *str);
void UART_SendHex_IT(UART_Ctx *ctx, uint8_t hex);
void UART_SendBin_IT(UART_Ctx *ctx, uint8_t bin);
void UART_RecvData_IT(UART_Ctx *ctx, uint8_t size);
void UART_Transmit_RecvData_IT(UART_Ctx *ctx, uint8_t *data, uint8_t size);
void UART_Handle_IT(UART_Ctx *ctx);

void UART_SendData_DMA(UART_Ctx *ctx, const uint8_t *data, uint8_t size);
void UART_Handle_DMA_TX(UART_Ctx *ctx);
void UART_RecvData_DMA(UART_Ctx *ctx, uint8_t *data, uint8_t size);
void UART_Handle_DMA_RX(UART_Ctx *ctx);

__WEAK void UART_Handle_Recv_IDLE(UART_Ctx *ctx, uint8_t size);
void UART_RecvData_IDLE(UART_Ctx *ctx, uint8_t *data, uint8_t max_size);

#endif
