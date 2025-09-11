@echo off
cd /d "%~dp0"
set INSTALLER=BLEPassMan\Output\blepassman_install.exe
set ZIPFILE=install\blepassman_install.zip
"c:\Program Files\7-Zip\7z.exe" a "%ZIPFILE%" "%INSTALLER%"
echo Installer zippato in %ZIPFILE%
pause