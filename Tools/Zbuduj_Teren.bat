@echo off
rem Importuje tekstury terenu z Art\Teren (tylko brakujace) i buduje od nowa material M_Teren (Tools\ue_teren.py).
rem Zamknij edytor z tym projektem. Zeby podmienic teksture, usun jej .uasset w Content\Swiat\Teren.
if exist "%~dp0..\Content\Swiat\Teren\M_Teren.uasset" del "%~dp0..\Content\Swiat\Teren\M_Teren.uasset"
if exist "%~dp0..\Content\Swiat\Teren\M_MapaRzeki.uasset" del "%~dp0..\Content\Swiat\Teren\M_MapaRzeki.uasset"
"D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%~dp0..\grarts2d.uproject" -run=pythonscript -script="%~dp0ue_teren.py" -unattended -nosplash -nullrhi > "%~dp0..\Saved\Zbuduj_Teren.log" 2>&1
findstr /c:"executed successfully" /c:"Traceback" /c:"Assertion" "%~dp0..\Saved\Zbuduj_Teren.log"
if not "%1"=="nopause" pause
