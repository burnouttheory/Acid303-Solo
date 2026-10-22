@echo off
setlocal
cd /d "%~dp0"
echo ============================================
echo PUBLIER UNE ANNONCE DE MISE A JOUR ACID303
 echo ============================================
set /p "VERSION=Numero de version (exemple 0.31.0) : "
set /p "LIEN=Lien public du dossier OneDrive : "
set /p "NOUVEAUTES=Description courte : "
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0GENERER_ANNONCE_MAJ.ps1" -Version "%VERSION%" -OneDriveLink "%LIEN%" -Notes "%NOUVEAUTES%"
if errorlevel 1 (
 echo ERREUR : verifiez la version ou le lien fourni.
 pause
 exit /b 1
)
echo.
echo ETAPE FINALE : deposer latest.json sur votre hebergement HTTPS public.
pause
