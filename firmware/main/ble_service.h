// firmware/main/ble_service.h
#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Event types posted to the UI queue
typedef enum {
    BLE_EVT_NOTIF_ADD,
    BLE_EVT_NOTIF_DISMISS,
    BLE_EVT_NOTIF_CLEAR,
    BLE_EVT_CONNECTED,
    BLE_EVT_DISCONNECTED,
} ble_evt_type_t;

typedef struct {
    ble_evt_type_t type;
    char id[48];
    char project[32];
    char message[64];
} ble_evt_t;

// Initialize NimBLE stack and GATT server.
// Events are posted to the provided queue.
void ble_service_init(QueueHandle_t evt_queue);

#endif // BLE_SERVICE_H
