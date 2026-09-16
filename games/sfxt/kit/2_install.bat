@echo off
setlocal
cd /d "%~dp0"
if not exist results mkdir results

set "GAMEDIR=%~1"
if "%GAMEDIR%"=="" (
	echo Enter the full path to your Street Fighter X Tekken folder
	echo   ^(the folder that contains SFTK.exe^), then press Enter.
	echo You can also drag that folder, or SFTK.exe itself, onto this .bat file.
	set /p "GAMEDIR=Game folder: "
)
set "GAMEDIR=%GAMEDIR:"=%"

rem Given SFTK.exe instead of the folder, use its folder.
if exist "%GAMEDIR%\SFTK.exe" goto have
for %%I in ("%GAMEDIR%") do (
	if /I "%%~nxI"=="SFTK.exe" ( set "GAMEDIR=%%~dpI" & goto trail )
)
echo.
echo Could not find SFTK.exe using: %GAMEDIR%
echo Run again and give the folder that contains SFTK.exe.
pause
exit /b 1

:trail
if "%GAMEDIR:~-1%"=="\" set "GAMEDIR=%GAMEDIR:~0,-1%"
:have
echo.
echo Game folder: %GAMEDIR%

rem The two PC builds differ in exe size, each takes a different set of files.
set "BUILD="
for %%I in ("%GAMEDIR%\SFTK.exe") do (
	if "%%~zI"=="14295096" set "BUILD=gfwl"
	if "%%~zI"=="14326352" set "BUILD=steam"
)
if "%BUILD%"=="" (
	echo This SFTK.exe is neither the GFWL build nor the Steam build this kit knows.
	echo Nothing was changed.
	pause
	exit /b 1
)
echo Detected the %BUILD% build.

rem Back up every file the kit replaces. A missing original is remembered so restore deletes it.
> results\install.txt echo build=%BUILD%
>> results\install.txt echo gamedir=%GAMEDIR%
for %%F in (xlive.dll steam_api.dll steam_api_real.dll xlive_steamworks.json) do (
	if exist "%BUILD%\%%F" (
		if exist "%GAMEDIR%\%%F" (
			if not exist "%GAMEDIR%\%%F.orig-backup" (
				copy /y "%GAMEDIR%\%%F" "%GAMEDIR%\%%F.orig-backup" >nul
				echo   backed up %%F
			)
			>> results\install.txt echo replaced=%%F
		) else (
			>> results\install.txt echo added=%%F
		)
		copy /y "%BUILD%\%%F" "%GAMEDIR%\%%F" >nul
	)
)
del "%GAMEDIR%\xlive_steamworks.log" 2>nul
del "%GAMEDIR%\steam_api_shim.log" 2>nul

echo.
echo Installed. Start the game the usual way, play a little, try the online,
echo ranked and lobby menus, then quit normally.
echo Afterwards run 4_zip_results.bat, it collects the game's logs from the folder.
echo To put the game back exactly as it was, run 3_restore.bat.
pause
