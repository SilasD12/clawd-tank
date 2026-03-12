#ifndef NVS_FLASH_H_SHIM
#define NVS_FLASH_H_SHIM

#include <stdint.h>

#ifndef ESP_OK
#define ESP_OK 0
#endif
typedef int esp_err_t;

static inline esp_err_t nvs_flash_init(void) { return ESP_OK; }

#endif
