@echo off
REM ============================================
REM ClanManager Windows Packaging Script
REM ============================================
REM Erstellt eine portable ZIP-Distribution mit
REM allen benötigten Qt-DLLs
REM ============================================

set APP_NAME=ClanManager
set BUILD_DIR=build
set DIST_DIR=dist\%APP_NAME%

echo.
echo === ClanManager Windows Packaging ===
echo.

REM Prüfe ob Build existiert
if not exist "%BUILD_DIR%\%APP_NAME%.exe" (
    echo FEHLER: %APP_NAME%.exe nicht gefunden!
    echo Bitte zuerst kompilieren:
    echo   cmake -B build -DCMAKE_BUILD_TYPE=Release
    echo   cmake --build build --config Release
    exit /b 1
)

REM Erstelle Distribution-Ordner
echo [1/4] Erstelle Distribution-Ordner...
if exist dist rmdir /s /q dist
mkdir "%DIST_DIR%"

REM Kopiere EXE
echo [2/4] Kopiere Anwendung...
copy "%BUILD_DIR%\%APP_NAME%.exe" "%DIST_DIR%\" >nul
if exist "distribution\demo_players.csv" copy "distribution\demo_players.csv" "%DIST_DIR%\" >nul

REM Qt-Dependencies kopieren (windeployqt)
echo [3/4] Kopiere Qt-Abhängigkeiten (windeployqt)...
windeployqt --release --no-translations --no-system-d3d-compiler --no-opengl-sw "%DIST_DIR%\%APP_NAME%.exe"

REM README kopieren
echo [4/4] Erstelle README...
if exist "distribution\README_Windows.txt" (
    copy "distribution\README_Windows.txt" "%DIST_DIR%\LIESMICH.txt" >nul
)

REM ZIP erstellen
echo.
echo Erstelle ZIP-Archiv...
cd dist
if exist "..\%APP_NAME%-Windows-x64.zip" del "..\%APP_NAME%-Windows-x64.zip"
tar -a -cf "..\%APP_NAME%-Windows-x64.zip" %APP_NAME%
cd ..

echo.
echo ==========================================
echo FERTIG!
echo ==========================================
echo.
echo ZIP-Datei erstellt: %APP_NAME%-Windows-x64.zip
echo.
echo Diese Datei kannst du direkt an andere
echo Personen zum Testen schicken!
echo.
pause
