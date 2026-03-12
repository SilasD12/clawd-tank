// firmware/main/main.c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "display.h"
#include "lvgl.h"

static const char *TAG = "clawd";

static _lock_t lvgl_lock;

static void ui_task(void *arg) {
    lv_display_t *display = (lv_display_t *)arg;
    (void)display;

    _lock_acquire(&lvgl_lock);
    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Clawd ready!");
    lv_obj_center(label);
    _lock_release(&lvgl_lock);

    while (1) {
        _lock_acquire(&lvgl_lock);
        lv_timer_handler();
        _lock_release(&lvgl_lock);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Clawd starting...");

    lv_display_t *display = display_init();

    xTaskCreate(ui_task, "ui_task", 4096, display, 5, NULL);
}
