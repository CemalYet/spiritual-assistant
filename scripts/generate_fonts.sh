#!/usr/bin/env bash
# ──────────────────────────────────────────────────────────────────
# Font generation script for Spiritual Assistant (Adhan Watch)
#
# 2-family modern type system:
#   Inter (sans-serif) → all text, labels, headings, FA icons
#   DM Mono (mono)     → digits, countdown, clock, battery%
#
# Prerequisites:
#   npm install -g lv_font_conv   (or use npx)
#   Download into ./ttf/:
#     - Inter-Regular.ttf, Inter-SemiBold.ttf, Inter-Bold.ttf
#     - DMMono-Regular.ttf, DMMono-Medium.ttf
#     - FontAwesome5-Solid.woff  (from LVGL repo)
#   Inter: https://github.com/rsms/inter/releases
#   DM Mono: https://fonts.google.com/specimen/DM+Mono
#
# Usage:
#   cd scripts && bash generate_fonts.sh
#
# Output: ../src/fonts/font_*.c  (11 files)
# ──────────────────────────────────────────────────────────────────
set -euo pipefail

OUT="../src/fonts"
TTF="./ttf"
mkdir -p "$OUT"

# Common flags: 4bpp, uncompressed, subpixel LCD rendering for LVGL
C="--bpp 4 --no-compress --no-prefilter --lcd --format lvgl --lv-include lvgl.h --force-fast-kern-format"

# Turkish Latin + common symbols
RANGE="0x20-0x7E,0xB0,0xB7,0xC7,0xD6,0xDC,0xE7,0xF6,0xFC,0x11E-0x11F,0x130-0x131,0x15E-0x15F,0x2014,0x2022"

# FontAwesome5 symbols: Settings(F013), MapPin(F3C5), WiFi(F1EB)
FA="0xF013,0xF3C5,0xF1EB"

# Clock/countdown only need digits + colon + dash + degree + %
DIGITS="0x20,0x25,0x2D-0x3A,0xB0"

echo "=== Generating Inter fonts ==="

lv_font_conv $C --size 8  --font "$TTF/Inter-SemiBold.ttf" -r "$RANGE" -o "$OUT/font_cinzel_8.c"      --lv-font-name font_cinzel_8
echo "  8 semibold ✓"

lv_font_conv $C --size 10 --font "$TTF/Inter-SemiBold.ttf" -r "$RANGE" -o "$OUT/font_cinzel_10.c"     --lv-font-name font_cinzel_10
echo "  10 semibold ✓"

lv_font_conv $C --size 11 --font "$TTF/Inter-SemiBold.ttf" -r "$RANGE" -o "$OUT/font_cinzel_11.c"     --lv-font-name font_cinzel_11
echo "  11 semibold ✓"

lv_font_conv $C --size 12 --font "$TTF/Inter-SemiBold.ttf" -r "$RANGE" --font "$TTF/FontAwesome5-Solid.woff" -r "$FA" -o "$OUT/font_cinzel_12_sb.c" --lv-font-name font_cinzel_12_sb
echo "  12 semibold + FA ✓"

lv_font_conv $C --size 13 --font "$TTF/Inter-Regular.ttf"  -r "$RANGE" -o "$OUT/font_cinzel_13.c"     --lv-font-name font_cinzel_13
echo "  13 regular ✓"

lv_font_conv $C --size 14 --font "$TTF/Inter-Regular.ttf"  -r "$RANGE" -o "$OUT/font_cinzel_14.c"     --lv-font-name font_cinzel_14
echo "  14 regular ✓"

lv_font_conv $C --size 20 --font "$TTF/Inter-SemiBold.ttf" -r "$RANGE" --font "$TTF/FontAwesome5-Solid.woff" -r "$FA" -o "$OUT/font_cinzel_20_sb.c" --lv-font-name font_cinzel_20_sb
echo "  20 semibold + FA ✓"

echo ""
echo "=== Generating DM Mono fonts ==="

lv_font_conv $C --size 11 --font "$TTF/DMMono-Regular.ttf" -r "$RANGE"  -o "$OUT/font_dmmono_11.c" --lv-font-name font_dmmono_11
echo "  11 (strip times) ✓"

lv_font_conv $C --size 16 --font "$TTF/DMMono-Regular.ttf" -r "$RANGE"  -o "$OUT/font_dmmono_16.c" --lv-font-name font_dmmono_16
echo "  16 (seconds/IP) ✓"

lv_font_conv $C --size 66 --font "$TTF/DMMono-Regular.ttf" -r "$DIGITS" -o "$OUT/font_dmmono_66.c" --lv-font-name font_dmmono_66
echo "  66 (countdown) ✓"

lv_font_conv $C --size 80 --font "$TTF/DMMono-Regular.ttf" -r "$DIGITS" -o "$OUT/font_dmmono_80.c" --lv-font-name font_dmmono_80
echo "  80 (clock hero) ✓"

echo ""
echo "=== Done! 11 font files in $OUT ==="
ls -lh "$OUT"/font_*.c
