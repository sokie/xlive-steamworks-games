# Zips every release/<name>/ folder into release/zips/<name>-<version>.zip, then packs all of them
# into one xlive-steamworks-games-<version>.zip and writes SHA256SUMS.txt next to the zips.
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
if (Test-Path $zips) { Remove-Item -Recurse -Force $zips }
New-Item -ItemType Directory -Force -Path $zips | Out-Null

$folders = Get-ChildItem $ReleaseDir -Directory | Where-Object { $_.Name -ne "zips" }
if (-not $folders) { throw "no release folders under $ReleaseDir, build first" }
foreach ($folder in $folders) {
	$out = Join-Path $zips "$($folder.Name)-$Version.zip"
	Compress-Archive -Path (Join-Path $folder.FullName "*") -DestinationPath $out
	Write-Host "wrote $out"
}

# The bundle keeps one folder per release, so a player unzips it and picks their game.
$bundle = Join-Path $zips "xlive-steamworks-games-$Version.zip"
Compress-Archive -Path ($folders | ForEach-Object { $_.FullName }) -DestinationPath $bundle
Write-Host "wrote $bundle"

$sha = [System.Security.Cryptography.SHA256]::Create()
$sums = Get-ChildItem $zips -Filter *.zip | ForEach-Object {
	$stream = [System.IO.File]::OpenRead($_.FullName)
	try { $hash = ($sha.ComputeHash($stream) | ForEach-Object { $_.ToString("x2") }) -join "" }
	finally { $stream.Close() }
	"{0}  {1}" -f $hash, $_.Name
}
Set-Content -Path (Join-Path $zips "SHA256SUMS.txt") -Value $sums -Encoding ascii
Write-Host "wrote SHA256SUMS.txt"
