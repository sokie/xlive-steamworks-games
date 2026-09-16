# Packs release\zips\sfxt-owner-kit.zip for an owner of the game: the probe, both release folders
# and the numbered scripts that install into either build, collect logs and restore the game.
# Build the repository with -DXLS_BUILD_TESTS=ON first (the probe exe comes from the SDK build).
param(
	[string]$Out = ""
)

$ErrorActionPreference = "Stop"
$root = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path
$release = Join-Path $root "release"
$sdkBin = Join-Path $root "sdk\bin"
if (-not $Out) { $Out = Join-Path $release "zips\sfxt-owner-kit.zip" }

foreach ($path in @("sfxt-gfwl\xlive.dll", "sfxt-steam\steam_api.dll", "sfxt-steam\steam_api_real.dll")) {
	if (-not (Test-Path (Join-Path $release $path))) { throw "missing release\$path, build the repository first" }
}
if (-not (Test-Path (Join-Path $sdkBin "xlive_smoke.exe"))) { throw "missing sdk\bin\xlive_smoke.exe, configure with -DXLS_BUILD_TESTS=ON" }

$stage = Join-Path $env:TEMP "sfxt-owner-kit"
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
foreach ($dir in @("probe", "gfwl", "steam", "results")) {
	New-Item -ItemType Directory -Path (Join-Path $stage $dir) | Out-Null
}

# The probe, logging into results.
foreach ($file in @("xlive_smoke.exe", "xlive.dll", "steam_api.dll")) {
	Copy-Item (Join-Path $sdkBin $file) (Join-Path $stage "probe")
}
[IO.File]::WriteAllText((Join-Path $stage "probe\steam_appid.txt"), "209120")
[IO.File]::WriteAllText((Join-Path $stage "probe\xlive_steamworks.json"), '{ "log": { "level": "debug", "path": "..\\results\\probe_debug.log" }, "steam": { "required": false }, "leaderboards": { "create_if_missing": false } }')

# The two release folders, logging at debug for the report.
Copy-Item (Join-Path $release "sfxt-gfwl\*") (Join-Path $stage "gfwl")
Copy-Item (Join-Path $release "sfxt-steam\*") (Join-Path $stage "steam")
foreach ($dir in @("gfwl", "steam")) {
	$json = Join-Path $stage "$dir\xlive_steamworks.json"
	$config = Get-Content $json -Raw | ConvertFrom-Json
	$config.log.level = "debug"
	$config | ConvertTo-Json -Depth 10 | Set-Content $json -Encoding UTF8
}

Copy-Item (Join-Path $PSScriptRoot "*.bat") $stage
Copy-Item (Join-Path $PSScriptRoot "README.txt") $stage
[IO.File]::WriteAllText((Join-Path $stage "results\PUT_RESULTS_HERE.txt"), "The scripts write the probe report, the game logs and system info here.")

# Force CRLF on everything cmd.exe or Notepad reads.
Get-ChildItem $stage -Recurse -Include *.bat, *.txt | ForEach-Object {
	$text = [IO.File]::ReadAllText($_.FullName) -replace "`r`n", "`n" -replace "`n", "`r`n"
	[IO.File]::WriteAllText($_.FullName, $text)
}

New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
if (Test-Path $Out) { Remove-Item -Force $Out }
Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $Out
Remove-Item -Recurse -Force $stage
Write-Host "wrote $Out"
