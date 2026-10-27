@echo off
cd /d "%~dp0"
python VERIFIER_V3.py
if errorlevel 1 (echo VERIFICATION V3 ECHEC) else (echo VERIFICATION V3 OK)
pause
