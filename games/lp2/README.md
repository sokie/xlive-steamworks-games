# Lost Planet 2 on xlive-steamworks

Lost Planet 2 is a plain Games for Windows LIVE title. The exe (`LP2DX9.exe` for Direct3D 9,
`LP2DX11.exe` for Direct3D 11) imports `xlive.dll` and nothing from `steam_api.dll`, so online play,
co-op, ranked matches, friends, voice, stats and Cloud all run through `xlive.dll`.

## Install

1. Back up the `xlive.dll` already in the game folder.
2. Copy everything from this folder next to `LP2DX9.exe` (and `LP2DX11.exe`, both use the same dll).
3. Start the game with Steam running.

## Files

| File | What it is |
| --- | --- |
| `xlive.dll` | the xlive-steamworks SDK build |
| `steam_api.dll` | Valve's Steamworks redistributable, which `xlive.dll` calls |
| `xlive_steamworks.json` | app id 45750, title 0x43430808, 50 achievements and 62 leaderboard views mapped from the game's SPA |
| `import-save.ps1` | imports existing saves to our steamworks wrapper, see Saves |

## Steam app id

The config uses app id 45750, Lost Planet 2 on Steam. `xlive.dll` starts Steam as that app for the
overlay, friends and presence. This release is for owners of Lost Planet 2 on Steam: run the game with
Steam running and signed in on the account that owns app 45750. If Steam cannot start the app the
wrapper warns once and the game continues offline.

## Saves

The game keeps its saves in `Documents\CAPCOM\LOST PLANET 2\`, one folder per gamertag holding
`Lostplanet2.Lostplanet2Save-capcom`, next to the shared `config.ini`. Under this wrapper the gamertag is
your Steam name (cut to 15 characters, anything outside ASCII becomes `_`), so by default the game starts with an
empty save in a new folder.

To carry a save over from another profile, run `import-save.ps1` from a PowerShell prompt in the game
folder:

```
powershell -ExecutionPolicy Bypass -File import-save.ps1
```

It lists the profiles that have a save, copies the one you pick into your Steam profile's folder and
leaves the original in place. Pass `-Gamertag <name>` if the folder the game
created for you is named differently. Saves made under GFWL itself carry a header encrypted to that Live
account and cannot be imported.

## Notes

- The log `xlive_steamworks.log` lands next to the dll. Set `log.level` in the json to `debug` for a
  full trace.
- Steam Cloud on app 45750 may report no quota, in which case the profile and the files the game
  writes through XStorage stay in `xlive-storage\<account id>\` next to the exe. Back that folder up
  with the game.
- Leaderboards are mapped, but currently not usable on steam.
- Game doesn't have achievements on steam, so unlocks are stored locally for now.
