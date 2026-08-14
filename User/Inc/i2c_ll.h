#ifndef __I2C_LL_H__
#define __I2C_LL_H__

#include "i2c_hw.h"
#include "stm32f103x6.h"
#include "stm32f1xx_ll_i2c.h"
#include <stdint.h>

/* 返回值定义 */
/* 简单超时计数值：每个等待标志的循环最多自减这么多次，防止总线异常时死循环 */
#define I2C_LL_TIMEOUT 0xFFFFFU

typedef enum { I2C_LL_OK, I2C_LL_ERROR } I2C_LL_ReturnVal;

typedef struct {
  uint8_t I2C_LL_ERROR_BERR : 1;
  uint8_t I2C_LL_ERROR_AF : 1;
  uint8_t I2C_LL_ERROR_ARLO : 1;
  uint8_t I2C_LL_ERROR_OVR : 1;
} I2C_LL_Error;

I2C_LL_ReturnVal I2C_LL_MasterTransmit(I2C_TypeDef *I2Cx, uint8_t DevAddr,
                                       uint8_t *pData, uint16_t Size);
I2C_LL_ReturnVal I2C_LL_MasterReceive(I2C_TypeDef *I2Cx, uint8_t DevAddr,
                                      uint8_t *pData, uint16_t Size);

__WEAK void I2C_LL_MasterTx_Callback(I2C_Ctx *ctx);
__WEAK void I2C_LL_MasterRx_Callback(I2C_Ctx *ctx);
__WEAK void I2C_LL_Error_Callback(I2C_Ctx *ctx, I2C_LL_Error error);

I2C_LL_ReturnVal I2C_LL_MasterTransmit_IT(I2C_Ctx *ctx, uint8_t DevAddr,
                                          uint8_t *pData, uint16_t Size);
I2C_LL_ReturnVal I2C_LL_MasterReceive_IT(I2C_Ctx *ctx, uint8_t DevAddr,
                                         uint8_t *pData, uint16_t Size);

void I2C_LL_Handle_EV_IT(I2C_Ctx *ctx);
void I2C_LL_Handle_ER_IT(I2C_Ctx *ctx);

#endif
