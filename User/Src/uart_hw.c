#include "uart_hw.h"
#include "stm32f103x6.h"
#include "stm32f1xx_ll_dma.h"
#include <stdint.h>

static uint8_t uart1_tx_buf_storage[TX_BUF_SIZE];
static ring_buffer_t uart1_tx_buf = {
    .storage = uart1_tx_buf_storage, .size = TX_BUF_SIZE, .head = 0, .tail = 0};

static uint8_t uart1_rx_buf_storage[RX_BUF_SIZE];
static ring_buffer_t uart1_rx_buf = {
    .storage = uart1_rx_buf_storage, .size = RX_BUF_SIZE, .head = 0, .tail = 0};

static UART_Ctx UART1_Ctx = {.uart = USART1,
                             .tx_buf = &uart1_tx_buf,
                             .rx_buf = &uart1_rx_buf,
                             .tx_dma = DMA1,
                             .rx_dma = DMA1,
                             .tx_dma_channel = LL_DMA_CHANNEL_4,
                             .rx_dma_channel = LL_DMA_CHANNEL_5};
UART_Ctx *const UART1 = &UART1_Ctx;

static uint8_t uart2_tx_buf_storage[TX_BUF_SIZE];
static ring_buffer_t uart2_tx_buf = {
    .storage = uart2_tx_buf_storage, .size = TX_BUF_SIZE, .head = 0, .tail = 0};

static uint8_t uart2_rx_buf_storage[RX_BUF_SIZE];
static ring_buffer_t uart2_rx_buf = {
    .storage = uart2_rx_buf_storage, .size = RX_BUF_SIZE, .head = 0, .tail = 0};

static UART_Ctx UART2_Ctx = {.uart = USART2,
                             .tx_buf = &uart2_tx_buf,
                             .rx_buf = &uart2_rx_buf,
                             .tx_dma = DMA1,
                             .rx_dma = DMA1,
                             .tx_dma_channel = LL_DMA_CHANNEL_7,
                             .rx_dma_channel = LL_DMA_CHANNEL_6};
UART_Ctx *const UART2 = &UART2_Ctx;
