#!/usr/bin/env python3
"""
Generate PNG frames for all Clawd animations.

Uses the exact Clawd geometry from the Piskel reference / idle animation
for consistent character appearance across all states.

Output PNGs use #1a1a2e background which png2rgb565.py treats as transparent.

Usage:
    python tools/sprite-designer/generate_sprite_pngs.py
"""

import os
import sys

try:
    from PIL import Image, ImageDraw
except ImportError:
    print("Error: Pillow is required. Install with: pip install Pillow")
    sys.exit(1)

W, H = 64, 64
BG = (0x1A, 0x1A, 0x2E)

# Authentic Clawd color from Piskel reference (NOT the spec's #ff6b2b)
BODY_COLOR = (0xE3, 0x87, 0x6E)
# Darker sleeping variant (desaturated version of authentic color)
BODY_DARK = (0x8A, 0x52, 0x43)

EYE_COLOR = (0x00, 0x00, 0x00)
EYE_CLOSED = (0x55, 0x55, 0x55)  # Sleeping closed eyes
ALERT_COLOR = (0xFF, 0xDD, 0x57)
SPARKLE_COLOR = (0xFF, 0xDD, 0x57)
Z_COLOR = (0x77, 0x77, 0xBB)
Z_FADE = (0x55, 0x55, 0x88)
BLE_COLOR = (0x44, 0x66, 0xAA)

# ---- Clawd geometry from Piskel reference (matches idle animation exactly) ----
# Body top: 40x7 at (12, 17)
# Body eye region: 40x6 at (12, 24)
# Claw/wide band: 52x7 at (6, 30)
# Body lower: 40x6 at (12, 37)
# 4 legs: each 4x7 at y=43, x = 15, 22, 39, 45
# Eyes: 2x6 each at (19, 24) and (43, 24)

LEG_POSITIONS = [15, 22, 39, 45]
LEG_W, LEG_H = 4, 7
LEG_Y = 43


def new_frame():
    """Create a new 64x64 image with background color."""
    img = Image.new("RGB", (W, H), BG)
    return img, ImageDraw.Draw(img)


def draw_rect(draw, x, y, w, h, color):
    """Draw a filled rectangle (x, y, width, height)."""
    draw.rectangle([x, y, x + w - 1, y + h - 1], fill=color)


def draw_clawd(draw, dx=0, dy=0, eye_dx=0, eye_dy=0, eye_mode='open',
               leg_h=LEG_H, color=None):
    """Draw the Clawd sprite matching Piskel reference geometry."""
    c = color or BODY_COLOR

    # Main body top (y=17..23)
    draw_rect(draw, 12 + dx, 17 + dy, 40, 7, c)
    # Body eye region (y=24..29)
    draw_rect(draw, 12 + dx, 24 + dy, 40, 6, c)
    # Claw/wide band (y=30..36)
    draw_rect(draw, 6 + dx, 30 + dy, 52, 7, c)
    # Body lower (y=37..42)
    draw_rect(draw, 12 + dx, 37 + dy, 40, 6, c)

    # 4 Legs
    for lx in LEG_POSITIONS:
        draw_rect(draw, lx + dx, LEG_Y + dy, LEG_W, leg_h, c)

    # Eyes
    if eye_mode == 'open':
        # Full 2x6 eyes
        draw_rect(draw, 19 + dx + eye_dx, 24 + dy + eye_dy, 2, 6, EYE_COLOR)
        draw_rect(draw, 43 + dx + eye_dx, 24 + dy + eye_dy, 2, 6, EYE_COLOR)
    elif eye_mode == 'half-closed':
        draw_rect(draw, 19 + dx + eye_dx, 27 + dy + eye_dy, 2, 3, EYE_COLOR)
        draw_rect(draw, 43 + dx + eye_dx, 27 + dy + eye_dy, 2, 3, EYE_COLOR)
    elif eye_mode == 'closed':
        draw_rect(draw, 19 + dx + eye_dx, 28 + dy + eye_dy, 2, 1, EYE_COLOR)
        draw_rect(draw, 43 + dx + eye_dx, 28 + dy + eye_dy, 2, 1, EYE_COLOR)
    elif eye_mode == 'half-open':
        draw_rect(draw, 19 + dx + eye_dx, 26 + dy + eye_dy, 2, 3, EYE_COLOR)
        draw_rect(draw, 43 + dx + eye_dx, 26 + dy + eye_dy, 2, 3, EYE_COLOR)


def draw_exclamation(draw, dx=0):
    """Draw '!' alert mark above Clawd's head."""
    # Centered above body (body is 40px wide starting at x=12, center ~32)
    ex, ey = 31 + dx, 11
    # Stick (2x3)
    draw_rect(draw, ex, ey, 2, 3, ALERT_COLOR)
    # Dot (2x1, with 1px gap)
    draw_rect(draw, ex, ey + 4, 2, 1, ALERT_COLOR)


