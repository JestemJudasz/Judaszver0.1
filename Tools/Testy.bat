@echo off
rem Testy automatyczne grarts2d (Source\grarts2d\Testy) bez uruchamiania gry. Zamknij edytor; najpierw Kompiluj.bat.
rem Pelny log: Saved\Testy.log (-abslog, bo inaczej silnik pisze do Saved\Logs\grarts2d.log).
"D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%~dp0..\grarts2d.uproject" -ExecCmds="Automation RunTests grarts2d" -TestExit="Automation Test Queue Empty" -unattended -nullrhi -nosplash -nosound -abslog="%~dp0..\Saved\Testy.log" > nul 2>&1
findstr /c:"Test Completed" /c:"Expected" /c:"No automation tests" "%~dp0..\Saved\Testy.log"
if not "%1"=="nopause" pause
