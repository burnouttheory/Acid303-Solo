@echo off
setlocal
cd /d "%~dp0"
echo ====================================================
echo   ACID303 SOLO - PREPARER UNE ANNONCE DE MISE A JOUR
 echo ====================================================
echo ATTENTION : publiez et testez la Release AVANT de lancer cet outil.
echo.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0GENERER_ANNONCE_GITHUB.ps1"
if errorlevel 1 (
  echo ERREUR : la publication n'a pas ete preparee.
  pause
  exit /b 1
)
echo.
echo Fichier latest.json actualise sur le disque.
echo Dans GitHub Desktop : Commit to main, puis Push origin.
pause
exit /b 0
