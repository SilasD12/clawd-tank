// Stub for nvs.h — used in host-side unit tests only.
// The actual NVS function declarations are provided by the test file itself
// via mock functions; this header only supplies required types/macros.
#ifndef NVS_H
#define NVS_H

#include <stdint.h>

typedef int      esp_err_t;
typedef uint32_t nvs_handle_t;

#define ESP_OK                 0
#define ESP_ERR_NVS_NOT_FOUND  0x1102

// NVS open mode
#define NVS_READONLY  0
#define NVS_READWRITE 1

// Function declarations — implemented as mocks in the test file
esp_err_t nvs_open(const char *ns, int mode, nvs_handle_t *handle);
esp_err_t nvs_get_u8(nvs_handle_t h, const char *key, uint8_t *val);
esp_err_t nvs_get_u16(nvs_handle_t h, const char *key, uint16_t *val);
esp_err_t nvs_set_u8(nvs_handle_t h, const char *key, uint8_t val);
esp_err_t nvs_set_u16(nvs_handle_t h, const char *key, uint16_t val);
esp_err_t nvs_commit(nvs_handle_t h);
void      nvs_close(nvs_handle_t h);

#endif // NVS_H
