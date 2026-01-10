#!/usr/bin/env zsh
# Backup-Skript für ClanManager
# Erstellt Voll- und Source-Backups mit Rotation.
# Nutzung:
#   ./tools/backup_clanmanager.sh            # Voll + Source, Standard Rotation
#   ./tools/backup_clanmanager.sh full       # nur Vollbackup
#   ./tools/backup_clanmanager.sh src        # nur Sourcebackup
#   ./tools/backup_clanmanager.sh prune 7    # löscht Voll/Source Backups älter als 7 Tage
#   KEEP_FULL=5 KEEP_SRC=10 ./tools/backup_clanmanager.sh   # Anzahl behalten überschreiben
#
# Output: ZIP-Dateien auf $HOME/Desktop.

set -euo pipefail

PROJECT_ROOT="${0:A:h:h}"  # tools/ -> Projektwurzel
DESKTOP="$HOME/Desktop"
TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
FULL_PREFIX="ClanManager_full_"
SRC_PREFIX="ClanManager_src_"
FULL_NAME="${FULL_PREFIX}${TIMESTAMP}.zip"
SRC_NAME="${SRC_PREFIX}${TIMESTAMP}.zip"
FULL_ZIP="$DESKTOP/$FULL_NAME"
SRC_ZIP="$DESKTOP/$SRC_NAME"
LATEST_FULL="$DESKTOP/ClanManager_backup_latest.zip"

KEEP_FULL="${KEEP_FULL:-5}"
KEEP_SRC="${KEEP_SRC:-8}"

cmd="${1:-both}"
arg2="${2:-}" || true

backup_full() {
  echo "[INFO] Erstelle Vollbackup: $FULL_ZIP"
  (cd "$PROJECT_ROOT" && zip -r "$FULL_ZIP" ClanManager >/dev/null)
  cp "$FULL_ZIP" "$LATEST_FULL"
}

backup_src() {
  echo "[INFO] Erstelle Sourcebackup: $SRC_ZIP"
  (cd "$PROJECT_ROOT" && zip -r "$SRC_ZIP" \
    ClanManager/include \
    ClanManager/src \
    ClanManager/CMakeLists.txt \
    ClanManager/README.md \
    ClanManager/tests \
    ClanManager/distribution \
    ClanManager/compile_commands.json \
    -x "*/build/*" "*/ClanManager.app/*" "*/ClanManager_autogen/*" >/dev/null)
}

prune_days() {
  local days="$1"
  echo "[INFO] Lösche Backups älter als $days Tage"
  find "$DESKTOP" -maxdepth 1 -name "${FULL_PREFIX}*.zip" -mtime +$days -print -delete || true
  find "$DESKTOP" -maxdepth 1 -name "${SRC_PREFIX}*.zip" -mtime +$days -print -delete || true
}

rotate_count() {
  echo "[INFO] Rotation nach Anzahl (FULL=$KEEP_FULL, SRC=$KEEP_SRC)"
  ls -1t "$DESKTOP"/${FULL_PREFIX}*.zip 2>/dev/null | tail -n +$((KEEP_FULL+1)) | while read -r f; do
    echo "[ROTATE] Entferne $f"; rm -f "$f"; done
  ls -1t "$DESKTOP"/${SRC_PREFIX}*.zip 2>/dev/null | tail -n +$((KEEP_SRC+1)) | while read -r f; do
    echo "[ROTATE] Entferne $f"; rm -f "$f"; done
}

case "$cmd" in
  full)
    backup_full; rotate_count ;;
  src)
    backup_src; rotate_count ;;
  prune)
    if [[ -z "$arg2" ]]; then
      echo "[ERROR] Anzahl Tage fehlt (z.B. prune 7)"; exit 1
    fi
    prune_days "$arg2" ;;
  both|*)
    backup_full
    backup_src
    rotate_count ;;
esac

echo "[DONE] Fertig. Dateien auf Desktop prüfen.";