def draw_sparkles(draw, dy=0):
    """Draw sparkle crosses around Clawd's body."""
    # Body bounds: x=6..57 (claw band), y=17..42 (top to lower body)
    bx, by = 6, 17 + dy
    bx2, by2 = 58, 43 + dy

    sparkle_points = [
        [(bx - 3, by + 4), (bx - 2, by + 3), (bx - 2, by + 5), (bx - 1, by + 4)],
        [(bx2 + 1, by + 4), (bx2 + 2, by + 3), (bx2 + 2, by + 5), (bx2 + 3, by + 4)],
        [(bx - 3, by2 - 4), (bx - 2, by2 - 5), (bx - 2, by2 - 3), (bx - 1, by2 - 4)],
        [(bx2 + 1, by2 - 4), (bx2 + 2, by2 - 5), (bx2 + 2, by2 - 3), (bx2 + 3, by2 - 4)],
    ]

    for points in sparkle_points:
        for px, py in points:
            if 0 <= px < W and 0 <= py < H:
                draw.point((px, py), fill=SPARKLE_COLOR)


def draw_question_mark(draw):
    """Draw '?' mark above Clawd's head."""
    # Centered above body
    qx, qy = 31, 11
    draw_rect(draw, qx, qy, 2, 1, ALERT_COLOR)
    draw.point((qx + 2, qy), fill=ALERT_COLOR)
    draw.point((qx + 2, qy + 1), fill=ALERT_COLOR)
    draw.point((qx + 1, qy + 2), fill=ALERT_COLOR)
    draw.point((qx + 1, qy + 4), fill=ALERT_COLOR)


def draw_z(draw, x, y, color):
    """Draw a small 'z' character in 3x3 pixels."""
    draw_rect(draw, x, y, 3, 1, color)
    draw.point((x + 1, y + 1), fill=color)
    draw_rect(draw, x, y + 2, 3, 1, color)


def draw_ble_icon(draw):
    """Draw 16x16 Bluetooth rune symbol."""
    c = BLE_COLOR
    for y in range(3, 13):
        draw.point((8, y), fill=c)
    draw.point((9, 4), fill=c)
    draw.point((10, 5), fill=c)
    draw.point((9, 6), fill=c)
    draw.point((9, 9), fill=c)
    draw.point((10, 10), fill=c)
    draw.point((9, 11), fill=c)
    draw.point((5, 9), fill=c)
    draw.point((6, 8), fill=c)
    draw.point((7, 7), fill=c)
    draw.point((5, 6), fill=c)
    draw.point((6, 7), fill=c)
    draw.point((9, 3), fill=c)
    draw.point((9, 12), fill=c)


# ---- Sleeping Clawd ----
# Curled up pose: body squished wider/shorter, positioned lower
# Based on authentic proportions but compressed
# Body: 48x10 at (8, 30) — wider, shorter
# Claw band absorbed into body at this squished scale
# Legs: very short stubs (4x3) at similar positions
# Eyes: horizontal closed lines (2x1 each)

SLEEP_BODY = {"x": 8, "y": 30, "w": 48, "h": 10}
SLEEP_LEG_POSITIONS = [14, 21, 38, 44]
SLEEP_LEG_Y = 40
SLEEP_LEG_H = 3
SLEEP_EYE_LEFT = {"x": 18, "y": 34}
SLEEP_EYE_RIGHT = {"x": 42, "y": 34}


def draw_sleeping_clawd(draw, dy=0):
    """Draw sleeping Clawd using authentic proportions, squished down."""
    c = BODY_DARK
    # Wider body
    draw_rect(draw, SLEEP_BODY["x"], SLEEP_BODY["y"] + dy,
              SLEEP_BODY["w"], SLEEP_BODY["h"], c)
    # Short leg stubs
    for lx in SLEEP_LEG_POSITIONS:
        draw_rect(draw, lx, SLEEP_LEG_Y + dy, LEG_W, SLEEP_LEG_H, c)
    # Closed eyes (horizontal lines)
    draw_rect(draw, SLEEP_EYE_LEFT["x"], SLEEP_EYE_LEFT["y"] + dy, 2, 1, EYE_CLOSED)
    draw_rect(draw, SLEEP_EYE_RIGHT["x"], SLEEP_EYE_RIGHT["y"] + dy, 2, 1, EYE_CLOSED)


# ---- Animation generators ----

def generate_alert_frames():
    """Generate 6 frames for the alert animation."""
    frames = []

    # Frame 0: Neutral
    img, draw = new_frame()
    draw_clawd(draw)
    frames.append(img)

    # Frame 1: Eyes shift right
    img, draw = new_frame()
    draw_clawd(draw, eye_dx=1)
    frames.append(img)

    # Frame 2: Body leans right, eyes shifted
    img, draw = new_frame()
    draw_clawd(draw, dx=1, eye_dx=1)
    frames.append(img)

    # Frame 3: "!" appears
    img, draw = new_frame()
    draw_clawd(draw, dx=1, eye_dx=1)
    draw_exclamation(draw, dx=1)
    frames.append(img)

    # Frame 4: Hold alert pose
    img, draw = new_frame()
    draw_clawd(draw, dx=1, eye_dx=1)
    draw_exclamation(draw, dx=1)
    frames.append(img)

    # Frame 5: "!" fades
    img, draw = new_frame()
    draw_clawd(draw, dx=1, eye_dx=1)
    frames.append(img)

    return frames


