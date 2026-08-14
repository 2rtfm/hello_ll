#include "i2c_ll.h"
#include "i2c.h"
#include "i2c_hw.h"
#include "stm32f1xx_ll_i2c.h"

/**
 * @brief  主机向从机发送数据（阻塞式轮询）
 * @param  I2Cx    I2C 外设实例（如 I2C1）
 * @param  DevAddr 从机 7 位地址
 * @param  pData   待发送数据缓冲区指针
 * @param  Size    待发送数据长度（字节）
 * @retval I2C_LL_OK(0) 成功；I2C_LL_ERROR(1) 失败
 */
I2C_LL_ReturnVal I2C_LL_MasterTransmit(I2C_TypeDef *I2Cx, uint8_t DevAddr,
                                       uint8_t *pData, uint16_t Size) {
  uint32_t timeout;

  /* 参数检查 */
  if ((pData == NULL) || (Size == 0U)) {
    return I2C_LL_ERROR;
  }

  /* 1. 等待总线空闲（BUSY=0），对应 HAL: I2C_WaitOnFlagUntilTimeout(BUSY) */
  timeout = I2C_LL_TIMEOUT;
  while (LL_I2C_IsActiveFlag_BUSY(I2Cx) != 0U) {
    if (--timeout == 0U) {
      return I2C_LL_ERROR; /* 总线忙超时 */
    }
  }

  /* 2. 使能 I2C 外设（PE=1），对应 HAL: __HAL_I2C_ENABLE */
  if (LL_I2C_IsEnabled(I2Cx) == 0U) {
    LL_I2C_Enable(I2Cx);
  }

  /* 3. 清除 POS 位，对应 HAL: CLEAR_BIT(CR1, POS)（轮询方式保持 POS=0） */
  LL_I2C_DisableBitPOS(I2Cx);

  /* 4. 产生起始条件，对应 HAL: I2C_MasterRequestWrite 中 SET_BIT(CR1, START) */
  LL_I2C_GenerateStartCondition(I2Cx);

  /* 5. 等待 SB 标志（起始条件已发送），对应 HAL 等待 SB */
  timeout = I2C_LL_TIMEOUT;
  while (LL_I2C_IsActiveFlag_SB(I2Cx) == 0U) {
    if (--timeout == 0U) {
      goto error_stop;
    }
  }

  /* 6. 发送从机地址：7 位地址左移 1 位，bit0=0 表示写方向，
        对应 HAL: I2C_7BIT_ADD_WRITE(DevAddress) 写入 DR */
  LL_I2C_TransmitData8(I2Cx, (uint8_t)DevAddr);

  /* 7. 等待 ADDR 标志（从机已应答地址），期间检查 AF 无应答标志，
        对应 HAL: I2C_WaitOnMasterAddressFlagUntilTimeout */
  timeout = I2C_LL_TIMEOUT;
  while (LL_I2C_IsActiveFlag_ADDR(I2Cx) == 0U) {
    if (LL_I2C_IsActiveFlag_AF(I2Cx) != 0U) {
      /* 从机无应答：清除 AF 并产生停止条件释放总线，返回错误 */
      LL_I2C_ClearFlag_AF(I2Cx);
      // LL_I2C_GenerateStopCondition(I2Cx);
      goto error_stop;
    }
    if (--timeout == 0U) {
      goto error_stop;
    }
  }

  /* 8. 清除 ADDR 标志（读 SR1 再读 SR2），对应 HAL: __HAL_I2C_CLEAR_ADDRFLAG */
  LL_I2C_ClearFlag_ADDR(I2Cx);

  /* 9. 循环发送数据，对应 HAL 发送循环（含 BTF 双字节优化） */
  while (Size > 0U) {
    /* 等待 TXE 标志（发送数据寄存器空），期间检查 AF */
    timeout = I2C_LL_TIMEOUT;
    while (LL_I2C_IsActiveFlag_TXE(I2Cx) == 0U &&
           LL_I2C_IsActiveFlag_AF(I2Cx) == 0U) {
      if (--timeout == 0U) {
        goto error_stop;
      }
    }
    if (LL_I2C_IsActiveFlag_AF(I2Cx) != 0U) {
      LL_I2C_ClearFlag_AF(I2Cx);
      // LL_I2C_GenerateStopCondition(I2Cx);
      goto error_stop;
    }

    /* 写一个数据字节到 DR */
    LL_I2C_TransmitData8(I2Cx, *pData++);
    Size--;
  }
  // 最后一字节传输后等待 BTF 传输结束，再产生终止条件
  timeout = I2C_LL_TIMEOUT;
  while (LL_I2C_IsActiveFlag_BTF(I2Cx) == 0U) {
    if (LL_I2C_IsActiveFlag_AF(I2Cx) != 0U) {
      LL_I2C_ClearFlag_AF(I2Cx);
      // LL_I2C_GenerateStopCondition(I2Cx);
      goto error_stop;
    }
    if (--timeout == 0U) {
      goto error_stop;
    }
  }
  /* 10. 产生停止条件结束传输，对应 HAL: SET_BIT(CR1, STOP) */
  LL_I2C_GenerateStopCondition(I2Cx);
  return I2C_LL_OK;

error_stop:
  LL_I2C_GenerateStopCondition(I2Cx);
  return I2C_LL_ERROR;
}

