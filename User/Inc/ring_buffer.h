#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#include "stm32f1xx.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint8_t *storage;
  uint8_t size;
  volatile uint8_t head;
  volatile uint8_t tail;
} ring_buffer_t;

void ring_buffer_init(ring_buffer_t *buffer, uint8_t *storage, uint8_t size);
ErrorStatus ring_buffer_push(ring_buffer_t *buffer, uint8_t data);
ErrorStatus ring_buffer_pop(ring_buffer_t *buffer, uint8_t *data);
bool ring_buffer_is_empty(const ring_buffer_t *buffer);
bool ring_buffer_is_full(const ring_buffer_t *buffer);
uint8_t ring_buffer_get_count(const ring_buffer_t *buffer);
void ring_buffer_clear(ring_buffer_t *buffer);

#endif
