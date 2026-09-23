# Imports an existing Lost Planet 2 save.
# The game keeps one save folder per gamertag, so a Steam profile starts empty until one is copied in.
param(
	[string]$SaveRoot = (Join-Path ([Environment]::GetFolderPath('MyDocuments')) 'CAPCOM\LOST PLANET 2'),
	[string]$Gamertag = ''
)

$ErrorActionPreference = 'Stop'
$saveName = 'Lostplanet2.Lostplanet2Save-capcom'

function Get-SteamPersonaName {
	$steam = (Get-ItemProperty -Path 'HKCU:\Software\Valve\Steam' -ErrorAction SilentlyContinue).SteamPath
	if (-not $steam) { return '' }
	$vdf = Join-Path $steam 'config\loginusers.vdf'
	if (-not (Test-Path $vdf)) { return '' }
	$text = Get-Content $vdf -Raw -Encoding UTF8
	$best = ''
	$bestStamp = -1
	foreach ($user in [regex]::Matches($text, '"(7656\d{13})"\s*\{([^}]*)\}')) {
		$block = $user.Groups[2].Value
		if ($block -notmatch '"PersonaName"\s*"((?:[^"\\]|\\.)*)"') { continue }
		$name = $Matches[1] -replace '\\(.)', '$1'
		if ($block -match '"MostRecent"\s*"1"') { return $name }
		if ($block -match '"Timestamp"\s*"(\d+)"' -and [long]$Matches[1] -gt $bestStamp) {
			$bestStamp = [long]$Matches[1]
			$best = $name
		}
	}
	return $best
}

# The wrapper's gamertag: the persona cut to 15 characters, control characters dropped, anything
# outside ASCII replaced by an underscore, trailing spaces trimmed.
function Get-GamertagFromPersona([string]$persona) {
	$tag = ''
	foreach ($c in $persona.ToCharArray()) {
		if ($tag.Length -ge 15) { break }
		$code = [int]$c
		if ($code -lt 0x20) { continue }
		if ($code -lt 0x7F) { $tag += $c } else { $tag += '_' }
	}
	return $tag.TrimEnd(' ')
}

if (-not $Gamertag) {
	$persona = Get-SteamPersonaName
	if (-not $persona) { throw 'no Steam account found, pass -Gamertag with the name the game shows for you' }
	$Gamertag = Get-GamertagFromPersona $persona
}
$targetDir = Join-Path $SaveRoot $Gamertag
$target = Join-Path $targetDir $saveName

if (-not (Test-Path $SaveRoot)) {
	Write-Host "no save folder at $SaveRoot, nothing to import"
	exit 0
}
if (Test-Path $target) {
	Write-Host "the Steam profile $Gamertag already has a save, $target, nothing done"
	exit 0
}
$candidates = @(Get-ChildItem $SaveRoot -Directory | Where-Object { $_.Name -ne $Gamertag -and (Test-Path (Join-Path $_.FullName $saveName)) } |
	ForEach-Object { Get-Item (Join-Path $_.FullName $saveName) } | Sort-Object LastWriteTime -Descending)
if ($candidates.Count -eq 0) {
	Write-Host "no save to import under $SaveRoot"
	exit 0
}

$pick = $candidates[0]
if ($candidates.Count -gt 1) {
	for ($i = 0; $i -lt $candidates.Count; $i++) {
		'{0,2}: {1}  {2:yyyy-MM-dd HH:mm}  {3} bytes' -f ($i + 1), $candidates[$i].Directory.Name, $candidates[$i].LastWriteTime, $candidates[$i].Length
	}
	$choice = [int](Read-Host "which profile's save to import (1-$($candidates.Count))")
	if ($choice -lt 1 -or $choice -gt $candidates.Count) { throw "no save numbered $choice" }
	$pick = $candidates[$choice - 1]
}

New-Item -ItemType Directory -Force -Path $targetDir | Out-Null
Copy-Item -LiteralPath $pick.FullName -Destination $target
Write-Host "copied the save of $($pick.Directory.Name) to $Gamertag, the original stays in place"