/**
 * @brief  主机从从机接收数据（阻塞式轮询）
 * @param  I2Cx    I2C 外设实例（如 I2C1）
 * @param  DevAddr 从机 7 位地址
 * @param  pData   接收数据缓冲区指针
 * @param  Size    待接收数据长度（字节）
 * @retval I2C_LL_OK(0) 成功；I2C_LL_ERROR(1) 失败
 *
 * @note   接收时序要点（F1 特有，对应 HAL EV6 事件处理）：
 *           - 收 1 字节：清 ADDR 前禁止应答，清 ADDR 后立即产生停止；
 *           - 收 2 字节：置 POS=1（保证第 1 字节仍被应答），清 ADDR
 * 后禁止应答， 等 BTF 时 DR 与移位寄存器各 1 字节，发停止后连续读 2 次；
 *           - 收 N(N>2) 字节：正常应答接收，最后 3 字节通过 BTF 机制收尾
 *             （禁止应答使最后一个字节收到 NACK，从机停止发送）。
 */
I2C_LL_ReturnVal I2C_LL_MasterReceive(I2C_TypeDef *I2Cx, uint8_t DevAddr,
                                      uint8_t *pData, uint16_t Size) {
  uint32_t timeout;

  /* 参数检查 */
  if ((pData == NULL) || (Size == 0U)) {
    return I2C_LL_ERROR;
  }

  /* 1. 等待总线空闲（BUSY=0） */
  timeout = I2C_LL_TIMEOUT;
  while (LL_I2C_IsActiveFlag_BUSY(I2Cx) != 0U) {
    if (--timeout == 0U) {
      return I2C_LL_ERROR;
    }
  }

  /* 2. 使能 I2C 外设（PE=1） */
  if (LL_I2C_IsEnabled(I2Cx) == 0U) {
    LL_I2C_Enable(I2Cx);
  }

  /* 3. 清除 POS 位（多字节接收场景使用 POS=0 即可） */
  LL_I2C_DisableBitPOS(I2Cx);

  /* 4. 产生起始条件 */
  LL_I2C_GenerateStartCondition(I2Cx);

  /* 5. 等待 SB 标志（起始条件已发送） */
  timeout = I2C_LL_TIMEOUT;
  while (LL_I2C_IsActiveFlag_SB(I2Cx) == 0U) {
    if (--timeout == 0U) {
      goto error_stop;
    }
  }

  /* 6. 发送从机地址：7 位地址左移 1 位，bit0=1 表示读方向，
        对应 HAL: I2C_7BIT_ADD_READ(DevAddress) 写入 DR */
  LL_I2C_TransmitData8(I2Cx, (uint8_t)(DevAddr | 0x01U));

  LL_I2C_AcknowledgeNextData(I2Cx, LL_I2C_ACK);

  /* 7. 等待 ADDR 标志，期间检查 AF 无应答标志 */
  timeout = I2C_LL_TIMEOUT;
  while (LL_I2C_IsActiveFlag_ADDR(I2Cx) == 0U) {
    if (LL_I2C_IsActiveFlag_AF(I2Cx) != 0U) {
      LL_I2C_ClearFlag_AF(I2Cx);
      // LL_I2C_GenerateStopCondition(I2Cx);
      goto error_stop;
    }
    if (--timeout == 0U) {
      goto error_stop;
    }
  }

  /* 8. 根据接收字节数配置 ACK/POS 并清除 ADDR，对应 HAL 中 EV6 事件的分支处理
   */
  if (Size == 1U) {
    /* 单字节：禁止应答 → 清 ADDR → 立即产生停止（EV6_3 时序，
       否则从机会多发一个字节且永远收不到停止） */
    LL_I2C_AcknowledgeNextData(I2Cx, LL_I2C_NACK);
    LL_I2C_ClearFlag_ADDR(I2Cx);
    LL_I2C_GenerateStopCondition(I2Cx);
  } else if (Size == 2U) {
    /* 双字节：置 POS=1（让第 1 字节仍被应答）→ 清 ADDR → 禁止应答
       （第 2 字节将收到 NACK，对应 HAL EV6_2 时序） */
    LL_I2C_EnableBitPOS(I2Cx);
    LL_I2C_ClearFlag_ADDR(I2Cx);
    LL_I2C_AcknowledgeNextData(I2Cx, LL_I2C_NACK);
  } else {
    /* 多字节：使能应答 → 清 ADDR，正常接收 */
    LL_I2C_ClearFlag_ADDR(I2Cx);
  }

  /* 9. 接收数据循环，对应 HAL 接收循环（含 1/2/3 字节收尾与 BTF 优化） */
  while (Size > 0U) {
    if (Size == 1U) {
      /* 最后一个字节：等 RXNE 读 1 字节（停止条件已在步骤 8 或上次 BTF 时产生）
       */
      timeout = I2C_LL_TIMEOUT;
      while (LL_I2C_IsActiveFlag_RXNE(I2Cx) == 0U) {
        if (--timeout == 0U) {
          goto error_stop;
        }
      }
      *pData = LL_I2C_ReceiveData8(I2Cx);
      pData++;
      Size--;
    } else if (Size == 2U) {
      /* 最后 2 字节：等 BTF（DR 与移位寄存器各 1 字节）→ 产生停止
         → 连续读 2 次 DR（读第 1 次后移位寄存器内容自动移入 DR） */
      timeout = I2C_LL_TIMEOUT;
      while (LL_I2C_IsActiveFlag_BTF(I2Cx) == 0U) {
        if (--timeout == 0U) {
          goto error_stop;
        }
      }
      LL_I2C_GenerateStopCondition(I2Cx);
      *pData = LL_I2C_ReceiveData8(I2Cx);
      pData++;
      *pData = LL_I2C_ReceiveData8(I2Cx);
      pData++;
      Size -= 2U;
    } else if (Size == 3U) {
      /* 最后 3 字节：等 BTF（硬件中已有 2 字节）→ 禁止应答（最后一字节 NACK）
         → 读 1 字节 → 等 BTF（第 3 字节到达移位寄存器）→ 产生停止 → 读 2 字节
       */
      timeout = I2C_LL_TIMEOUT;
      while (LL_I2C_IsActiveFlag_BTF(I2Cx) == 0U) {
        if (--timeout == 0U) {
          goto error_stop;
        }
      }
      LL_I2C_AcknowledgeNextData(I2Cx, LL_I2C_NACK);
      *pData = LL_I2C_ReceiveData8(I2Cx);
      pData++;
      Size--;
    } else {
      /* 多于 3 字节：等 RXNE 读 1 字节 */
      timeout = I2C_LL_TIMEOUT;
      while (LL_I2C_IsActiveFlag_RXNE(I2Cx) == 0U) {
        if (--timeout == 0U) {
          goto error_stop;
        }
      }
      *pData = LL_I2C_ReceiveData8(I2Cx);
      pData++;
      Size--;
    }
  }

  return I2C_LL_OK;

error_stop:
  LL_I2C_GenerateStopCondition(I2Cx);
  return I2C_LL_ERROR;
}

