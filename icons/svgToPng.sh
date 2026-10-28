#!/bin/sh
# Render the PNGs listed in iconResources.qrc from their SVG sources, and
# build cstitch.icns for the macOS app bundle.  Requires rsvg-convert
# (brew install librsvg).
set -e
iconDir=$(cd "$(dirname "$0")" && pwd)
srcDir=$(dirname "$iconDir")
cd "$srcDir"

for png in $(grep -oE 'icons/[^<]*\.png' iconResources.qrc | sort -u); do
  svg="${png%.png}.svg"
  case "$png" in
    icons/32x32_cstitch.png) svg=icons/cstitch.svg ;;
  esac
  case "$png" in
    icons/32x32_*) rsvg-convert -w 32 -h 32 "$svg" -o "$png" ;;
    *) rsvg-convert "$svg" -o "$png" ;;
  esac
done

iconset=$(mktemp -d)/cstitch.iconset
mkdir -p "$iconset"
for size in 16 32 128 256 512; do
  rsvg-convert -w $size -h $size icons/cstitch.svg -o "$iconset/icon_${size}x${size}.png"
  rsvg-convert -w $((size*2)) -h $((size*2)) icons/cstitch.svg -o "$iconset/icon_${size}x${size}@2x.png"
done
iconutil -c icns "$iconset" -o icons/cstitch.icns
rm -rf "$(dirname "$iconset")"
