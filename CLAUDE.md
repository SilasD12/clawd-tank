# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Clawd Tank is a physical notification display for Claude Code sessions. It runs on a **M5StickC Plus2** (240x135 ST7789V2 SPI display) and shows an animated pixel-art crab ("Clawd") alongside notification cards received over BLE from a Python host daemon.

Three components: **firmware** (ESP-IDF C), **simulator** (native macOS/Linux), **host** (Python daemon + Claude Code hooks).

## Build Commands

### Firmware (ESP-IDF 5.3.2)

Environment is managed via direnv (`firmware/.envrc`). ESP-IDF path: `bsp/esp-idf/`.
**Hardware Note:** M5StickC Plus2 requires **GPIO 4 (Power Hold)** to be set HIGH immediately in `app_main` to maintain power.

```bash
# Build
cd firmware && idf.py build

# Flash + monitor (using idf.py)
cd firmware && idf.py -p /dev/ttyACM0 flash monitor

# Build + Flash (using PlatformIO)
cd firmware && pio run -e m5stick-c-plus2 -t upload
```

### Simulator

Requires CMake 3.16+ and SDL2. For distribution, use static linking.

```bash
# Build (dynamic SDL2 — local development)
cd simulator && cmake -B build && cmake --build build

# Run interactive
./simulator/build/clawd-tank-sim
```

### Host Daemon (Linux/macOS)

The daemon bridges Claude Code to the display via BLE.

```bash
# Setup (Linux)
cd host
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements-linux.txt

# Run Daemon (BLE scanning)
python3 -m clawd_tank_daemon.daemon --ble

# Install Claude Code Hooks
./install-hooks.sh
```

**Daemon Flags:**
- `--ble`: Enable BLE transport (default on Linux).
- `--sim`: Enable simulator transport (TCP).
- `--takeover`: Stop any existing daemon process and take over the lock.

### Tests

```bash
# C unit tests (firmware)
cd firmware/test && make test

# Python tests (host daemon)
cd host && .venv/bin/pytest -v
```

## Architecture

### Data Flow

```
Claude Code hooks (SessionStart/PreToolUse/Notification/Stop/etc.)
    → ~/.clawd-tank/clawd-tank-notify (Hook Handler)
    → Unix socket
    → clawd_tank_daemon (Daemon)
    → BLE (NimBLE)
    → M5StickC Plus2 (Firmware)
```

### Hardware Specifics (M5StickC Plus2)

- **Power Hold:** GPIO 4 (Must be HIGH).
- **Display:** ST7789V2, 240x135, SPI.
- **Backlight:** GPIO 27 (PWM via LEDC).
- **Internal LED:** GPIO 19.
- **Reset:** GPIO 12.
- **SPI Pins:** MOSI (15), SCLK (13), CS (5), DC (14).

## Key Constraints & Current Screen Situation

- **Safe Mode Active**: The display is currently in a "Safe Mode" configuration to ensure hardware stability on Linux/M5StickC Plus2.
- **Initialization**: `display_init` in `display.c` clears the screen to **WHITE** (`0xFF`) to verify pixel drive.
- **Forced Brightness**: `ui_manager.c` explicitly calls `display_set_brightness(255)` on all BLE events (connect, disconnect, sync) to prevent the screen from going black due to daemon sync commands.
- **Clock Speed**: SPI clock is reduced to **8MHz** (`LCD_PIXEL_CLK`) for maximum signal integrity.
- **Power Hold**: **GPIO 4** MUST be set HIGH at the very start of `app_main` in `main.c` or the device will shut down immediately.
- **Inversion**: `esp_lcd_panel_invert_color(panel, true)` is enabled for correct color mapping.
- **Display**: 240x135 pixels, 16-bit RGB565.
- **BLE MTU**: 256 bytes.
- **Memory**: ESP32-PICO-V3-02 has 2MB PSRAM, but firmware aims to fit in internal SRAM (~200KB) for speed.

## TODO Tracking

Always check `TODO.md` for current progress. Update it after completing tasks.
