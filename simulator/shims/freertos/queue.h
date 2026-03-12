#ifndef SIM_FREERTOS_QUEUE_H
#define SIM_FREERTOS_QUEUE_H

#include "FreeRTOS.h"
#include <stddef.h>

/* Stub — simulator does not use FreeRTOS queues */
static inline QueueHandle_t xQueueCreate(int len, int size) { (void)len; (void)size; return NULL; }
static inline BaseType_t xQueueReceive(QueueHandle_t q, void *buf, TickType_t wait) { (void)q; (void)buf; (void)wait; return pdFALSE; }
static inline BaseType_t xQueueSend(QueueHandle_t q, const void *buf, TickType_t wait) { (void)q; (void)buf; (void)wait; return pdTRUE; }

#endif
