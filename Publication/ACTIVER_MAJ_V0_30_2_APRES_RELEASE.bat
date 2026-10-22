@echo off
setlocal
cd /d "%~dp0"
echo ====================================================
echo ACID303 SOLO - ACTIVER ANNONCE VERSION 0.30.2
 echo ====================================================
echo.
echo AVANT DE CONTINUER :
echo - Avez-vous publie la Release GitHub v0.30.2 ?
echo - Contient-elle l'EXE de la bonne version, teste sur Windows ?
echo.
choice /C ON /M "La Release est publiee et testee"
if errorlevel 2 (
 echo Rien n'a ete modifie.
 pause
 exit /b 1
)
copy /Y "%~dp0ANNONCE_V0_30_2_APRES_PUBLICATION.json" "%~dp0latest.json" >nul
if errorlevel 1 (
 echo ERREUR pendant la copie.
 pause
 exit /b 1
)
echo.
echo Publication/latest.json annonce maintenant la version 0.30.2.
echo Dans GitHub Desktop, faites COMMIT puis PUSH ORIGIN.
echo.
pause
exit /b 0
