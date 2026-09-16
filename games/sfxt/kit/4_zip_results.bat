@echo off
setlocal
cd /d "%~dp0"

rem The game logs into its own folder, copy them in while the install record still names it.
set "GAMEDIR="
if exist results\install.txt (
	for /f "usebackq tokens=1,* delims==" %%A in ("results\install.txt") do (
		if "%%A"=="gamedir" set "GAMEDIR=%%B"
	)
)
if not "%GAMEDIR%"=="" (
	copy /y "%GAMEDIR%\xlive_steamworks.log" "results\game_xlive_steamworks.log" >nul 2>nul
	copy /y "%GAMEDIR%\steam_api_shim.log" "results\game_steam_api_shim.log" >nul 2>nul
)

if not exist results\probe_report.txt if not exist results\game_xlive_steamworks.log (
	echo No results yet. Run 1_run_probe.bat or play the game after 2_install.bat first.
	pause
	exit /b 1
)

set "OUT=%~dp0sfxt-results.zip"
del "%OUT%" 2>nul

echo Packaging results ...
powershell -NoProfile -Command "Compress-Archive -Path 'results\*' -DestinationPath '%OUT%' -Force"

if exist "%OUT%" (
	echo.
	echo Wrote %OUT%
	echo Please send that one file back. Thank you!
) else (
	echo Could not create the zip. Please send the whole 'results' folder instead.
)
pause