__WEAK void I2C_LL_MasterTx_Callback(I2C_Ctx *ctx) {}
__WEAK void I2C_LL_MasterRx_Callback(I2C_Ctx *ctx) {}
__WEAK void I2C_LL_Error_Callback(I2C_Ctx *ctx, I2C_LL_Error error) {}

/**
 * @brief  主机向从机发送数据（中断）
 * @param  ctx     I2C 外设 Ctx 实例（如 cI2C1）
 * @param  DevAddr 从机 7 位地址
 * @param  pData   待发送数据缓冲区指针
 * @param  Size    待发送数据长度（字节）
 * @retval I2C_LL_OK(0) 成功；I2C_LL_ERROR(1) 失败
 */
I2C_LL_ReturnVal I2C_LL_MasterTransmit_IT(I2C_Ctx *ctx, uint8_t DevAddr,
                                          uint8_t *pData, uint16_t Size) {

  /* 参数检查 */
  if ((pData == NULL) || (Size == 0U)) {
    return I2C_LL_ERROR;
  }

  if (ctx->status != I2C_LL_STATUS_IDLE) {
    return I2C_LL_ERROR;
  }

  ctx->addr = DevAddr;
  ctx->buf = pData;
  ctx->left = Size;
  ctx->status = I2C_LL_STATUS_WAIT_SB_TX;

  LL_I2C_EnableIT_EVT(ctx->i2c);
  LL_I2C_EnableIT_ERR(ctx->i2c);

  /* 2. 使能 I2C 外设（PE=1），对应 HAL: __HAL_I2C_ENABLE */
  if (LL_I2C_IsEnabled(ctx->i2c) == 0U) {
    LL_I2C_Enable(ctx->i2c);
  }

  /* 3. 清除 POS 位，对应 HAL: CLEAR_BIT(CR1, POS)（轮询方式保持 POS=0） */
  LL_I2C_DisableBitPOS(ctx->i2c);

  /* 4. 产生起始条件，对应 HAL: I2C_MasterRequestWrite 中 SET_BIT(CR1, START) */
  LL_I2C_GenerateStartCondition(ctx->i2c);
  return I2C_LL_OK;
}

