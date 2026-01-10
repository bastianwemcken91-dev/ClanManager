# Clan Manager (CSV Demo)

Minimaler Qt6 (Widgets) Prototyp zum Verwalten einer Spieler-Liste mit GUI.

Features:

- Tabelle mit Feldern: Spielername, T17-Name, Level, Gruppe, Teilnahmen, Kommentar, Beitrittsdatum, Dienstrang
- Import/Export über CSV/Tab-separierte Dateien
- Teilnahmezähler (+1) für ausgewählte Spieler

## Backups

Ein Skript zum Erstellen und Rotieren von Backups befindet sich unter `tools/backup_clanmanager.sh`.

Standard (Voll + Source, behält die neuesten N ZIPs):

```bash
./tools/backup_clanmanager.sh
```

Nur Vollbackup:

```bash
./tools/backup_clanmanager.sh full
```

Nur Source-Dateien (ohne `build/`, ohne `.app`):

```bash
./tools/backup_clanmanager.sh src
```

Backups älter als 7 Tage löschen:

```bash
./tools/backup_clanmanager.sh prune 7
```

Anzahl der zu behaltenden ZIPs überschreiben:

```bash
KEEP_FULL=10 KEEP_SRC=15 ./tools/backup_clanmanager.sh
```

Ergebnis liegt auf dem macOS Desktop (`ClanManager_full_<timestamp>.zip`, `ClanManager_src_<timestamp>.zip`, sowie aktuelles `ClanManager_backup_latest.zip`).

Build (macOS, Qt6):

```bash
mkdir build && cd build
cmake -DCMAKE_PREFIX_PATH="$(brew --prefix qt)/lib/cmake/Qt6" ..
cmake --build .
./ClanManager.app/Contents/MacOS/ClanManager
```

Packaging:

After a successful build you can prepare a distributable package:

```bash
# from project root
./distribution/install.sh
# then create tarball
cd distribution
tar -czf ClanManager.tar.gz ClanManager
```

Hinweis: Zur Zeit ist nur CSV/TSV-Import implementiert. Excel/XLSX kann später über `QXlsx` eingebunden werden.
