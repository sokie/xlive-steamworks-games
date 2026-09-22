# Imports an existing Virtua Tennis 4 save.
# The game names a save after the profile's XUID, so a Steam profile starts empty until one is copied in.
param(
	[string]$SaveDir = (Join-Path ([Environment]::GetFolderPath('MyDocuments')) 'Virtua Tennis 4\Saved Games'),
	[string]$SteamId64 = ''
)

$ErrorActionPreference = 'Stop'

function Get-MostRecentSteamId64 {
	$steam = (Get-ItemProperty -Path 'HKCU:\Software\Valve\Steam' -ErrorAction SilentlyContinue).SteamPath
	if (-not $steam) { return '' }
	$vdf = Join-Path $steam 'config\loginusers.vdf'
	if (-not (Test-Path $vdf)) { return '' }
	$text = Get-Content $vdf -Raw
	$newest = ''
	$newestStamp = -1
	foreach ($user in [regex]::Matches($text, '"(7656\d{13})"\s*\{([^}]*)\}')) {
		$block = $user.Groups[2].Value
		if ($block -match '"MostRecent"\s*"1"') { return $user.Groups[1].Value }
		if ($block -match '"Timestamp"\s*"(\d+)"' -and [long]$Matches[1] -gt $newestStamp) {
			$newestStamp = [long]$Matches[1]
			$newest = $user.Groups[1].Value
		}
	}
	return $newest
}

if (-not $SteamId64) { $SteamId64 = Get-MostRecentSteamId64 }
if (-not $SteamId64) { throw 'no Steam account found, pass -SteamId64 with your SteamID64' }

$accountId = [uint64]$SteamId64 - 76561197960265728
$xuid = '{0:x16}' -f ([uint64]0x0009000000000000 + $accountId)
$target = Join-Path $SaveDir "$xuid.sav"

if (-not (Test-Path $SaveDir)) {
	Write-Host "no save folder at $SaveDir, nothing to import"
	exit 0
}
if (Test-Path $target) {
	Write-Host "the Steam profile already has a save, $target, nothing done"
	exit 0
}
$candidates = @(Get-ChildItem $SaveDir -Filter *.sav | Where-Object { $_.Name -ne "$xuid.sav" } | Sort-Object LastWriteTime -Descending)
if ($candidates.Count -eq 0) {
	Write-Host "no save to import in $SaveDir"
	exit 0
}

$pick = $candidates[0]
if ($candidates.Count -gt 1) {
	for ($i = 0; $i -lt $candidates.Count; $i++) {
		'{0,2}: {1}  {2:yyyy-MM-dd HH:mm}  {3} bytes' -f ($i + 1), $candidates[$i].Name, $candidates[$i].LastWriteTime, $candidates[$i].Length
	}
	$choice = [int](Read-Host "which save to import (1-$($candidates.Count))")
	if ($choice -lt 1 -or $choice -gt $candidates.Count) { throw "no save numbered $choice" }
	$pick = $candidates[$choice - 1]
}

Copy-Item -LiteralPath $pick.FullName -Destination $target
Write-Host "copied $($pick.Name) to $xuid.sav, the original stays in place"
