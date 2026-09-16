@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

if not exist results\install.txt (
	echo Nothing to restore: 2_install.bat has not been run.
	pause
	exit /b 1
)
set "GAMEDIR="
for /f "usebackq tokens=1,* delims==" %%A in ("results\install.txt") do (
	if "%%A"=="gamedir" set "GAMEDIR=%%B"
)
if "%GAMEDIR%"=="" (
	echo results\install.txt names no game folder.
	pause
	exit /b 1
)
echo Game folder: %GAMEDIR%

rem Files the kit replaced come back from their backups, files it added are deleted.
for /f "usebackq tokens=1,* delims==" %%A in ("results\install.txt") do (
	if "%%A"=="replaced" (
		if exist "%GAMEDIR%\%%B.orig-backup" (
			copy /y "%GAMEDIR%\%%B.orig-backup" "%GAMEDIR%\%%B" >nul
			del "%GAMEDIR%\%%B.orig-backup" >nul
			echo   restored %%B
		)
	)
	if "%%A"=="added" (
		del "%GAMEDIR%\%%B" 2>nul
		echo   removed %%B
	)
)
del "%GAMEDIR%\xlive_steamworks.log" 2>nul
del "%GAMEDIR%\steam_api_shim.log" 2>nul
del results\install.txt
echo Done. The game is back to its original files.
pause
