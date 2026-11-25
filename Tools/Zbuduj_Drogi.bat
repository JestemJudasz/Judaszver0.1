@echo off
rem Drogi: tekstury (Tools\drogi.py -> Art\Drogi) i import do Unreala z materialem M_Droga (Tools\ue_drogi.py).
rem Zamknij edytor z tym projektem. Caly Content\Swiat\Drogi jest kasowany i budowany od nowa.
set PY=D:\UE_5.8\Engine\Binaries\ThirdParty\Python3\Win64\python.exe
set PYTHONPATH=%~dp0_py
"%PY%" "%~dp0drogi.py" || exit /b 1
if exist "%~dp0..\Content\Swiat\Drogi" rmdir /s /q "%~dp0..\Content\Swiat\Drogi"
"D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%~dp0..\grarts2d.uproject" -run=pythonscript -script="%~dp0ue_drogi.py" -unattended -nosplash -nullrhi > "%~dp0..\Saved\Zbuduj_Drogi.log" 2>&1
findstr /c:"executed successfully" /c:"Traceback" /c:"Assertion" /c:"Drogi:" "%~dp0..\Saved\Zbuduj_Drogi.log"
if not "%1"=="nopause" pause