/**
 * @brief  主机从从机接收数据（阻塞式轮询）
 * @param  I2Cx    I2C 外设实例（如 I2C1）
 * @param  DevAddr 从机 7 位地址
 * @param  pData   接收数据缓冲区指针
 * @param  Size    待接收数据长度（字节）
 * @retval I2C_LL_OK(0) 成功；I2C_LL_ERROR(1) 失败
 *
 * @note   接收时序要点（F1 特有，对应 HAL EV6 事件处理）：
 *           - 收 1 字节：清 ADDR 前禁止应答，清 ADDR 后立即产生停止；
 *           - 收 2 字节：置 POS=1（保证第 1 字节仍被应答），清 ADDR
 * 后禁止应答， 等 BTF 时 DR 与移位寄存器各 1 字节，发停止后连续读 2 次；
 *           - 收 N(N>2) 字节：正常应答接收，最后 3 字节通过 BTF 机制收尾
 *             （禁止应答使最后一个字节收到 NACK，从机停止发送）。
 */
I2C_LL_ReturnVal I2C_LL_MasterReceive_IT(I2C_Ctx *ctx, uint8_t DevAddr,
                                         uint8_t *pData, uint16_t Size) {

  /* 参数检查 */
  if ((pData == NULL) || (Size == 0U)) {
    return I2C_LL_ERROR;
  }

  if (ctx->status != I2C_LL_STATUS_IDLE) {
    return I2C_LL_ERROR;
  }

  ctx->addr = DevAddr;
  ctx->buf = pData;
  ctx->left = Size;
  ctx->status = I2C_LL_STATUS_WAIT_SB_RX;

  LL_I2C_EnableIT_EVT(ctx->i2c);
  LL_I2C_EnableIT_ERR(ctx->i2c);

  /* 2. 使能 I2C 外设（PE=1） */
  // if (LL_I2C_IsEnabled(I2Cx) == 0U) {
  LL_I2C_Enable(ctx->i2c);
  // }

  /* 3. 清除 POS 位（多字节接收场景使用 POS=0 即可） */
  LL_I2C_DisableBitPOS(ctx->i2c);

  /* 4. 产生起始条件 */
  LL_I2C_GenerateStartCondition(ctx->i2c);
  return I2C_LL_OK;
}

