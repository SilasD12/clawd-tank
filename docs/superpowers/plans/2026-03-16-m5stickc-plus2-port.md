# M5StickC Plus2 Display Port Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Port all display and layout code from the Waveshare ESP32-C6-LCD-1.47 (320x172 ST7789) to the M5StickC Plus2 (240x135 ST7789V2, landscape orientation).

**Architecture:** This is a constants-and-configuration port. The display resolution changes from 320x172 to 240x135 (landscape). All hardcoded dimensions, pin assignments, layout positions, and SPI configuration must be updated. The SoC changes from ESP32-C6 (RISC-V) to ESP32 (Xtensa dual-core with PSRAM). Sprites remain at their current pixel dimensions (they are pixel art, not resolution-dependent) but some may clip on the shorter 135px display — this is acceptable for now and can be addressed later with the sprite pipeline.

**Tech Stack:** ESP-IDF 5.3+, LVGL 9.2+, ST7789V2 SPI display, ESP32

**Scale factors:**
- Width: 240/320 = 0.75
- Height: 135/172 = 0.785

**M5StickC Plus2 pin reference:**
| Function | GPIO |
|----------|------|
| TFT_MOSI | 15 |
| TFT_SCLK | 13 |
| TFT_CS | 5 |
| TFT_DC | 14 |
| TFT_RST | 12 |
| TFT_BL | 27 |
| Button A (front) | 37 |
| Button B (side) | 39 |
| Internal LED | 19 |
| IR LED | 9 |
| Buzzer | 2 |
| I2C SDA | 21 |
| I2C SCL | 22 |

**Note:** The M5StickC Plus2 has NO WS2812B RGB LED. It has a simple internal LED on GPIO19. The RGB LED flash-on-notification feature will need to be adapted (use the internal LED or buzzer instead, or simply disable the RGB effect).

---

## Chunk 1: Display Driver and Build Configuration

### Task 1: Update display.c — Pin definitions and resolution

**Files:**
- Modify: `firmware/main/display.c:17-33`

- [ ] **Step 1: Update pin definitions**

Change lines 17-23 from Waveshare pins to M5StickC Plus2 pins:

```c
// M5StickC Plus2 pin definitions
#define PIN_MOSI    15
#define PIN_SCLK    13
#define PIN_CS      5
#define PIN_DC      14
#define PIN_RST     12
#define PIN_BL      27
```

- [ ] **Step 2: Update display resolution constants**

Change lines 28-29:

```c
#define LCD_H_RES       240   // landscape width
#define LCD_V_RES       135   // landscape height
```

- [ ] **Step 3: Update SPI host and MISO pin**

The M5StickC Plus2 display is on VSPI (`SPI3_HOST`), not HSPI (`SPI2_HOST`). Change line 26:

```c
#define LCD_HOST        SPI3_HOST
```

In `display_init()`, line 85, the MISO pin is set to GPIO 5 which conflicts with TFT_CS on the M5StickC Plus2. Change to -1:

```c
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_SCLK,
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * LVGL_BUF_LINES * sizeof(uint16_t),
    };
```

**Note:** DMA buffer sizes auto-adjust since they reference `LCD_H_RES` — changing from 320 to 240 reduces each DMA buffer from 12,800 to 9,600 bytes. These use `MALLOC_CAP_DMA` (internal SRAM only, even with PSRAM enabled).

- [ ] **Step 4: Update ST7789 panel gap offset and mirror settings**

The ST7789 controller RAM is 240x320. For the M5StickC Plus2's 135x240 panel in landscape (swap_xy=true):
- The 135-pixel physical dimension needs a gap offset: (240 - 135) / 2 = 52 (approximately — may need tuning)
- The 240-pixel dimension also needs an offset: (320 - 240) / 2 = 40

Change lines 118-123:

