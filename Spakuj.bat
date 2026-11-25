@echo off
rem Pakuje gre grarts2d do jednego pliku ZIP dla kolegi. NAJPIERW ZAMKNIJ UNREAL EDITOR.
rem Wynik: Paczka\grarts2d.zip  (kolega rozpakowuje i uruchamia grarts2d.exe)
setlocal
set PROJ=%~dp0grarts2d.uproject
set OUT=%~dp0Paczka
set LOG=%~dp0Pakowanie.log
if "%KONF%"=="" set KONF=Shipping
echo.
echo  Pakuje grarts2d (%KONF%, Win64). To potrwa od kilkunastu minut do godziny...
echo.
if exist "%OUT%" rmdir /s /q "%OUT%"
call "D:\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="%PROJ%" -platform=Win64 -clientconfig=%KONF% -build -cook -stage -pak -iostore -package -archive -archivedirectory="%OUT%" -prereqs -nodebuginfo -utf8output -unattended > "%LOG%" 2>&1
set WYNIK=%ERRORLEVEL%
if not %WYNIK%==0 goto blad
rem Dane terenu (Data\*) gra czyta z dysku, nie z paczki - kopiujemy obok exe.
xcopy /e /i /y /q "%~dp0Data" "%OUT%\Windows\grarts2d\Data" >> "%LOG%"
echo  Pakuje do ZIP...
rem tar z Windows robi ZIP bez limitu 2 GB na plik (Compress-Archive go ma).
tar -a -c -f "%OUT%\grarts2d.zip" -C "%OUT%\Windows" . >> "%LOG%" 2>&1
if errorlevel 1 (set WYNIK=1& goto blad)
echo.
echo  GOTOWE: %OUT%\grarts2d.zip
goto koniec
:blad
findstr /i /c:"error" "%LOG%"
echo.
echo  BLAD PAKOWANIA - napisz Claude "gotowe", przeczyta plik Pakowanie.log sam
:koniec
echo.
if not "%1"=="nopause" pause
exit /b %WYNIK%
