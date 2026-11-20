@echo off
rem Robi od nowa drzewa i krzaki: Tools\natura_drzewa.py (Art\Natura\*.glb), potem import do Unreala (Tools\ue_natura.py).
rem Zamknij edytor z tym projektem. Caly Content\Swiat\Natura jest kasowany i budowany od nowa.
set PY=D:\UE_5.8\Engine\Binaries\ThirdParty\Python3\Win64\python.exe
set PYTHONPATH=%~dp0_py
"%PY%" "%~dp0natura_drzewa.py" || exit /b 1
if exist "%~dp0..\Content\Swiat\Natura" rmdir /s /q "%~dp0..\Content\Swiat\Natura"
"D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%~dp0..\grarts2d.uproject" -run=pythonscript -script="%~dp0ue_natura.py" -unattended -nosplash -nullrhi > "%~dp0..\Saved\Zbuduj_Nature.log" 2>&1
findstr /c:"executed successfully" /c:"Traceback" /c:"Assertion" /c:"Natura:" "%~dp0..\Saved\Zbuduj_Nature.log"
if not "%1"=="nopause" pause
