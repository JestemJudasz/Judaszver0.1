@echo off
rem Dzwieki: import Art\Dzwieki\**\*.wav do Unreala (Tools\ue_dzwieki.py). Zamknij edytor z tym projektem.
rem Caly Content\Dzwieki jest kasowany i budowany od nowa.
if exist "%~dp0..\Content\Dzwieki" rmdir /s /q "%~dp0..\Content\Dzwieki"
"D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%~dp0..\grarts2d.uproject" -run=pythonscript -script="%~dp0ue_dzwieki.py" -unattended -nosplash -nullrhi > "%~dp0..\Saved\Zbuduj_Dzwieki.log" 2>&1
findstr /c:"executed successfully" /c:"Traceback" /c:"Assertion" /c:"Dzwieki:" "%~dp0..\Saved\Zbuduj_Dzwieki.log"
if not "%1"=="nopause" pause
