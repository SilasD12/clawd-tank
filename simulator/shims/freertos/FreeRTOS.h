#ifndef SIM_FREERTOS_H
#define SIM_FREERTOS_H

typedef void *QueueHandle_t;
typedef unsigned int TickType_t;
typedef int BaseType_t;

#define pdTRUE  1
#define pdFALSE 0
#define pdMS_TO_TICKS(ms) (ms)

/* ESP-IDF newlib _lock_t stubs — single-threaded simulator */
typedef int _lock_t;
#define _lock_init(lock)
#define _lock_acquire(lock)
#define _lock_release(lock)

#endif
