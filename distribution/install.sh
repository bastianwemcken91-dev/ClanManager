#!/usr/bin/env bash
# Simple installer/packager script for ClanManager
# Usage: run from project root or call with path to build output

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_APP="$ROOT_DIR/build/ClanManager.app"
OUT_DIR="$ROOT_DIR/distribution/ClanManager"

echo "Root: $ROOT_DIR"

if [ ! -d "$BUILD_APP" ]; then
  echo "Build output not found at $BUILD_APP"
  echo "Please build the project first (see README), then re-run this script." >&2
  exit 1
fi

rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"

echo "Copying app to distribution folder..."
cp -a "$BUILD_APP" "$OUT_DIR/"

# Copy example resources if present
if [ -d "$ROOT_DIR/distribution/resources" ]; then
  echo "Copying example resources..."
  cp -a "$ROOT_DIR/distribution/resources" "$OUT_DIR/"
fi

echo "Distribution prepared at: $OUT_DIR"
echo "You can compress it with: (cd $ROOT_DIR/distribution && tar -czf ClanManager.tar.gz ClanManager)"
