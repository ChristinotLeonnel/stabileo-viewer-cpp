@echo off
REM =========================================================================
REM  build_and_run.bat — Compile en Release et lance l'exécutable
REM =========================================================================

set CMAKE_EXE="C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

echo.
echo ======================================================
echo   StabileoViewer — Compilation Release
echo ======================================================
echo.

if not exist build (
    echo [INFO] Génération de la solution CMake ...
    %CMAKE_EXE% -B build -G "Visual Studio 18 2026" -A x64
    if %ERRORLEVEL% NEQ 0 (
        echo [ERREUR] cmake configure échoué.
        pause
        exit /b 1
    )
)

%CMAKE_EXE% --build build --config Release --parallel

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERREUR] La compilation a échoué.
    pause
    exit /b 1
)

echo.
echo [OK] Compilation réussie. Lancement ...
echo.

start "" "build\bin\Release\StabileoViewer.exe"
