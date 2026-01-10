#!/usr/bin/env bash
# Build helper: configure, strip hard AGL flags from generated link.txt files, then build
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT_DIR"

rm -rf build
mkdir build && cd build
cmake -DCMAKE_PREFIX_PATH="$(brew --prefix qt)/lib/cmake/Qt6" ..
# Remove hard AGL entries from generated link scripts (workaround)
# Use a small Python rewrite to avoid BSD sed in-place quirks that may truncate files
python3 - <<'PY'
import glob,io
for path in glob.glob('**/link.txt', recursive=True):
	try:
		with open(path, 'r', encoding='utf-8', errors='ignore') as f:
			s = f.read()
	except Exception:
		continue
	ns = s.replace(' -framework AGL', '').replace('-framework AGL', '')
	ns = ns.replace('-Wl,-weak_framework,AGL', '')
	ns = ns.replace(',-weak_framework,AGL', '')
	if ns != s:
		with open(path, 'w', encoding='utf-8') as f:
			f.write(ns)
PY
cmake --build . --verbose

echo "Build finished. Run: ./build/ClanManager.app/Contents/MacOS/ClanManager"