def generate_happy_frames():
    """Generate 6 frames for the happy animation."""
    frames = []

    # Frame 0: Neutral
    img, draw = new_frame()
    draw_clawd(draw)
    frames.append(img)

    # Frame 1: Crouch (shorter legs)
    img, draw = new_frame()
    draw_clawd(draw, leg_h=4)
    frames.append(img)

    # Frame 2: Jump up 4px
    img, draw = new_frame()
    draw_clawd(draw, dy=-4)
    frames.append(img)

    # Frame 3: Peak with sparkles
    img, draw = new_frame()
    draw_clawd(draw, dy=-4)
    draw_sparkles(draw, dy=-4)
    frames.append(img)

    # Frame 4: Coming down
    img, draw = new_frame()
    draw_clawd(draw, dy=-2)
    frames.append(img)

    # Frame 5: Landing
    img, draw = new_frame()
    draw_clawd(draw)
    frames.append(img)

    return frames


def generate_sleeping_frames():
    """Generate 6 frames for the sleeping animation."""
    frames = []

    # Frame 0: Curled up
    img, draw = new_frame()
    draw_sleeping_clawd(draw)
    frames.append(img)

    # Frame 1: Same
    img, draw = new_frame()
    draw_sleeping_clawd(draw)
    frames.append(img)

    # Frame 2: Breathe out (+1px)
    img, draw = new_frame()
    draw_sleeping_clawd(draw, dy=1)
    frames.append(img)

    # Frame 3: Breathe in + "z"
    img, draw = new_frame()
    draw_sleeping_clawd(draw, dy=-1)
    draw_z(draw, 52, 25, Z_COLOR)
    frames.append(img)

    # Frame 4: "z" floats up
    img, draw = new_frame()
    draw_sleeping_clawd(draw)
    draw_z(draw, 53, 22, Z_FADE)
    frames.append(img)

    # Frame 5: New "z" near body
    img, draw = new_frame()
    draw_sleeping_clawd(draw)
    draw_z(draw, 52, 26, Z_COLOR)
    frames.append(img)

    return frames


def generate_disconnected_frames():
    """Generate 6 frames for the disconnected animation."""
    frames = []

    # Frame 0: Eyes looking up-right
    img, draw = new_frame()
    draw_clawd(draw, eye_dx=1, eye_dy=-1)
    frames.append(img)

    # Frame 1: Head tilts right
    img, draw = new_frame()
    draw_clawd(draw, dx=1, eye_dx=1, eye_dy=-1)
    frames.append(img)

    # Frame 2: Eyes shift left
    img, draw = new_frame()
    draw_clawd(draw, eye_dx=-1)
    frames.append(img)

    # Frame 3: Eyes back right
    img, draw = new_frame()
    draw_clawd(draw, eye_dx=1, eye_dy=-1)
    frames.append(img)

    # Frame 4: "?" appears
    img, draw = new_frame()
    draw_clawd(draw, eye_dx=1, eye_dy=-1)
    draw_question_mark(draw)
    frames.append(img)

    # Frame 5: "?" fades
    img, draw = new_frame()
    draw_clawd(draw, eye_dx=1, eye_dy=-1)
    frames.append(img)

    return frames


def generate_ble_icon():
    """Generate 16x16 BLE icon."""
    img = Image.new("RGB", (16, 16), BG)
    draw = ImageDraw.Draw(img)
    draw_ble_icon(draw)
    return [img]


def save_frames(frames, output_dir):
    """Save frames as numbered PNG files."""
    os.makedirs(output_dir, exist_ok=True)
    for i, img in enumerate(frames):
        path = os.path.join(output_dir, f"frame_{i:02d}.png")
        img.save(path, "PNG")
        print(f"  Saved: {path}")


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))

    animations = [
        ("alert", generate_alert_frames),
        ("happy", generate_happy_frames),
        ("sleeping", generate_sleeping_frames),
        ("disconnected", generate_disconnected_frames),
        ("ble-icon", generate_ble_icon),
    ]

    for name, gen_fn in animations:
        print(f"Generating {name} frames...")
        frames = gen_fn()
        out_dir = os.path.join(script_dir, "exports", name)
        save_frames(frames, out_dir)
        print(f"  {len(frames)} frame(s) saved to {out_dir}\n")

    print("Done! Run png2rgb565.py to convert to C headers.")


if __name__ == "__main__":
    main()