void I2C_LL_Handle_EV_IT(I2C_Ctx *ctx) {
  switch (ctx->status) {
  case I2C_LL_STATUS_IDLE:
    break;
  case I2C_LL_STATUS_WAIT_SB_TX:
    if (LL_I2C_IsActiveFlag_SB(ctx->i2c)) {
      LL_I2C_TransmitData8(ctx->i2c, ctx->addr);
      ctx->status = I2C_LL_STATUS_WAIT_ADDR_TX;
    }
    break;
  case I2C_LL_STATUS_WAIT_ADDR_TX:
    if (LL_I2C_IsActiveFlag_ADDR(ctx->i2c)) {
      LL_I2C_ClearFlag_ADDR(ctx->i2c);
      LL_I2C_EnableIT_BUF(ctx->i2c);
      ctx->status = I2C_LL_STATUS_WAIT_TXE;
    }
    break;
  case I2C_LL_STATUS_WAIT_TXE:
    if (LL_I2C_IsActiveFlag_TXE(ctx->i2c)) {
      if (ctx->left > 0U) {
        LL_I2C_TransmitData8(ctx->i2c, *ctx->buf++);
        ctx->left--;
      }
      if (ctx->left == 0U) {
        LL_I2C_DisableIT_BUF(ctx->i2c);
        ctx->status = I2C_LL_STATUS_WAIT_BTF_TX;
      }
    }
    break;
  case I2C_LL_STATUS_WAIT_BTF_TX:
    if (LL_I2C_IsActiveFlag_BTF(ctx->i2c)) {
      LL_I2C_GenerateStopCondition(ctx->i2c);
      LL_I2C_DisableIT_EVT(ctx->i2c);
      LL_I2C_DisableIT_ERR(ctx->i2c);
      ctx->status = I2C_LL_STATUS_IDLE;
      I2C_LL_MasterTx_Callback(ctx);
    }
    break;
  case I2C_LL_STATUS_WAIT_SB_RX:
    if (LL_I2C_IsActiveFlag_SB(ctx->i2c)) {
      LL_I2C_TransmitData8(ctx->i2c, ctx->addr | 0x01U);
      LL_I2C_AcknowledgeNextData(ctx->i2c, LL_I2C_ACK);
      ctx->status = I2C_LL_STATUS_WAIT_ADDR_RX;
    }
    break;
  case I2C_LL_STATUS_WAIT_ADDR_RX:
    if (LL_I2C_IsActiveFlag_ADDR(ctx->i2c)) {
      if (ctx->left == 1U) {
        /* 单字节：禁止应答 → 清 ADDR → 立即产生停止（EV6_3 时序，
           否则从机会多发一个字节且永远收不到停止） */
        LL_I2C_AcknowledgeNextData(ctx->i2c, LL_I2C_NACK);
        LL_I2C_ClearFlag_ADDR(ctx->i2c);
        LL_I2C_GenerateStopCondition(ctx->i2c);
        LL_I2C_EnableIT_BUF(ctx->i2c);
        ctx->status = I2C_LL_STATUS_WAIT_RXNE;
      } else if (ctx->left == 2U) {
        /* 双字节：置 POS=1（让第 1 字节仍被应答）→ 清 ADDR → 禁止应答
           （第 2 字节将收到 NACK，对应 HAL EV6_2 时序） */
        LL_I2C_EnableBitPOS(ctx->i2c);
        LL_I2C_ClearFlag_ADDR(ctx->i2c);
        LL_I2C_AcknowledgeNextData(ctx->i2c, LL_I2C_NACK);
        ctx->status = I2C_LL_STATUS_WAIT_BTF_L2_RX;
      } else if (ctx->left == 3U) {
        LL_I2C_ClearFlag_ADDR(ctx->i2c);
        ctx->status = I2C_LL_STATUS_WAIT_BTF_L3_RX;
      } else {
        /* 多字节：使能应答 → 清 ADDR，正常接收 */
        LL_I2C_ClearFlag_ADDR(ctx->i2c);
        LL_I2C_EnableIT_BUF(ctx->i2c);
        ctx->status = I2C_LL_STATUS_WAIT_RXNE;
      }
    }
    break;
  case I2C_LL_STATUS_WAIT_RXNE:
    if (LL_I2C_IsActiveFlag_RXNE(ctx->i2c)) {
      if (ctx->left == 1U) {
        /* 单字节接收（停止条件已产生）*/
        *ctx->buf = LL_I2C_ReceiveData8(ctx->i2c);
        LL_I2C_DisableIT_BUF(ctx->i2c);
        LL_I2C_DisableIT_EVT(ctx->i2c);
        LL_I2C_DisableIT_ERR(ctx->i2c);
        ctx->status = I2C_LL_STATUS_IDLE;
        I2C_LL_MasterRx_Callback(ctx);
      } else if (ctx->left > 3U) {
        /* 多于 3 字节：等 RXNE 读 1 字节 */
        *ctx->buf++ = LL_I2C_ReceiveData8(ctx->i2c);
        ctx->left--;
        if (ctx->left == 3U) {
          LL_I2C_DisableIT_BUF(ctx->i2c);
          ctx->status = I2C_LL_STATUS_WAIT_BTF_L3_RX;
        }
      }
    }
    break;
  case I2C_LL_STATUS_WAIT_BTF_L2_RX:
    if (LL_I2C_IsActiveFlag_BTF(ctx->i2c)) {
      /* 双字节接收 (NACK 已设置) */
      LL_I2C_GenerateStopCondition(ctx->i2c);
      *ctx->buf++ = LL_I2C_ReceiveData8(ctx->i2c);
      *ctx->buf = LL_I2C_ReceiveData8(ctx->i2c);
      LL_I2C_DisableIT_BUF(ctx->i2c);
      LL_I2C_DisableIT_EVT(ctx->i2c);
      LL_I2C_DisableIT_ERR(ctx->i2c);
      ctx->status = I2C_LL_STATUS_IDLE;
      I2C_LL_MasterRx_Callback(ctx);
    }
    break;
  case I2C_LL_STATUS_WAIT_BTF_L3_RX:
    if (LL_I2C_IsActiveFlag_BTF(ctx->i2c)) {
      /* 最后 3 字节收尾, 此时已收到 2 字节 */
      LL_I2C_AcknowledgeNextData(ctx->i2c, LL_I2C_NACK);
      *ctx->buf++ = LL_I2C_ReceiveData8(ctx->i2c);
      ctx->left--;
      ctx->status = I2C_LL_STATUS_WAIT_BTF_L2_RX;
    }
    break;
  case I2C_LL_STATUS_ERROR:
    break;
  }
}

