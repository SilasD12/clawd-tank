#ifndef SIM_FREERTOS_TASK_H
#define SIM_FREERTOS_TASK_H

#include "FreeRTOS.h"

static inline void vTaskDelay(TickType_t ticks) { (void)ticks; }

#endif
