@echo off
setlocal EnableExtensions DisableDelayedExpansion
pushd "%~dp0" || exit /b 1

echo ==============================================
echo ACID303 SOLO - CREATION INSTALLATEUR WINDOWS
echo ==============================================
echo.

rem Etape 1 : le VST3 doit deja avoir ete compile par BUILD_WINDOWS.bat.
set "BUNDLE=%~dp0..\build\Acid303Solo_artefacts\Release\VST3\Acid303 Solo.vst3"
if not exist "%BUNDLE%\Contents\" (
  echo ERREUR : VST3 compile introuvable.
  echo Lancez BUILD_WINDOWS.bat a la racine du projet.
  echo Dossier attendu : "%BUNDLE%"
  goto :error
)

rem Etape 2 : preparer une COPIE FRAICHE du VST3 compile.
rem Ne pas laisser de fichiers d'une ancienne version dans payload.
if exist "%~dp0payload\Acid303 Solo.vst3\" rmdir /S /Q "%~dp0payload\Acid303 Solo.vst3"
if not exist "%~dp0payload\Acid303 Solo.vst3\" mkdir "%~dp0payload\Acid303 Solo.vst3"
xcopy "%BUNDLE%\*" "%~dp0payload\Acid303 Solo.vst3\" /E /I /Y /Q >nul
if errorlevel 1 (
  echo ERREUR : copie des fichiers VST3 impossible.
  goto :error
)

rem Etape 3 : rechercher ISCC.exe dans le PATH, les dossiers systeme et le profil utilisateur.
set "ISCC="
for /f "delims=" %%I in ('where ISCC.exe 2^>nul') do if not defined ISCC set "ISCC=%%I"
if defined ISCC if not exist "%ISCC%" set "ISCC="
if not defined ISCC if exist "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" set "ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if not defined ISCC if exist "%ProgramFiles%\Inno Setup 6\ISCC.exe" set "ISCC=%ProgramFiles%\Inno Setup 6\ISCC.exe"
if not defined ISCC if exist "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" set "ISCC=%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe"
if not defined ISCC if exist "%LOCALAPPDATA%\Inno Setup 6\ISCC.exe" set "ISCC=%LOCALAPPDATA%\Inno Setup 6\ISCC.exe"
if not defined ISCC if exist "%ProgramFiles(x86)%\Inno Setup 7\ISCC.exe" set "ISCC=%ProgramFiles(x86)%\Inno Setup 7\ISCC.exe"
if not defined ISCC if exist "%ProgramFiles%\Inno Setup 7\ISCC.exe" set "ISCC=%ProgramFiles%\Inno Setup 7\ISCC.exe"
if not defined ISCC if exist "%LOCALAPPDATA%\Programs\Inno Setup 7\ISCC.exe" set "ISCC=%LOCALAPPDATA%\Programs\Inno Setup 7\ISCC.exe"

rem Etape 4 : si Inno est ailleurs, afficher une fenetre pour choisir ISCC.exe.
if not defined ISCC (
  echo Inno Setup semble installe, mais son compilateur ISCC.exe n'a pas ete trouve.
  echo Selectionnez le fichier ISCC.exe dans la fenetre suivante.
  set "TEMP_ISCC=%TEMP%\acid303_iscc_%RANDOM%%RANDOM%.txt"
  powershell.exe -NoProfile -Sta -ExecutionPolicy Bypass -File "%~dp0SELECTIONNER_ISCC.ps1" > "%TEMP_ISCC%"
  if exist "%TEMP_ISCC%" (
    for /f "usebackq delims=" %%I in ("%TEMP_ISCC%") do if not defined ISCC set "ISCC=%%I"
    del /q "%TEMP_ISCC%" >nul 2>&1
  )
)

if not defined ISCC goto :notfound
if not exist "%ISCC%" goto :notfound

echo.
echo Compilateur Inno Setup detecte :
echo "%ISCC%"
echo.
"%ISCC%" "%~dp0Acid303Solo.iss"
if errorlevel 1 goto :error

echo.
echo INSTALLATEUR CREE AVEC SUCCES.
echo Ouvrez le dossier : %~dp0Output
echo Testez le programme en fermant FL Studio auparavant.
pause
popd
exit /b 0

:notfound
echo.
echo ERREUR : ISCC.exe non localise.
echo Dans le menu Demarrer, recherchez "Inno Setup Compiler".
echo Depuis Inno Setup : File / Open / Acid303Solo.iss, puis Build / Compile.
echo Aucun besoin de desinstaller ou de reinstaller Inno Setup.
:error
pause
popd
exit /b 1
