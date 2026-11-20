@echo off
rem Kompilacja kodu gry grarts2d. NAJPIERW ZAMKNIJ UNREAL EDITOR.
echo.
echo  Kompiluje grarts2d (edytor musi byc zamkniety)...
echo.
call "D:\UE_5.8\Engine\Build\BatchFiles\Build.bat" grarts2dEditor Win64 Development -Project="%~dp0grarts2d.uproject" -WaitMutex -NoHotReloadFromIDE > "%~dp0Kompilacja.log" 2>&1
set WYNIK=%ERRORLEVEL%
findstr /i /c:"error" "%~dp0Kompilacja.log"
echo.
if %WYNIK%==0 (echo  GOTOWE - mozesz otworzyc grarts2d.uproject) else (echo  BLAD KOMPILACJI - napisz Claude "gotowe", przeczyta plik Kompilacja.log sam)
echo.
if not "%1"=="nopause" pause
exit /b %WYNIK%
