#ifndef __I2C_LL_H__
#define __I2C_LL_H__

#include "stm32f103x6.h"
#include "stm32f1xx_ll_i2c.h"
#include <stdint.h>

/* 返回值定义 */
#define I2C_LL_OK 0U    /*!< 传输成功 */
#define I2C_LL_ERROR 1U /*!< 传输失败（无应答 / 超时 / 参数错误） */
/* 简单超时计数值：每个等待标志的循环最多自减这么多次，防止总线异常时死循环 */
#define I2C_LL_TIMEOUT 0xFFFFFU

uint8_t I2C_LL_MasterTransmit(I2C_TypeDef *I2Cx, uint8_t DevAddr,
                              uint8_t *pData, uint16_t Size);
uint8_t I2C_LL_MasterReceive(I2C_TypeDef *I2Cx, uint8_t DevAddr, uint8_t *pData,
                             uint16_t Size);

#endif
