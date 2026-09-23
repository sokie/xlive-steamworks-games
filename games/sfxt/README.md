# Street Fighter X Tekken on xlive-steamworks

Two builds, one for the disc release and one for Steam:

| Build | SFTK.exe size | Imports | Release folder |
| --- | --- | --- | --- |
| GFWL | 14,295,096 bytes | `xlive.dll` only | `sfxt-gfwl` |
| Steam | 14,326,352 bytes | `xlive.dll` and a 2012 `steam_api.dll` | `sfxt-steam` |

Online play, lobbies, friends, voice and stats run through `xlive.dll` in both builds. The Steam
build only uses its own `steam_api.dll` for the language, DLC ownership and the store page.

## Install

1. Back up the files `xlive.dll`, and for the Steam build `steam_api.dll`.
2. Copy everything from the release folder next to `SFTK.exe`.
3. Start the game from Steam, or with Steam running.

The Steam build's `steam_api.dll` here is a shim: the 2012 exe needs the SDK 1.19 entry points
(`SteamAPI_Init`, `SteamApps`, `SteamFriends` and their vtables) and forwards every other
call to `steam_api_real.dll`, which is the latest SDK renamed. Both files must stay together.

## Files

| File | What it is |
| --- | --- |
| `xlive.dll` | the xlive-steamworks SDK build |
| `steam_api.dll` | GFWL folder: Valve's SDK redistributable. Steam folder: the shim |
| `steam_api_real.dll` | Steam folder only: Valve's SDK redistributable under the name the shim forwards to |
| `xlive_steamworks.json` | app id 209120, 50 achievements and 168 leaderboard views extracted from the game's SPA, unused for now |
| `import-save.ps1` | imports existing saves to our steamworks wrapper, see Saves |

## Saves

The game keeps its saves in `Documents\CAPCOM\SFTK\savedata\`, one folder per profile named after the
profile's XUID, next to the shared `config.ini`. Under this wrapper the XUID comes from your Steam account,
so the game starts with an empty folder.

To carry the save data of another profile over, run `import-save.ps1` from a PowerShell prompt in the game
folder:

```
powershell -ExecutionPolicy Bypass -File import-save.ps1
```

It lists the profiles that have save data, copies the whole folder of the one you pick into your Steam
profile's folder and leaves the original in place. It does nothing when your folder already holds files. A
save made under GFWL itself carries a token encrypted to that Live account and may not load.

## Notes

- The log `xlive_steamworks.log` lands next to the dll. Set `log.level` in the json to `debug` for a
  full trace.
- Steam Cloud has no quota on app 209120, so the save data the game writes through XStorage and
  the profile stay in `xlive-storage\<account id>\` next to the exe. Back that folder up with the
  game. Leaderboards need `leaderboards.create_if_missing` or boards created in the partner site.
- The in-game Invite button opens Steam's invite dialog (friends with an Invite button) while you
  are in a lobby, and the plain Steam friends list otherwise.
- The Steam build asks Steam only for DLC ownership (84 ids, 210030 to 210113) and the store page.
  It takes the language from `language.cfg` when that file exists.

## Issues

- **The in-game Store errors out and returns to the title screen.** On entering the main menu the
  game downloads `title/sftk.xoc` from per-title storage (the log shows the request). That file is
  the publisher's DLC roster (`dlc_list`, `gem_list`, `purchasable_dlc`, `entitlement`, ...) that
  GFWL's servers used to serve, and no copy of it is known to exist. Without it the store has no
  catalogue. The DLC itself is unaffected, its ownership comes from Steam.
- The Home key opens whatever Steam does for `ActivateGameOverlay`.
- Steam lists no achievements for the app yet, so unlocks are recorded in the local folder of the
  wrapper until the developer adds them.
