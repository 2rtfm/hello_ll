#ifndef __US_DELAY_H
#define __US_DELAY_H

#include <stdint.h>

void DWT_Init(void);
void delay_us(uint32_t us);

#endif
