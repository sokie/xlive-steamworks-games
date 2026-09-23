# Imports the save data of another Street Fighter X Tekken profile.
# The game keeps one folder per profile XUID, so a Steam profile starts empty until one is copied in.
param(
	[string]$SaveDir = (Join-Path ([Environment]::GetFolderPath('MyDocuments')) 'CAPCOM\SFTK\savedata'),
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

function Get-NewestWrite([string]$dir) {
	$file = Get-ChildItem $dir -File -Recurse | Sort-Object LastWriteTime -Descending | Select-Object -First 1
	if ($file) { return $file.LastWriteTime } else { return [datetime]::MinValue }
}

if (-not $SteamId64) { $SteamId64 = Get-MostRecentSteamId64 }
if (-not $SteamId64) { throw 'no Steam account found, pass -SteamId64 with your SteamID64' }

$accountId = [uint64]$SteamId64 - 76561197960265728
$xuid = '{0:x16}' -f ([uint64]0x0009000000000000 + $accountId)
$targetDir = Join-Path $SaveDir $xuid

if (-not (Test-Path $SaveDir)) {
	Write-Host "no save folder at $SaveDir, nothing to import"
	exit 0
}
if ((Test-Path $targetDir) -and @(Get-ChildItem $targetDir -File -Recurse).Count -gt 0) {
	Write-Host "the Steam profile already has save data in $targetDir, nothing done"
	exit 0
}
$candidates = @(Get-ChildItem $SaveDir -Directory | Where-Object { $_.Name -ne $xuid -and @(Get-ChildItem $_.FullName -File -Recurse).Count -gt 0 } |
	Sort-Object { Get-NewestWrite $_.FullName } -Descending)
if ($candidates.Count -eq 0) {
	Write-Host "no save data to import under $SaveDir"
	exit 0
}

$pick = $candidates[0]
if ($candidates.Count -gt 1) {
	for ($i = 0; $i -lt $candidates.Count; $i++) {
		$files = @(Get-ChildItem $candidates[$i].FullName -File -Recurse).Count
		'{0,2}: {1}  {2:yyyy-MM-dd HH:mm}  {3} files' -f ($i + 1), $candidates[$i].Name, (Get-NewestWrite $candidates[$i].FullName), $files
	}
	$choice = [int](Read-Host "which profile's save data to import (1-$($candidates.Count))")
	if ($choice -lt 1 -or $choice -gt $candidates.Count) { throw "no profile numbered $choice" }
	$pick = $candidates[$choice - 1]
}

New-Item -ItemType Directory -Force -Path $targetDir | Out-Null
Copy-Item -Path (Join-Path $pick.FullName '*') -Destination $targetDir -Recurse
Write-Host "copied the save data of $($pick.Name) to $xuid, the original stays in place"
