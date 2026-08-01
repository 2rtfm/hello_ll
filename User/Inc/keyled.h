#ifndef __KEYLED_H__
#define __KEYLED_H__

#include "main.h"
#include "stm32f1xx_ll_gpio.h"

#ifdef LED_Pin
#define LED_Toggle() LL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin)
#define LED_ON() LL_GPIO_SetOutputPin(LED_GPIO_Port, LED_Pin)
#define LED_OFF() LL_GPIO_ResetOutputPin(LED_GPIO_Port, LED_Pin)
#endif

#ifdef KEY_UP_Pin
int ScanKey(void);
#endif
#endif
