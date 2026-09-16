# Zips every release/<name>/ folder into release/zips/<name>-<version>.zip.
param(
	[string]$Version = "",
	[string]$ReleaseDir = ""
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
if (-not $ReleaseDir) { $ReleaseDir = Join-Path $root "release" }
if (-not (Test-Path $ReleaseDir)) { throw "no release folder at $ReleaseDir, build first" }
if (-not $Version) {
	$Version = (git -C $root describe --tags --always 2>$null)
	if (-not $Version) { $Version = Get-Date -Format "yyyyMMdd" }
}

$zips = Join-Path $ReleaseDir "zips"
New-Item -ItemType Directory -Force -Path $zips | Out-Null
Get-ChildItem $ReleaseDir -Directory | Where-Object { $_.Name -ne "zips" } | ForEach-Object {
	$out = Join-Path $zips "$($_.Name)-$Version.zip"
	if (Test-Path $out) { Remove-Item -Force $out }
	Compress-Archive -Path (Join-Path $_.FullName "*") -DestinationPath $out
	Write-Host "wrote $out"
}
