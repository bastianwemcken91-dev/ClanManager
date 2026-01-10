#!/usr/bin/env bash
# Package ClanManager app and resources into zip and optional DMG (unsigned)
# Run from project root: ./distribution/package.sh

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_APP="$ROOT_DIR/build/ClanManager.app"
OUT_DIR="$ROOT_DIR/distribution/ClanManager"
RES_DIR="$ROOT_DIR/distribution/resources"
ZIP_OUT="$ROOT_DIR/distribution/ClanManager.zip"
DMG_OUT="$ROOT_DIR/distribution/ClanManager.dmg"

if [ ! -d "$BUILD_APP" ]; then
  echo "Build output not found at $BUILD_APP"
  echo "Please build the project first (see README), then re-run this script." >&2
  exit 1
fi

rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"

echo "Copying app to distribution folder..."
cp -a "$BUILD_APP" "$OUT_DIR/"

if [ -d "$RES_DIR" ]; then
  echo "Copying resources..."
  cp -a "$RES_DIR" "$OUT_DIR/"
fi

# Create zip
if [ -f "$ZIP_OUT" ]; then rm -f "$ZIP_OUT"; fi
(cd "$ROOT_DIR/distribution" && zip -r "$(basename "$ZIP_OUT")" "$(basename "$OUT_DIR")")

echo "Created zip: $ZIP_OUT"

# Create unsigned DMG (macOS only)
if [[ "$(uname -s)" == "Darwin" ]]; then
  if [ -f "$DMG_OUT" ]; then rm -f "$DMG_OUT"; fi
  echo "Creating DMG (unsigned) at $DMG_OUT..."
  hdiutil create -srcfolder "$OUT_DIR" -volname ClanManager -ov -format UDZO "$DMG_OUT"
  echo "Created DMG: $DMG_OUT"
else
  echo "Skipping DMG creation (not macOS)."
fi

echo "Packaging complete. Output: $ZIP_OUT ${DMG_OUT}"