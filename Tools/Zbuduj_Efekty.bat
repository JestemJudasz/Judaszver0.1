@echo off
rem Efekty wojny: tekstury i siatki czastek (Tools\efekty.py -> Art\Efekty), import do Unreala z materialami (Tools\ue_efekty.py).
rem Zamknij edytor z tym projektem. Caly Content\Swiat\Efekty jest kasowany i budowany od nowa. Najpierw Zbuduj_Budynki (tekstura drewna).
set PY=D:\UE_5.8\Engine\Binaries\ThirdParty\Python3\Win64\python.exe
set PYTHONPATH=%~dp0_py;%~dp0
"%PY%" "%~dp0efekty.py" || exit /b 1
if exist "%~dp0..\Content\Swiat\Efekty" rmdir /s /q "%~dp0..\Content\Swiat\Efekty"
"D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%~dp0..\grarts2d.uproject" -run=pythonscript -script="%~dp0ue_efekty.py" -unattended -nosplash -nullrhi > "%~dp0..\Saved\Zbuduj_Efekty.log" 2>&1
findstr /c:"executed successfully" /c:"Traceback" /c:"Assertion" /c:"Efekty:" "%~dp0..\Saved\Zbuduj_Efekty.log"
if not "%1"=="nopause" pause
