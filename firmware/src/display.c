// firmware/main/display.c
#include "display.h"
#include "config_store.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include <string.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "display";

// M5StickC Plus2 pin definitions
#define PIN_MOSI    15
#define PIN_SCLK    13
#define PIN_CS      5
#define PIN_DC      14
#define PIN_RST     12
#define PIN_BL      27
#define PIN_POWER_HOLD 4

// Display config
#define LCD_HOST            SPI3_HOST
#define LCD_SPI_QUEUE_DEPTH 10
#define LCD_PIXEL_CLK   (8 * 1000 * 1000)  // 8 MHz - ultra safe for testing
#define LCD_H_RES       240   // landscape width
#define LCD_V_RES       135   // landscape height
#define LCD_CMD_BITS    8
#define LCD_PARAM_BITS  8
#define LVGL_BUF_LINES  20
#define LVGL_TICK_MS    2

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io,
                                     esp_lcd_panel_io_event_data_t *edata,
                                     void *user_ctx) {
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area,
                           uint8_t *px_map) {
    esp_lcd_panel_handle_t panel = lv_display_get_user_data(disp);

    int w = area->x2 - area->x1 + 1;
    int h = area->y2 - area->y1 + 1;
    lv_draw_sw_rgb565_swap(px_map, w * h);

    esp_lcd_panel_draw_bitmap(panel,
        area->x1, area->y1, area->x2 + 1, area->y2 + 1, px_map);
}

static void lvgl_tick_cb(void *arg) {
    lv_tick_inc(LVGL_TICK_MS);
}

lv_display_t *display_init(void) {
    ESP_LOGI(TAG, "Initializing display...");

    // 1. Backlight — set to normal GPIO HIGH for MAX BRIGHTNESS
    gpio_reset_pin(PIN_BL);
    gpio_set_direction(PIN_BL, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_BL, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    // 2. Hardware Reset
    gpio_reset_pin(PIN_RST);
    gpio_set_direction(PIN_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    // 3. SPI bus
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_SCLK,
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * LVGL_BUF_LINES * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // 4. Panel I/O
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_DC,
        .cs_gpio_num = PIN_CS,
        .pclk_hz = LCD_PIXEL_CLK,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = LCD_SPI_QUEUE_DEPTH,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    // 5. ST7789 panel
    esp_lcd_panel_handle_t panel = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = -1, // manual reset
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel, true));

    // Landscape: swap X/Y, then mirror as needed
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel, true, false));
    // Correct offsets for M5StickC Plus2 135x240
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel, 40, 52));

    // 6. CLEAR SCREEN TO WHITE (FORCE VISIBILITY)
    {
        size_t clear_sz = LCD_H_RES * LVGL_BUF_LINES * sizeof(uint16_t);
        void *clear_buf = heap_caps_calloc(1, clear_sz, MALLOC_CAP_DMA);
        configASSERT(clear_buf);
        memset(clear_buf, 0xFF, clear_sz); // WHITE
        for (int y = 0; y < LCD_V_RES; y += LVGL_BUF_LINES) {
            int h = (y + LVGL_BUF_LINES <= LCD_V_RES) ? LVGL_BUF_LINES : (LCD_V_RES - y);
            esp_lcd_panel_draw_bitmap(panel, 0, y, LCD_H_RES, y + h, clear_buf);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
        free(clear_buf);
    }

    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));
    
    // 7. Initialize LEDC (PWM) at MAX for testing
    ledc_timer_config_t bl_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 10000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&bl_timer));
    ledc_channel_config_t bl_channel = {
        .gpio_num = PIN_BL,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 255, // FORCED MAX
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&bl_channel));

    // 8. LVGL init
    lv_init();

    lv_display_t *display = lv_display_create(LCD_H_RES, LCD_V_RES);
    if (!display) {
        ESP_LOGE(TAG, "lv_display_create failed");
        abort();
    }

    size_t buf_sz = LCD_H_RES * LVGL_BUF_LINES * sizeof(lv_color16_t);
    void *buf1 = heap_caps_malloc(buf_sz, MALLOC_CAP_DMA);
    void *buf2 = heap_caps_malloc(buf_sz, MALLOC_CAP_DMA);
    configASSERT(buf1 && buf2);

    lv_display_set_buffers(display, buf1, buf2, buf_sz,
                            LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_user_data(display, panel);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(display, lvgl_flush_cb);

    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = notify_lvgl_flush_ready,
    };
    ESP_ERROR_CHECK(esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, display));

    const esp_timer_create_args_t tick_args = {
        .callback = &lvgl_tick_cb,
        .name = "lvgl_tick",
    };
    esp_timer_handle_t tick_timer;
    ESP_ERROR_CHECK(esp_timer_create(&tick_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, LVGL_TICK_MS * 1000));

    ESP_LOGI(TAG, "Display initialized: %dx%d landscape (TEST MODE WHITE)", LCD_H_RES, LCD_V_RES);
    return display;
}

void display_set_brightness(uint8_t duty)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ESP_LOGI(TAG, "Brightness set to %u", duty);
}
