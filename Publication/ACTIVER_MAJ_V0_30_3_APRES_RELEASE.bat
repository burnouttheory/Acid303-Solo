@echo off
setlocal
cd /d "%~dp0"
echo ===================================================
echo ACID303 SOLO : ACTIVER LA MISE A JOUR v0.30.3
echo ===================================================
echo.
echo AVANT DE CONTINUER, verifiez :
echo 1. Le code v0.30.3 est publie sur la branche main.
echo 2. La Release v0.30.3 contient l'installateur teste.
echo.
choice /C ON /M "Confirmer que l'installateur est disponible sur GitHub"
if errorlevel 2 (
 echo Aucune modification effectuee.
 pause
 exit /b 1
)
copy /Y "%~dp0ANNONCE_V0_30_3_APRES_PUBLICATION.json" "%~dp0latest.json" >nul
if errorlevel 1 (
 echo ERREUR pendant la copie.
 pause
 exit /b 1
)
echo.
echo Fichier latest.json modifie en version 0.30.3.
echo Retournez dans GitHub Desktop : Commit puis Push origin.
pause
