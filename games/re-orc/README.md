# Resident Evil: Operation Raccoon City on xlive-steamworks

The Steam release of Operation Raccoon City ships a GFWL `xlive.dll` and a 2012 `steam_api.dll`.
Online play, lobbies, friends, voice and stats run through `xlive.dll`. The `steam_api.dll` only
carries the language, DLC ownership, the store page and the overlay.

| File | Imports | Release folder |
| --- | --- | --- |
| `RaccoonCity.exe` | `xlive.dll` and a 2012 `steam_api.dll` | `re-orc-steam` |

## Install

1. Back up the files `xlive.dll` and `steam_api.dll`.
2. Copy everything from the `re-orc-steam` release folder next to `RaccoonCity.exe`.
3. Start the game from Steam, or with Steam running.

The release's `steam_api.dll` is a shim: the 2012 exe imports the SDK 1.13 entry points
(`SteamAPI_Init`, `SteamApps`, `SteamFriends`, `SteamUtils` and their vtables) and our shim wraps
those, forwarding every other call to `steam_api_real.dll`, which is the latest SDK renamed. Both
files must stay together.

## Files

| File | What it is |
| --- | --- |
| `xlive.dll` | the xlive-steamworks SDK build |
| `steam_api.dll` | the shim |
| `steam_api_real.dll` | Valve's SDK redistributable under the name the shim forwards to |
| `xlive_steamworks.json` | app id 209100, title id 0x43430fa1, 50 achievements from the game's SPA |
| `import-save.ps1` | imports existing saves to our steamworks wrapper, see Saves |

## Saves

The game keeps its saves in `Documents\CAPCOM\RERC\`, one file per profile named after the profile's XUID,
next to the shared `settings.dat`. With the wrapper the XUID comes from your Steam account, so the game
starts with an empty save.

To carry the save of another profile over, run `import-save.ps1` from a PowerShell prompt in the game
folder:

```
powershell -ExecutionPolicy Bypass -File import-save.ps1
```

It will list the saves it finds, copies the one you pick to the new name and leaves the original in place. 
A save made under GFWL itself carries a token encrypted to that Live account and may not load.

## Notes

- The log `xlive_steamworks.log` lands next to the dll. Set `log.level` in the json to `debug` for a
  full trace.
- The achievement map in the json is `ACH_%u`, unused for now.
- Save data the game writes through XStorage and the profile stay in `xlive-storage\<account id>\`
  next to the exe. Back that folder up with the game.
