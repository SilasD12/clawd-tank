# Clawd Display Simulator — Design Spec

## Goal

A native macOS binary that runs the Clawd LVGL UI without hardware, enabling:
1. **Visual iteration** — see UI changes without flashing the ESP32-C6
2. **Agent-driven development** — Claude agents build, run, screenshot, inspect, and iterate in a loop

## Architecture

The simulator compiles the existing firmware rendering code (`scene.c`, `notification_ui.c`, `ui_manager.c`, `notification.c`, assets) natively on macOS against a different backend. No changes to firmware source files.

### What gets replaced

| Firmware file | Simulator replacement | Why |
|---|---|---|
| `display.c` | `sim_display.c` | SPI + ST7789 → SDL2 window or headless framebuffer |
| `ble_service.c` | `sim_events.c` | BLE GATT → keyboard, inline events, scenario JSON |
| `main.c` | `sim_main.c` | FreeRTOS task → native main loop |

### What compiles unmodified from `firmware/main/`

- `scene.c` — scene renderer (sky, stars, grass, Clawd sprite animations)
- `notification_ui.c` — notification panel (featured card + compact list)
- `ui_manager.c` — state machine orchestrating scene + notifications
- `notification.c` — plain C data store
- All asset headers (`assets/*.h` — `const uint16_t[]` sprite frame arrays)

## Directory Structure

```
simulator/
  CMakeLists.txt              # Native CMake build (not ESP-IDF)
  sim_main.c                  # Native entry point
  sim_display.c               # SDL2 + headless display backend
  sim_display.h
  sim_events.c                # Event injection
  sim_events.h
  sim_screenshot.c            # PNG capture from LVGL framebuffer
  sim_screenshot.h
  lv_conf.h                   # LVGL config for native target
  stb_image_write.h           # Single-header PNG writer (public domain)
  cjson/                      # cJSON vendored (single .c/.h, MIT)
    cJSON.c
    cJSON.h
  shims/                      # Shadow headers for ESP-IDF includes
    esp_log.h
    sys/
      lock.h
    freertos/
      FreeRTOS.h
      queue.h
    ble_service.h             # Re-exports event types without BLE deps
  scenarios/
    demo.json                 # Example event timeline
```

## Display Backend (`sim_display.c`)

Two modes behind the same API:

```c
lv_display_t *sim_display_init(bool headless, int scale);
uint16_t     *sim_display_get_framebuffer(void);
void          sim_display_tick(void);
void          sim_display_shutdown(void);
```

### Interactive mode (default)

- SDL2 window at 320x172 rendered at **3x scale** (960x516 window)
- LVGL flush callback copies RGB565 framebuffer to SDL texture
- SDL event loop pumps keyboard input
- Main loop runs `lv_timer_handler()` at ~30fps (33ms interval)

### Headless mode (`--headless`)

- No SDL window, no display server required
- LVGL renders to a plain memory buffer
- Flush callback memcpys into a framebuffer array
- LVGL tick driven by `gettimeofday()` instead of real-time — simulated time advances per frame

## Event Injection (`sim_events.c`)

### Interactive mode — keyboard shortcuts

| Key | Event |
|-----|-------|
| `c` | BLE connect |
| `d` | BLE disconnect |
| `n` | Add sample notification (cycles through built-in list) |
| `1`–`8` | Dismiss notification by index |
| `x` | Clear all notifications |
| `s` | Save screenshot to `./screenshot.png` |

### Headless mode — two input methods

**Inline events (`--events`):**
```bash
./clawd-sim --headless \
  --events 'connect; wait 500; notify "GitHub" "PR #42 merged"; wait 2000; disconnect' \
  --screenshot-interval 100 --screenshot-dir ./shots/
```

Event syntax (semicolon-separated):
- `connect` — BLE connected event
- `disconnect` — BLE disconnected event
- `notify "project" "message"` — add notification
- `dismiss <index>` — dismiss notification by index (0-based)
- `clear` — clear all notifications
- `wait <ms>` — advance simulated time by N milliseconds

**Scenario file (`--scenario`):**
```json
[
  {"time_ms": 0, "event": "connect"},
  {"time_ms": 500, "event": "notify", "project": "GitHub", "message": "PR #42 merged"},
  {"time_ms": 1500, "event": "notify", "project": "Slack", "message": "Deploy complete"},
  {"time_ms": 4000, "event": "dismiss", "index": 0},
  {"time_ms": 6000, "event": "disconnect"}
]
```

Events are converted to `ble_evt_t` structs and fed into `ui_manager_handle_event()` — same path as real firmware.

## Screenshot Capture (`sim_screenshot.c`)

