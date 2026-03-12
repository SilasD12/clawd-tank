#ifndef CLAWD_SPRITES_H
#define CLAWD_SPRITES_H

/**
 * Master sprite header — includes all Clawd animation frames and BLE icon.
 * Auto-generated sprite data lives in individual headers.
 */

#include "sprite_idle.h"
#include "sprite_alert.h"
#include "sprite_happy.h"
#include "sprite_sleeping.h"
#include "sprite_disconnected.h"
#include "sprite_ble_icon.h"

/* Transparent key color (RGB565 for #1a1a2e) */
#define CLAWD_TRANSPARENT_KEY 0x18C5

/* Sprite canvas size */
#define CLAWD_SPRITE_W 64
#define CLAWD_SPRITE_H 64

/* BLE icon size */
#define BLE_ICON_W 16
#define BLE_ICON_H 16

/* Animation IDs (must match clawd_anim_id_t in scene.h) */
#define CLAWD_ANIM_COUNT 5

/* Frame counts per animation */
static const int clawd_frame_counts[CLAWD_ANIM_COUNT] = {
    IDLE_FRAME_COUNT,
    ALERT_FRAME_COUNT,
    HAPPY_FRAME_COUNT,
    SLEEPING_FRAME_COUNT,
    DISCONNECTED_FRAME_COUNT,
};

/* Frame tables per animation */
static const uint16_t* const * const clawd_frame_tables[CLAWD_ANIM_COUNT] = {
    idle_frames,
    alert_frames,
    happy_frames,
    sleeping_frames,
    disconnected_frames,
};

#endif // CLAWD_SPRITES_H
