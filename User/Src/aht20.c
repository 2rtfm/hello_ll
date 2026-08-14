#include "aht20.h"
#include "i2c_hw.h"
#include "i2c_ll.h"
#include "stm32f103x6.h"
#include "stm32f1xx_ll_utils.h"
#include <stdint.h>

static uint8_t aht20_mesure_cmd[] = {0xAC, 0x33, 0x00};
static uint8_t aht20_buf[6];

void _AHT20_vtoa(uint32_t val, char *str);

void AHT20_Init(void) {
  uint8_t status;
  LL_mDelay(40);
  I2C_MasterReceive(I2C1, AHT20_ADDR, &status, 1);
  if (status & 0x80) {
    return;
  }
  I2C_MasterTransmit(I2C1, AHT20_ADDR, (uint8_t[]){0xBE, 0x08, 0x00}, 3);
}

void AHT20_Read(char *temp, char *hum) {
  I2C_MasterTransmit(I2C1, AHT20_ADDR, aht20_mesure_cmd, 3);
  LL_mDelay(80);
  I2C_MasterReceive(I2C1, AHT20_ADDR, aht20_buf, 6);
  uint32_t raw_data;
  uint32_t hum_data;
  int32_t temp_data;
  raw_data = aht20_buf[1] << 12 | aht20_buf[2] << 4 | aht20_buf[3] >> 4;
  hum_data = raw_data * 625 >> 16;
  _AHT20_vtoa(hum_data, hum);
  raw_data = (aht20_buf[3] & 0x0F) << 16 | aht20_buf[4] << 8 | aht20_buf[5];
  temp_data = (int32_t)(raw_data * 625 >> 15) - 5000;
  if (temp_data < 0) {
    _AHT20_vtoa(-temp_data, temp);
    *temp = '-';
  } else {
    _AHT20_vtoa(temp_data, temp);
  }
}

void AHT20_Measure_IT() {
  I2C_MasterTransmit_DMA(cI2C1, AHT20_ADDR, aht20_mesure_cmd, 3);
}

void AHT20_Recv_IT() { I2C_MasterReceive_DMA(cI2C1, AHT20_ADDR, aht20_buf, 6); }

void AHT20_Format(char *temp, char *hum) {
  uint32_t raw_data;
  uint32_t hum_data;
  int32_t temp_data;
  raw_data = aht20_buf[1] << 12 | aht20_buf[2] << 4 | aht20_buf[3] >> 4;
  hum_data = raw_data * 625 >> 16;
  _AHT20_vtoa(hum_data, hum);
  raw_data = (aht20_buf[3] & 0x0F) << 16 | aht20_buf[4] << 8 | aht20_buf[5];
  temp_data = (int32_t)(raw_data * 625 >> 15) - 5000;
  if (temp_data < 0) {
    _AHT20_vtoa(-temp_data, temp);
    *temp = '-';
  } else {
    _AHT20_vtoa(temp_data, temp);
  }
}

uint8_t *AHT20_RawData(void) { return aht20_buf; }

void _AHT20_vtoa(uint32_t val, char *str) {
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
