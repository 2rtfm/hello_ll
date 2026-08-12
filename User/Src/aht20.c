#include "aht20.h"
#include "i2c_ll.h"
#include "stm32f103x6.h"
#include "stm32f1xx_ll_utils.h"
#include <stdint.h>

void _aht20_vtoa(uint32_t val, char *str);

void aht20_init(void) {
  uint8_t status;
  LL_mDelay(40);
  I2C_LL_MasterReceive(I2C1, AHT20_ADDR, &status, 1);
  if (status & 0x80) {
    return;
  }
  I2C_LL_MasterTransmit(I2C1, AHT20_ADDR, (uint8_t[]){0xBE, 0x08, 0x00}, 3);
}

void aht20_read(char *temp, char *hum) {
  I2C_LL_MasterTransmit(I2C1, AHT20_ADDR, (uint8_t[]){0xAC, 0x33, 0x00}, 3);
  LL_mDelay(80);
  uint8_t data[6];
  I2C_LL_MasterReceive(I2C1, AHT20_ADDR, data, 6);
  uint32_t raw_data;
  uint32_t hum_data;
  int32_t temp_data;
  raw_data = data[1] << 12 | data[2] << 4 | data[3] >> 4;
  hum_data = raw_data * 625 >> 16;
  _aht20_vtoa(hum_data, hum);
  raw_data = (data[3] & 0x0F) << 16 | data[4] << 8 | data[5];
  temp_data = (int32_t)(raw_data * 625 >> 15) - 5000;
  if (temp_data < 0) {
    _aht20_vtoa(-temp_data, temp);
    *temp = '-';
  } else {
    _aht20_vtoa(temp_data, temp);
  }
}

void _aht20_vtoa(uint32_t val, char *str) {
  char *p = str + 6;
  *p-- = '\0';
  do {
    *p-- = '0' + val % 10;
    val /= 10;
    if (p - str == 3) {
      *p-- = '.';
    }
  } while (val);
  while (p >= str) {
    *p-- = ' ';
  }
}