```c
void sim_screenshot_init(const char *output_dir);
void sim_screenshot_capture(uint16_t *framebuffer, int w, int h, uint32_t time_ms);
```

- Reads LVGL's RGB565 framebuffer, converts to RGB888, writes PNG via `stb_image_write.h`
- Filenames: `frame_000000.png`, `frame_000100.png`, etc. (zero-padded ms timestamp)
- Event-triggered screenshots named: `event_000500_connect.png`

### CLI flags

| Flag | Behavior |
|------|----------|
| `--screenshot-interval <ms>` | Save a PNG every N ms of simulated time |
| `--screenshot-on-event` | Save immediately after each event fires |
| `--screenshot-dir <path>` | Output directory (created if missing) |

Both flags can combine.

## ESP-IDF Shims

### `shims/esp_log.h`
```c
#define ESP_LOGI(tag, fmt, ...) printf("[I][%s] " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) printf("[W][%s] " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGE(tag, fmt, ...) printf("[E][%s] " fmt "\n", tag, ##__VA_ARGS__)
```

### `shims/sys/lock.h`
```c
typedef int _lock_t;
#define _lock_init(lock)
#define _lock_acquire(lock)
#define _lock_release(lock)
```
No-op stubs — simulator is single-threaded.

### `shims/freertos/FreeRTOS.h` + `shims/freertos/queue.h`
```c
typedef void *QueueHandle_t;
typedef unsigned int TickType_t;
#define pdMS_TO_TICKS(ms) (ms)
```

### `shims/ble_service.h`
Re-exports the event types without pulling in FreeRTOS:
```c
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
```

## LVGL Configuration (`simulator/lv_conf.h`)

Key settings matching firmware behavior:
- `LV_COLOR_DEPTH 16` (RGB565)
- `LV_USE_SDL 1` (interactive mode display driver)
- `LV_FONT_MONTSERRAT_8 1`, `LV_FONT_MONTSERRAT_10 1`, `LV_FONT_MONTSERRAT_18 1`
- `LV_USE_FLOAT 1`
- No `LV_USE_OS` (no RTOS)

## Build System

Plain CMake, not ESP-IDF:

```bash
cd simulator
cmake -B build
cmake --build build
```

### Dependencies

| Dependency | Source | Install |
|---|---|---|
| SDL2 | Homebrew | `brew install sdl2` |
| LVGL v9.5 | `firmware/managed_components/lvgl__lvgl/` | Compiled from existing source |
| stb_image_write | Vendored in `simulator/` | Single header |
| cJSON | Vendored in `simulator/cjson/` | Single .c/.h |

### CMake structure

- Compiles LVGL from `../firmware/managed_components/lvgl__lvgl/`
- Compiles shared firmware sources from `../firmware/main/` (scene, notification_ui, ui_manager, notification)
- Include path order: `simulator/shims/` first (shadows ESP-IDF headers), then `simulator/` (for `lv_conf.h`), then LVGL, then `firmware/main/`
- Links SDL2

## Agent Development Loop

The primary use case for Claude agents:

```bash
# 1. Build
cmake --build simulator/build

# 2. Run with events, capture screenshots
./simulator/build/clawd-sim --headless \
  --events 'connect; wait 500; notify "GitHub" "PR merged"; wait 3000' \
  --screenshot-interval 100 --screenshot-on-event \
  --screenshot-dir ./shots/

# 3. Inspect key frames (agent uses Read tool on PNGs)
#    ./shots/frame_000000.png  → idle state before connect
#    ./shots/event_000000_connect.png → moment of connection
#    ./shots/frame_000500.png  → just connected
#    ./shots/event_000500_notify.png → notification arriving
#    ./shots/frame_001000.png  → notification panel sliding in
#    ./shots/frame_003500.png  → settled state

# 4. Make code changes to firmware/main/ files
# 5. Rebuild and re-run (go to step 1)
```

## CLI Summary

```
clawd-sim [OPTIONS]

Display:
  --headless              Run without SDL2 window
  --scale <N>             Window scale factor (default: 3, interactive only)

Events:
  --events '<commands>'   Inline event string (semicolon-separated)
  --scenario <file.json>  Load timed events from JSON file

Screenshots:
  --screenshot-dir <dir>  Output directory for PNGs
  --screenshot-interval <ms>  Periodic screenshot interval
  --screenshot-on-event   Screenshot after each event

General:
  --run-ms <ms>           Total simulation duration (headless; default: inferred from events)
  --help                  Show usage
```

## Out of Scope (for now)

- GIF/video export (can be added later)
- Touch/mouse input simulation
- Network simulation
- Hot-reload on file changes (could be added — `fswatch` + rebuild + re-run)
- CI integration (trivial once headless works, but not designing CI config now)
