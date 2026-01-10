# PG60 Manager

Minimaler Qt-basierten Clan-Manager (PG60 Manager) — Grundgerüst

Build (macOS, Qt6 installed):

```bash
mkdir -p build && cd build
cmake ..
cmake --build . -j 8
./PG60Manager
```

Windows (MSVC + Qt6):

1. Installiere Visual Studio mit "Desktop development with C++" (MSVC toolset), CMake und Qt (Qt installer, wähle die entsprechende MSVC-Build).
2. Öffne `x64 Native Tools Command Prompt for VS 20xx` oder setze die passende VS-Umgebung.
3. In der Projekt-Ordner `ClanManagerNew`:

```powershell
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
.
# oder ausführbare Datei direkt starten:
cd Release
PG60Manager.exe
```

Deployment (Windows):
- Nutze `windeployqt` (aus dem Qt-Bin-Ordner) um alle benötigten Qt-DLLs zu bündeln:

```powershell
"C:\Qt\6.x\msvc2019_64\bin\windeployqt.exe" path\to\Release\PG60Manager.exe
```

Alternativ kannst du das Projekt mit Qt Creator öffnen (öffne `CMakeLists.txt`) und den Kits-Dialog verwenden.


Funktionen:
- Einfache GUI mit Tabelle
- CSV Import/Export (erste Spalten: id,name,level,group,joinDate(YYYY-MM-DD),rank)
- Neuen Spieler hinzufügen (Name)

Weitere Funktionen:
- Doppelklick auf eine Zeile: Spieler bearbeiten (Detaildialog)
- Rechtsklick-Kontextmenü: Bearbeiten / Befördern / Degradieren / Löschen / Soldbuch
- Suche & Rang-Filter in der UI; Rang-Filter wird aus den geladenen Daten befüllt
- Beispiel-CSV: `sample_players.csv`

Dies ist ein kleines Scaffold; wir können die UI weiter anpassen (Filter, Reihenfolge, Buttons) entsprechend dem Screenshot.
