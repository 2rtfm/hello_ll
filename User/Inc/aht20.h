#ifndef __AHT20_H__
#define __AHT20_H__

#include <stdint.h>

#define AHT20_ADDR 0x70

void aht20_init(void);
void aht20_read(char *temp, char *hum);

#endif
