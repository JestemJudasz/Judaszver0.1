@echo off
rem Robi od nowa ludzikow: Tools\ludzie.py (Art\Ludzie\*.glb), potem import do Unreala (Tools\ue_ludzie.py).
rem Zamknij edytor z tym projektem. Caly Content\Swiat\Ludzie jest kasowany i budowany od nowa.
set PY=D:\UE_5.8\Engine\Binaries\ThirdParty\Python3\Win64\python.exe
set PYTHONPATH=%~dp0_py
"%PY%" "%~dp0ludzie.py" || exit /b 1
if exist "%~dp0..\Content\Swiat\Ludzie" rmdir /s /q "%~dp0..\Content\Swiat\Ludzie"
"D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%~dp0..\grarts2d.uproject" -run=pythonscript -script="%~dp0ue_ludzie.py" -unattended -nosplash -nullrhi > "%~dp0..\Saved\Zbuduj_Ludzi.log" 2>&1
findstr /c:"executed successfully" /c:"Traceback" /c:"Assertion" /c:"Ludzie:" "%~dp0..\Saved\Zbuduj_Ludzi.log"
if not "%1"=="nopause" pause
