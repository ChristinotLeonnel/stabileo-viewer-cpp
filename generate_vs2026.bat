@echo off
REM =========================================================================
REM  generate_vs2026.bat — Génère la solution Visual Studio 2026
REM  Usage : double-cliquer ou exécuter depuis ce dossier
REM =========================================================================

set CMAKE_EXE="C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

echo.
echo ======================================================
echo   StabileoViewer — Génération de la solution VS 2026
echo ======================================================
echo.

if not exist build mkdir build

%CMAKE_EXE% -B build -G "Visual Studio 18 2026" -A x64

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERREUR] La génération CMake a échoué.
    pause
    exit /b 1
)

echo.
echo [OK] Solution générée : build\StabileoViewer.sln
echo      Ouvrez-la dans Visual Studio 2026 et compilez en Release.
echo.

start "" "build\StabileoViewer.sln"
pause