```c
    // Landscape: swap X/Y, then mirror as needed
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel, true, false));
    // Apply offset for M5StickC Plus2 135x240 panel in landscape
    // With swap_xy=true, CASET addresses rows and RASET addresses columns.
    // x_gap=40 (320-240=80, offset to center), y_gap=52 (240-135=105, but panel starts at row 52)
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel, 40, 52));
```

**Important — hardware verification required:** The gap values and mirror/color settings WILL likely need tuning on the actual M5StickC Plus2. After flashing, test these combinations if the display looks wrong:

| Symptom | Fix to try |
|---------|-----------|
| Image offset/shifted | `esp_lcd_panel_set_gap(panel, 40, 53)` — some units need 53 not 52 |
| Image horizontally flipped | `esp_lcd_panel_mirror(panel, false, false)` or `(false, true)` |
| Image vertically flipped | `esp_lcd_panel_mirror(panel, true, true)` |
| Colors look wrong (red↔blue) | Change `LCD_RGB_ELEMENT_ORDER_RGB` to `LCD_RGB_ELEMENT_ORDER_BGR` |
| Blank/white screen | Verify SPI3_HOST is correct, check pin wiring |

The current Waveshare code uses `mirror(true, false)` with `gap(0, 34)`. The M5StickC Plus2 commonly needs `mirror(true, false)` with `gap(40, 52)`, but this varies by hardware revision.

- [ ] **Step 5: Update SPI clock speed**

The ESP32 can drive SPI faster than the ESP32-C6. Optionally increase from 12 MHz to 20 MHz for smoother rendering:

```c
#define LCD_PIXEL_CLK   (20 * 1000 * 1000)  // 20 MHz (M5StickC Plus2 ST7789V2)
```

If there are display artifacts, reduce back to `(12 * 1000 * 1000)`.

- [ ] **Step 6: Commit**

```bash
git add firmware/main/display.c
git commit -m "feat: port display driver to M5StickC Plus2 (240x135 ST7789V2)"
```

---

### Task 2: Update sdkconfig.defaults for ESP32

**Files:**
- Modify: `firmware/sdkconfig.defaults`

- [ ] **Step 1: Update target and flash configuration**

The ESP32-PICO-V3-02 in the M5StickC Plus2 has 8MB flash and 2MB PSRAM. Replace the full file contents with:

```ini
# firmware/sdkconfig.defaults

# BLE (NimBLE)
CONFIG_BT_ENABLED=y
CONFIG_BT_NIMBLE_ENABLED=y
CONFIG_BT_NIMBLE_ROLE_PERIPHERAL=y
CONFIG_BT_NIMBLE_ROLE_CENTRAL=n
CONFIG_BT_NIMBLE_ROLE_OBSERVER=n
CONFIG_BT_NIMBLE_ROLE_BROADCASTER=n
CONFIG_BT_NIMBLE_ATT_PREFERRED_MTU=256

# SPI LCD
CONFIG_LCD_HOST_SPI2=y

# Partition table
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"

# Flash (ESP32-PICO-V3-02 has 8MB flash)
CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y
CONFIG_ESPTOOLPY_FLASHSIZE="8MB"

# PSRAM (ESP32-PICO-V3-02 has 2MB PSRAM)
CONFIG_ESP32_SPIRAM_SUPPORT=y
CONFIG_SPIRAM=y
CONFIG_SPIRAM_USE_MALLOC=y

# LVGL
CONFIG_LV_COLOR_DEPTH_16=y
CONFIG_LV_FONT_MONTSERRAT_8=y
CONFIG_LV_FONT_MONTSERRAT_10=y
CONFIG_LV_FONT_MONTSERRAT_12=y
CONFIG_LV_FONT_MONTSERRAT_14=y
CONFIG_LV_FONT_MONTSERRAT_18=y

# FreeRTOS
CONFIG_FREERTOS_HZ=1000
```

- [ ] **Step 2: Set the IDF target to ESP32**

The project must be configured for ESP32 (not ESP32-C6). Run:

```bash
cd firmware && idf.py set-target esp32
```

This regenerates `sdkconfig` from `sdkconfig.defaults`. After this, do a full clean build:

```bash
cd firmware && idf.py fullclean && idf.py build
```

- [ ] **Step 3: Commit**

```bash
git add firmware/sdkconfig.defaults
git commit -m "chore: update sdkconfig.defaults for ESP32 (M5StickC Plus2)"
```

---

### Task 3: Adapt RGB LED for M5StickC Plus2

**Files:**
- Modify: `firmware/main/rgb_led.c:10`

The M5StickC Plus2 has no WS2812B. Two options:

**Option A (recommended — simplest):** Change the GPIO to the internal LED (GPIO19) and use simple on/off instead of RGB. This requires replacing the `led_strip` driver with basic GPIO toggle.

**Option B (minimal):** Just change the GPIO and accept that the LED strip driver will fail silently (if the hardware isn't present, the RMT init may error or do nothing).

- [ ] **Step 1: Update GPIO and disable RGB LED gracefully**

Replace the RGB LED GPIO and add graceful failure handling. Change line 10:

```c
#define RGB_LED_GPIO    19   // M5StickC Plus2 internal LED (not RGB — on/off only)
```

Then in `rgb_led_init()`, wrap the `led_strip_new_rmt_device` call so that if it fails, the LED feature is simply disabled (s_strip stays NULL, all subsequent operations become no-ops since the existing code already null-checks s_strip).

Alternatively, if you want a visible notification indicator, replace the LED strip driver entirely with a simple GPIO blink on GPIO19. But this is cosmetic and can be deferred.

- [ ] **Step 2: Commit**

```bash
git add firmware/main/rgb_led.c
git commit -m "chore: update RGB LED GPIO for M5StickC Plus2 (GPIO19)"
```

---

## Chunk 2: Scene Layout Rescaling

### Task 4: Update scene.c constants and layout

**Files:**
- Modify: `firmware/main/scene.c:38-42` (scene constants)
- Modify: `firmware/main/scene.c:244-255` (star positions)
- Modify: `firmware/main/scene.c:484-494` (grass tufts)
- Modify: `firmware/main/scene.c:527-537` (HUD canvas sizes)
- Modify: `firmware/main/scene.c:549-554` (multi-session x_centers)
- Modify: `firmware/main/scene.c:567` (narrow threshold)
- Modify: `firmware/main/scene.c:1113` (x_centers center reference)
- Modify: `firmware/main/scene.c:1192` (walk-in start offset)

- [ ] **Step 1: Update scene dimension constants**

Change lines 38-42:

```c
#define SCENE_HEIGHT       135
#define GRASS_HEIGHT       11
#define STAR_COUNT         6
#define STAR_TWINKLE_MIN   2000
#define STAR_TWINKLE_MAX   4000
```

GRASS_HEIGHT: 14 * 0.785 ≈ 11

- [ ] **Step 2: Rescale star positions**

The stars are positioned with absolute x,y coordinates in a 320x172 space. Scale to 240x135. Change the `star_cfg` array (around line 244):

```c
static const struct {
    int x, y, size;
    lv_color_t color;
} star_cfg[STAR_COUNT] = {
    {  8,  6, 2, {.red = 0xFF, .green = 0xFF, .blue = 0x88} },
    { 34, 12, 3, {.red = 0x88, .green = 0xCC, .blue = 0xFF} },
    { 60, 17, 2, {.red = 0xFF, .green = 0xAA, .blue = 0x88} },
    { 90,  4, 3, {.red = 0xAA, .green = 0xCC, .blue = 0xFF} },
    {112, 14, 2, {.red = 0xFF, .green = 0xDD, .blue = 0x88} },
    {120, 24, 3, {.red = 0x88, .green = 0xFF, .blue = 0xCC} },
};
```

Scaling: x * 0.75, y * 0.785 (rounded to int).

- [ ] **Step 3: Rescale grass tufts**

Change the tufts array (around line 484):

```c
    static const struct { int x; int w; } tufts[] = {
        {6, 2}, {19, 2}, {38, 3}, {59, 2}, {75, 2}, {98, 2}, {116, 2},
    };
```

Scaling: x * 0.75 (rounded), w * 0.75 (min 2).

- [ ] **Step 4: Rescale multi-session X center positions**

The `x_centers` table positions sprites across the full 320px width. Scale to 240px. The center of a 240px display is 120 (not 160). Change `x_centers` (around line 549):

```c
static const int x_centers[][4] = {
    {120},              /* 1 session: center */
    { 80, 160},         /* 2 sessions */
    { 60, 120, 180},    /* 3 sessions */
    { 48,  96, 144, 192} /* 4 sessions */
};
```

- [ ] **Step 5: Update center reference constant**

Throughout `scene.c`, the value `160` is used as the center of the 320px display for computing x_off from x_centers. This needs to change to `120` (center of 240px). Search for all `- 160` occurrences and replace with `- 120`:

Affected locations:
- Line ~639: `int target_x_off = (cnt >= 2) ? x_centers[cnt - 1][i] - 160 : 0;`
- Line ~800: `int target = (cnt >= 2) ? x_centers[cnt - 1][r] - 160 : 0;`
- Line ~1113: `int x_off = x_centers[count - 1][new_i] - 160;`

Change ALL instances of `- 160` in x_centers computations to `- 120`.

**Recommended approach:** Define a constant for readability:

```c
#define SCENE_CENTER_X  120  /* half of LCD_H_RES (240) */
```

Then replace `- 160` with `- SCENE_CENTER_X` everywhere.

- [ ] **Step 6: Update narrow mode threshold**

Line ~567: `scene->narrow = (width_px < 320);` — change to:

```c
scene->narrow = (width_px < 240);
```

- [ ] **Step 7: Update walk-in start X offset**

Line ~1192: `int start_x_off = 250;` — this was "well past right edge from center" on 320px. For 240px display, reduce to 180:

```c
int start_x_off = 180;  /* well past right edge from center */
```

- [ ] **Step 8: Update stale comments referencing 320px**

Several comments reference the old 320px display width. Update them:

Line ~1110 (comment above multi-session assignment loop):
```c
 * assuming 240px width; convert to offsets from center (120). */
```

Line ~1183-1184 (walk-in comment):
```c
 * "180px right of center" (off-screen on a 240px display). */
```

- [ ] **Step 9: Rescale HUD canvas sizes**

Around lines 527-537, the HUD canvases are sized for the 320px display. Scale down:

```c
    /* HUD: subagent counter canvas (top-left) — sized for 2x mini-crab (20x14) + text */
    s->hud_canvas = lv_canvas_create(s->container);
    static uint8_t hud_buf[60 * 14 * 4];  /* was 80*16*4, scaled down */
    lv_canvas_set_buffer(s->hud_canvas, hud_buf, 60, 14, LV_COLOR_FORMAT_ARGB8888);
    lv_obj_align(s->hud_canvas, LV_ALIGN_TOP_LEFT, 3, 2);
    lv_obj_add_flag(s->hud_canvas, LV_OBJ_FLAG_HIDDEN);

    /* HUD: overflow/total badge canvas (top-right of SCREEN, not container) */
    s->hud_badge_canvas = lv_canvas_create(lv_screen_active());
    static uint8_t badge_buf[36 * 10 * 4];  /* was 48*12*4, scaled down */
    lv_canvas_set_buffer(s->hud_badge_canvas, badge_buf, 36, 10, LV_COLOR_FORMAT_ARGB8888);
    lv_obj_align(s->hud_badge_canvas, LV_ALIGN_TOP_RIGHT, -1, 3);
    lv_obj_add_flag(s->hud_badge_canvas, LV_OBJ_FLAG_HIDDEN);
```

- [ ] **Step 10: Commit**

```bash
git add firmware/main/scene.c
git commit -m "feat: rescale scene layout for 240x135 display"
```

---

## Chunk 3: UI Manager and Notification Panel Rescaling

### Task 5: Update ui_manager.c width constants

**Files:**
- Modify: `firmware/main/ui_manager.c:24-26`

- [ ] **Step 1: Update scene width constants**

Change lines 24-26:

```c
#define SCENE_FULL_WIDTH   240
#define SCENE_NOTIF_WIDTH   80
#define SCENE_ANIM_MS      400
```

SCENE_NOTIF_WIDTH: 107 * 0.75 ≈ 80 (the scene panel width when notification panel is visible).

- [ ] **Step 2: Commit**

```bash
git add firmware/main/ui_manager.c
git commit -m "feat: update UI manager width constants for 240px display"
```

---

### Task 6: Update notification_ui.c layout constants

**Files:**
- Modify: `firmware/main/notification_ui.c:7-13` (screen/layout constants)
- Modify: `firmware/main/notification_ui.c:82-83` (container size/position)
- Modify: `firmware/main/notification_ui.c:94` (counter font)
- Modify: `firmware/main/notification_ui.c:107-109` (featured card padding/size)
- Modify: `firmware/main/notification_ui.c:115` (project name font)
- Modify: `firmware/main/notification_ui.c:123-125` (message position/font)
- Modify: `firmware/main/notification_ui.c:131-133` (badge position/font)
- Modify: `firmware/main/notification_ui.c:384` (hero badge position)
- Modify: `firmware/main/notification_ui.c:422` (normal badge position)
- Modify: `firmware/main/notification_ui.c:478` (compact label font)

- [ ] **Step 1: Update screen and layout constants**

Change lines 7-13:

```c
#define SCREEN_W              240
#define SCREEN_H              135
#define COUNTER_H             16
#define FEATURED_H            50
#define FEATURED_H_EXPANDED   (SCREEN_H - COUNTER_H - 6)  /* fills panel on new notif */
#define COMPACT_ROW_H         11
#define DOT_SIZE              5
```

Scaling rationale:
- COUNTER_H: 20 * 0.785 ≈ 16
- FEATURED_H: 64 * 0.785 ≈ 50
- FEATURED_H_EXPANDED: auto-computed from SCREEN_H (was 144, now 113)
- COMPACT_ROW_H: 14 * 0.785 ≈ 11
- DOT_SIZE: 6 * 0.785 ≈ 5

- [ ] **Step 2: Update container dimensions**

Line 82-83: Update the container size and position to match new SCENE_NOTIF_WIDTH (80). Add a named constant to avoid magic numbers:

Add after DOT_SIZE define:
```c
#define NOTIF_PANEL_X        80   /* must match SCENE_NOTIF_WIDTH in ui_manager.c */
```

Then update lines 82-83:
```c
    lv_obj_set_size(ui->container, SCREEN_W - NOTIF_PANEL_X, SCREEN_H);
    lv_obj_set_pos(ui->container, NOTIF_PANEL_X, 0);
```

- [ ] **Step 3: Downsize fonts for the smaller display**

The notification panel is now 160px wide (was 213px). Use smaller fonts to fit:

Line ~94 (counter label font): Change `montserrat_14` to `montserrat_12`:
```c
    lv_obj_set_style_text_font(ui->counter_label, &lv_font_montserrat_12, 0);
```

Line ~115 (featured project font): Change `montserrat_14` to `montserrat_12`:
```c
    lv_obj_set_style_text_font(ui->featured_project, &lv_font_montserrat_12, 0);
```

Line ~123 (featured message font): Change `montserrat_12` to `montserrat_10`:
```c
    lv_obj_set_style_text_font(ui->featured_message, &lv_font_montserrat_10, 0);
```

Line ~131 (featured badge font): Keep `montserrat_10` (already small).

Line ~478 (compact row label font): Change `montserrat_12` to `montserrat_10`:
```c
    lv_obj_set_style_text_font(label, &lv_font_montserrat_10, 0);
```

- [ ] **Step 4: Adjust featured card internal layout**

The featured card's internal element positions need scaling:

Line ~107 (card padding): Reduce from 6 to 4:
```c
    lv_obj_set_style_pad_all(ui->featured_card, 4, 0);
```

Line ~125 (message y position): Change from 18 to 14:
```c
    lv_obj_set_pos(ui->featured_message, 0, 14);
```

Line ~133 (badge y position): Change from 38 to 30:
```c
    lv_obj_set_pos(ui->featured_badge, 0, 30);
```

- [ ] **Step 5: Adjust hero badge and normal badge Y positions in rebuild_display()**

Line ~384 (hero badge y in expanded view): Change `FEATURED_H_EXPANDED - 24` to `FEATURED_H_EXPANDED - 20`:
```c
    lv_obj_set_pos(ui->featured_badge, 0, FEATURED_H_EXPANDED - 20);
```

Line ~422 (normal badge y): Change from 38 to 30:
```c
    lv_obj_set_pos(ui->featured_badge, 0, 30);
```

- [ ] **Step 6: Update compact list Y start**

Line ~446: The compact list Y start computation already uses the constants, so it auto-adjusts:
```c
    int y_pos = COUNTER_H + FEATURED_H + 6;
```
This becomes 16 + 50 + 6 = 72, leaving (135 - 72) / 11 = 5.7 → 5 compact rows visible. Previously it was (172 - 90) / 14 = 5.8 → 5. Same effective count — good.

- [ ] **Step 7: Commit**

```bash
git add firmware/main/notification_ui.c
git commit -m "feat: rescale notification panel layout for 240x135 display"
```

---

## Chunk 4: Simulator Updates

### Task 7: Update simulator display constants

**Files:**
- Modify: `simulator/sim_display.h:8-9`
- Modify: `simulator/sim_display.c:15` (framebuffer array)

- [ ] **Step 1: Update resolution defines**

Change `simulator/sim_display.h` lines 8-9:

```c
#define SIM_LCD_H_RES 240
#define SIM_LCD_V_RES 135
```

Also update the comment on line 20:
```c
/** Get pointer to the raw RGB565 framebuffer (240*135 uint16_t). */
```

- [ ] **Step 2: Update framebuffer comment in sim_screenshot.h**

In `simulator/sim_screenshot.h` lines 11-13, update the comments:

```c
 * @param framebuffer  Pointer to 240*135 uint16_t array
 * @param w            Width (240)
 * @param h            Height (135)
```

- [ ] **Step 3: Commit**

```bash
git add simulator/sim_display.h simulator/sim_display.c simulator/sim_screenshot.h
git commit -m "feat: update simulator display resolution to 240x135"
```

---

### Task 8: Update scene.c time label font for smaller display

**Files:**
- Modify: `firmware/main/scene.c:511` (time label font)

- [ ] **Step 1: Downsize time label font**

The time label uses `montserrat_18` which is quite large for a 135px tall display. Change to `montserrat_14`:

```c
    lv_obj_set_style_text_font(s->time_label, &lv_font_montserrat_14, 0);
```

- [ ] **Step 2: Commit**

```bash
git add firmware/main/scene.c
git commit -m "chore: reduce time label font size for smaller display"
```

---

## Chunk 5: Documentation and Sprite Notes

### Task 9: Update CLAUDE.md

**Files:**
- Modify: `CLAUDE.md`

- [ ] **Step 1: Update hardware references**

Find and replace all references to:
- "Waveshare ESP32-C6-LCD-1.47" → "M5StickC Plus2"
- "320x172" → "240x135" (in display context, not in code references)
- "ESP32-C6" → "ESP32" / "ESP32-PICO-V3-02"
- "ESP32-C6FH8" → "ESP32-PICO-V3-02"
- "8MB embedded flash" → "8MB flash, 2MB PSRAM"

Update the Key Constraints section:
```
- **Display**: 240x135 pixels, 16-bit RGB565, SPI. All UI must fit this resolution.
- **Target chip**: ESP32-PICO-V3-02 (Xtensa dual-core). 8MB flash, 2MB PSRAM.
```

Update the flash command port:
```bash
cd firmware && idf.py -p /dev/ttyACM0 flash monitor
```
(Port may differ on M5StickC Plus2 — typically `/dev/ttyUSB0` on Linux.)

- [ ] **Step 2: Commit**

```bash
git add CLAUDE.md
git commit -m "docs: update CLAUDE.md for M5StickC Plus2 hardware"
```

---

### Task 10: Sprite overflow audit (informational — no code changes)

**No code changes in this task.** This documents which sprites clip on the 135px display for future reference.

**Sprite fit analysis on 240x135 with GRASS_HEIGHT=11:**

The scene container is 135px tall. Sprites use `LV_ALIGN_BOTTOM_MID` with a negative y_offset. The sprite top edge = 135 + y_offset - sprite_height. If this is negative, the top rows clip.

| Sprite | WxH | y_offset | Top edge | Clips? |
|--------|-----|----------|----------|--------|
| idle | 72x51 | -8 | 76 | No |
| alert | 120x98 | -4 | 33 | No |
| happy | 124x89 | -7 | 39 | No |
| sleeping | 92x96 | -8 | 31 | No |
| **disconnected** | **182x137** | **-6** | **-8** | **Yes (8px)** |
| thinking | 76x92 | -8 | 35 | No |
| typing | 86x78 | -8 | 49 | No |
| juggling | 68x66 | -8 | 61 | No |
| building | 124x75 | -4 | 56 | No |
| **confused** | **152x113** | **-4** | **18** | No (tight) |
| sweeping | 138x56 | 0 | 79 | No |
| walking | 60x40 | -8 | 87 | No |
| going_away | 170x61 | -6 | 68 | No |
| mini_crab | 10x7 | -5 | 123 | No |

**Only `disconnected` clips** (8 pixels from top). This is the "no connection" state sprite which is very large. The clipping affects the top of the sprite — likely just the antenna/ears. This is acceptable for now. If it looks bad, regenerate the sprite at a smaller scale using:

```bash
python tools/svg2frames.py assets/disconnected.svg frames/disconnected/ --fps 6 --scale 3
python tools/png2rgb565.py frames/disconnected/ firmware/main/assets/sprite_disconnected.h --name disconnected
python tools/crop_sprites.py
```

(Use `--scale 3` instead of `--scale 4` to produce ~75% sized frames.)

**Width concerns:** The `disconnected` sprite at 182px fits within 240px width. The `going_away` at 170px also fits. No width clipping issues.

---

## Summary of all changes

| File | Change |
|------|--------|
| `firmware/main/display.c` | Pins (6→15, 7→13, 14→5, 15→14, 21→12, 22→27), resolution (320x172→240x135), SPI host SPI2→SPI3, gap (0,34→40,52), MISO=-1, SPI clock optionally 20MHz |
| `firmware/sdkconfig.defaults` | Add PSRAM config, target ESP32 |
| `firmware/main/rgb_led.c` | GPIO 8→19 |
| `firmware/main/scene.c` | SCENE_HEIGHT=135, GRASS_HEIGHT=11, star positions scaled, grass tufts scaled, x_centers for 240px, center ref 160→120, HUD canvas sizes, narrow threshold 320→240, walk-in offset 250→180, time font 18→14 |
| `firmware/main/ui_manager.c` | SCENE_FULL_WIDTH=240, SCENE_NOTIF_WIDTH=80 |
| `firmware/main/notification_ui.c` | SCREEN_W=240, SCREEN_H=135, COUNTER_H=16, FEATURED_H=50, COMPACT_ROW_H=11, DOT_SIZE=5, container pos/size, fonts downsized, internal layout positions |
| `simulator/sim_display.h` | SIM_LCD_H_RES=240, SIM_LCD_V_RES=135 |
| `simulator/sim_screenshot.h` | Comment updates |
| `CLAUDE.md` | Hardware references updated |
