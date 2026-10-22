@echo off
setlocal
cd /d "%~dp0"
title Acid303 Solo 0.30.2 - Build VST3

echo ============================================
echo   ACID303 SOLO - BUILD VST3 Windows 64 bits
 echo ============================================
where cmake >nul 2>&1
if errorlevel 1 (echo ERREUR : Installer CMake et l'ajouter au PATH.&pause&exit /b 1)
where git >nul 2>&1
if errorlevel 1 (echo ERREUR : Installer Git pour Windows.&pause&exit /b 1)

echo [1/2] Configuration JUCE - necessite Internet au premier lancement...
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 goto :error

echo [2/2] Compilation Release...
cmake --build build --config Release --target Acid303Solo_VST3 --parallel 4
if errorlevel 1 goto :error

echo.
echo COMPILATION TERMINEE - dossier a copier :
echo   build\Acid303Solo_artefacts\Release\VST3\Acid303 Solo.vst3
echo vers :
echo   C:\Program Files\Common Files\VST3\
echo.
pause
exit /b 0
:error
echo.
echo ECHEC : Copier la premiere erreur de compilation pour depannage.
pause
exit /b 1
