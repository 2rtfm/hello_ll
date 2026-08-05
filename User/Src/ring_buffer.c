#include "ring_buffer.h"
#include "stm32f1xx.h"
#include <stdint.h>

void ring_buffer_init(ring_buffer_t *buffer, uint8_t *storage, uint8_t size) {
  buffer->storage = storage;
  buffer->size = size;
  buffer->head = 0;
  buffer->tail = 0;
}

ErrorStatus ring_buffer_push(ring_buffer_t *buffer, uint8_t data) {
  uint8_t next = buffer->head + 1;
  if (next == buffer->size) {
    next = 0;
  }
  if (next == buffer->tail) {
    return ERROR;
  }
  buffer->storage[buffer->head] = data;
  buffer->head = next;
  return SUCCESS;
}

ErrorStatus ring_buffer_pop(ring_buffer_t *buffer, uint8_t *data) {
  if (buffer->head == buffer->tail) {
    return ERROR;
  }
  *data = buffer->storage[buffer->tail];
  buffer->tail++;
  if (buffer->tail == buffer->size) {
    buffer->tail = 0;
  }
  return SUCCESS;
}

uint8_t ring_buffer_get_count(const ring_buffer_t *buffer) {
  return (buffer->head >= buffer->tail)
             ? (uint8_t)(buffer->head - buffer->tail)
             : (uint8_t)(buffer->head + buffer->size - buffer->tail);
}

bool ring_buffer_is_empty(const ring_buffer_t *buffer) {
  return buffer->head == buffer->tail;
}

bool ring_buffer_is_full(const ring_buffer_t *buffer) {
  uint8_t next = buffer->head + 1;
  if (next == buffer->size) {
    next = 0;
  }
  if (next == buffer->tail) {
    return true;
  }
  return false;
}

void ring_buffer_clear(ring_buffer_t *buffer) {
  buffer->head = 0;
  buffer->tail = 0;
}
