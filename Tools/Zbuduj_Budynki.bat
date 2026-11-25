@echo off
rem Robi od nowa budynki: Tools\budynki.py (Art\Budynki\*.glb + tekstury), potem import do Unreala (Tools\ue_budynki.py).
rem Zamknij edytor z tym projektem. Caly Content\Swiat\Budynki jest kasowany i budowany od nowa.
set PY=D:\UE_5.8\Engine\Binaries\ThirdParty\Python3\Win64\python.exe
set PYTHONPATH=%~dp0_py
"%PY%" "%~dp0budynki.py" || exit /b 1
"%PY%" "%~dp0ikony.py" || exit /b 1
if exist "%~dp0..\Content\Swiat\Budynki" rmdir /s /q "%~dp0..\Content\Swiat\Budynki"
"D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%~dp0..\grarts2d.uproject" -run=pythonscript -script="%~dp0ue_budynki.py" -unattended -nosplash -nullrhi > "%~dp0..\Saved\Zbuduj_Budynki.log" 2>&1
findstr /c:"executed successfully" /c:"Traceback" /c:"Assertion" /c:"Budynki:" "%~dp0..\Saved\Zbuduj_Budynki.log"
if not "%1"=="nopause" pause
