# Street Fighter X Tekken on xlive-steamworks

Two builds so we can support Disc version and steam

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

The Steam build's `steam_api.dll` here is a shim: it gives the 2012 exe the SDK 1.19 entry points it
imports (`SteamAPI_Init`, `SteamApps`, `SteamFriends` and their vtables) and forwards every other
call to `steam_api_real.dll`, which is Valve's SDK 1.65 dll renamed. Both files must stay together.
It writes `steam_api_shim.log` next to itself, listing which Steam calls the exe made.

## Files

| File | What it is |
| --- | --- |
| `xlive.dll` | the xlive-steamworks SDK build |
| `steam_api.dll` | GFWL folder: Valve's 1.65 dll. Steam folder: the shim |
| `steam_api_real.dll` | Steam folder only: Valve's 1.65 dll under the name the shim forwards to |
| `xlive_steamworks.json` | app id 209120, 50 achievements and 168 leaderboard views extracted from the game's SPA, unused for now |

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

## Test kit for an owner

`kit\make_owner_kit.ps1` packs `release\zips\sfxt-owner-kit.zip` from the two release folders and
the SDK's probe (build with `-DXLS_BUILD_TESTS=ON`).

## Issues

- **The in-game Store errors out and returns to the title screen.** On entering the main menu the
  game downloads `title/sftk.xoc` from per-title storage (the log shows the request). That file is
  the publisher's DLC roster (`dlc_list`, `gem_list`, `purchasable_dlc`, `entitlement`, ...) that
  GFWL's servers used to serve, and no copy of it is known to exist. Without it the store has no
  catalogue. It is the same under XLLN. A studio that still has the file drops it in
  `xlive-title-storage\title\sftk.xoc`. The DLC itself is unaffected, its ownership comes from
  Steam.
- The Home key opens whatever Steam does for `ActivateGameOverlay`: the in-game overlay when it is
  enabled in Steam's settings, or the steam client.
- Steam lists no achievements for the app yet, so unlocks are recorded in the local folder of the
  wrapper until the game devs adds them.
