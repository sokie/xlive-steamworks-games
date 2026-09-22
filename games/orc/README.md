# Resident Evil: Operation Raccoon City on xlive-steamworks

The Steam release of Operation Raccoon City ships a GFWL `xlive.dll` and a 2012 `steam_api.dll`.
Online play, lobbies, friends, voice and stats run through `xlive.dll`. The `steam_api.dll` only
carries the language, DLC ownership, the store page and the overlay.

| File | Imports | Release folder |
| --- | --- | --- |
| `RaccoonCity.exe` | `xlive.dll` and a 2012 `steam_api.dll` | `orc-steam` |

## Install

1. Back up the files `xlive.dll` and `steam_api.dll`.
2. Copy everything from the `orc-steam` release folder next to `RaccoonCity.exe`.
3. Start the game from Steam, or with Steam running.

The release's `steam_api.dll` is a shim: the 2012 exe imports the SDK 1.13 entry points
(`SteamAPI_Init`, `SteamApps`, `SteamFriends`, `SteamUtils` and their vtables) and the shim answers
those, forwarding every other call to `steam_api_real.dll`, which is the latest SDK renamed. Both
files must stay together. It writes `steam_api_shim.log` next to itself, listing which Steam calls
the exe made.

## Files

| File | What it is |
| --- | --- |
| `xlive.dll` | the xlive-steamworks SDK build |
| `steam_api.dll` | the shim |
| `steam_api_real.dll` | Valve's SDK redistributable under the name the shim forwards to |
| `xlive_steamworks.json` | app id 209100, title id 0x43430fa1, 50 achievements from the game's SPA |

## Notes

- The log `xlive_steamworks.log` lands next to the dll. Set `log.level` in the json to `debug` for a
  full trace.
- The exe uses the older Steam interfaces `ISteamApps004`, `ISteamFriends009` and `ISteamUtils005`.
  Steam only appends methods, so the shim serves 004 and 009 from the newer 005 and 011 vtables and
  writes 005 out for `ISteamUtils` because the modern SDK dropped its `RunFrame` slot. The shim log
  names every vtable slot the exe reaches, so a first run confirms the layout.
- The achievement map in the json is `ACH_%u`, a placeholder. Replace the names with the game's real
  Steam achievement API names for app 209100 before achievements sync.
- Save data the game writes through XStorage and the profile stay in `xlive-storage\<account id>\`
  next to the exe. Back that folder up with the game.
