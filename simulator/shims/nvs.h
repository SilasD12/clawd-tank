#ifndef NVS_H_SHIM
#define NVS_H_SHIM

#include <stdint.h>
#include <string.h>

typedef uint32_t nvs_handle_t;
#ifndef ESP_OK
#define ESP_OK 0
#endif
#ifndef ESP_ERR_NVS_NOT_FOUND
#define ESP_ERR_NVS_NOT_FOUND 0x1102
#endif

/* Simple in-memory KV for simulator config persistence */
static uint8_t  _shim_brightness = 0;
static uint16_t _shim_sleep_timeout = 0;
static int _shim_brightness_set = 0;
static int _shim_sleep_timeout_set = 0;

static inline int nvs_open(const char *ns, int mode, nvs_handle_t *h) {
    (void)ns; (void)mode; *h = 1; return 0;
}
static inline int nvs_get_u8(nvs_handle_t h, const char *key, uint8_t *v) {
    (void)h;
    if (strcmp(key, "brightness") == 0 && _shim_brightness_set) { *v = _shim_brightness; return 0; }
    return ESP_ERR_NVS_NOT_FOUND;
}
static inline int nvs_get_u16(nvs_handle_t h, const char *key, uint16_t *v) {
    (void)h;
    if (strcmp(key, "sleep_tmout") == 0 && _shim_sleep_timeout_set) { *v = _shim_sleep_timeout; return 0; }
    return ESP_ERR_NVS_NOT_FOUND;
}
static inline int nvs_set_u8(nvs_handle_t h, const char *key, uint8_t v) {
    (void)h;
    if (strcmp(key, "brightness") == 0) { _shim_brightness = v; _shim_brightness_set = 1; }
    return 0;
}
static inline int nvs_set_u16(nvs_handle_t h, const char *key, uint16_t v) {
    (void)h;
    if (strcmp(key, "sleep_tmout") == 0) { _shim_sleep_timeout = v; _shim_sleep_timeout_set = 1; }
    return 0;
}
static inline int nvs_commit(nvs_handle_t h) { (void)h; return 0; }
static inline void nvs_close(nvs_handle_t h) { (void)h; }

#endif