void I2C_LL_Handle_ER_IT(I2C_Ctx *ctx) {
  I2C_LL_Error error = {0};
  /* 1. 总线错误：必须软复位 I2C 外设 */
  if (LL_I2C_IsActiveFlag_BERR(ctx->i2c)) {
    LL_I2C_ClearFlag_BERR(ctx->i2c);
    /* 软件复位 I2C1 */
    LL_I2C_EnableReset(ctx->i2c);  // SWRST = 1
    LL_I2C_DisableReset(ctx->i2c); // SWRST = 0
    error.I2C_LL_ERROR_BERR = 1;
    // 需要重新初始化
    // MX_I2C1_Init();
  }

  /* 2. 应答失败：清除标志，并发送 STOP 释放总线 */
  if (LL_I2C_IsActiveFlag_AF(ctx->i2c)) {
    LL_I2C_ClearFlag_AF(ctx->i2c);
    LL_I2C_GenerateStopCondition(ctx->i2c);
    error.I2C_LL_ERROR_AF = 1;
  }

  /* 3. 仲裁丢失 / 过载欠载：只清标志即可 */
  if (LL_I2C_IsActiveFlag_ARLO(ctx->i2c)) {
    LL_I2C_ClearFlag_ARLO(ctx->i2c);
    error.I2C_LL_ERROR_ARLO = 1;
  }

  if (LL_I2C_IsActiveFlag_OVR(ctx->i2c)) {
    LL_I2C_ClearFlag_OVR(ctx->i2c);
    error.I2C_LL_ERROR_OVR = 1;
  }

  /*
   * 4. 如果使能了 SMBus / PEC，可补充：
   *    LL_I2C_ClearFlag_PECERR(I2C1);
   *    LL_I2C_ClearFlag_TIMEOUT(I2C1);
   *    LL_I2C_ClearFlag_SMBALERT(I2C1);
   * 普通 I2C 模式下通常不会置位，可省略。
   */

  LL_I2C_DisableIT_BUF(ctx->i2c);
  LL_I2C_DisableIT_EVT(ctx->i2c);
  LL_I2C_DisableIT_ERR(ctx->i2c);

  ctx->status = I2C_LL_STATUS_ERROR;
  I2C_LL_Error_Callback(ctx, error);
}
