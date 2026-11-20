@echo off
rem Przelicza dane z Dane_zrodlowe na Data/europa_*.  Python z UE + biblioteki w Tools\_py.
set PYTHONPATH=%~dp0_py
"D:\UE_5.8\Engine\Binaries\ThirdParty\Python3\Win64\python.exe" "%~dp0dem_europa.py"
if not "%1"=="nopause" pause
