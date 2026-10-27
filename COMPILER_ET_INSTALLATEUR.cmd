@echo off
setlocal EnableExtensions DisableDelayedExpansion
cd /d "%~dp0"
title ACID303 SOLO V3.0.0 OFFICIEL - BURNOUT THEORY - MIGRATION DEPUIS V206

echo =========================================================
echo  ACID303 SOLO V3.0.0 OFFICIEL - BUILD SANS CHEMINS LONGS
echo =========================================================
echo.
echo Ce kit est une MIGRATION : l installateur V3 remplacera le VST3 v2.0.6 (meme identite).
echo Les fichiers temporaires sont places dans un chemin court.
echo.
where powershell.exe >nul 2>&1
if errorlevel 1 goto :missing_powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\Build-Official.ps1"
if errorlevel 1 goto :error

echo.
echo Fin du script de construction.
pause
exit /b 0

:missing_powershell
echo ERREUR : Windows PowerShell est introuvable.
:error
echo.
echo ECHEC : aucun VST3 installe n'a ete modifie.
echo En cas d'erreur, envoyez-moi le journal complet.
pause
exit /b 1
