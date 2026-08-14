#ifndef __AHT20_H__
#define __AHT20_H__

#define AHT20_ADDR 0x70

typedef enum {
  AHT20_STATUS_IDLE,
  AHT20_STATUS_SENDING_MESURE,
  AHT20_STATUS_SENDING_COMPLETE,
  AHT20_STATUS_READING_MESURE,
  AHT20_STATUS_READING_COMPLETE
} AHT20_Status;

void AHT20_Init(void);
void AHT20_Read(char *temp, char *hum);

void AHT20_Measure_IT();
void AHT20_Recv_IT();
void AHT20_Format_IT(char *temp, char *hum);

#endif
