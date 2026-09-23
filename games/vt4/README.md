# Virtua Tennis 4 on xlive-steamworks

Virtua Tennis 4 is a plain Games for Windows LIVE title. `VT4.exe` imports `xlive.dll` and nothing from
`steam_api.dll`, so online play, friends, achievements and leaderboards run through `xlive.dll`.
`Launcher.exe` is the game's own settings tool and is not affected.

## Install

1. Back up the `xlive.dll` already in the game folder.
2. Copy everything from this folder next to `VT4.exe`.
3. Start the game with Steam running.

## Files

| File | What it is |
| --- | --- |
| `xlive.dll` | the xlive-steamworks SDK build |
| `steam_api.dll` | Valve's Steamworks redistributable, which `xlive.dll` calls |
| `xlive_steamworks.json` | app id 71390, title 0x53450FA2, 48 achievements and 7 ranking leaderboards mapped from the game's SPA |
| `import-save.ps1` | imports existing saves to our steamworks wrapper, see Saves |

## Steam app id

The config uses app id 71390, Virtua Tennis 4 on Steam. `xlive.dll` starts Steam as that app for the
overlay, friends and presence. This release is for owners of Virtua Tennis 4 on Steam: run the game with
Steam running and signed in on the account that owns app 71390. If Steam cannot start the app the wrapper
warns once and the game continues offline.

## Saves

The game keeps its saves in `Documents\Virtua Tennis 4\Saved Games\`, one `<xuid>.sav` per profile next to
`Config.txt`, where the XUID is the id of the signed-in profile. Under this wrapper the profile is your Steam
account, so its XUID is new and the game starts with an empty save.

To carry a save over or from another profile, run `import-save.ps1` from a PowerShell
prompt in the game folder:

```
powershell -ExecutionPolicy Bypass -File import-save.ps1
```

It lists the saves it finds, copies the one you pick to the new name and leaves the original in place.
Saves made under GFWL itself are encrypted to that Live account and cannot be imported.

## Notes

- The log `xlive_steamworks.log` lands next to the dll. Set `log.level` in the json to `debug` for a
  full trace.
- The achievement map in the json is `ACH_%u`, a placeholder. The game doesn't have achievement on
  steam, so we store them locally only for now.
- Leaderboards are mapped, but also not usable on steam.
