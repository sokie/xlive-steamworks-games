# xlive-steamworks-games

Per-title builds on top of the [xlive-steamworks](https://github.com/sokie/xlive-steamworks) SDK,
a GFWL `xlive.dll` that runs on the Steamworks SDK. Each game folder builds the
SDK from source, adds whatever that game needs on top (an old `steam_api.dll` ABI, its own config,
extra binaries) and outputs a release folder for you copy into the game directory.

## Layout

```
sdk/            the xlive-steamworks SDK, as a git submodule (pinned commit)
games/<title>/  one folder per title: config and README, plus a CMakeLists.txt when the game needs more
tools/          make_release.ps1 zips the release folders and the bundle
.github/        the workflow that builds every push and publishes a tag as a GitHub release
release/        build output: release/<title>-<variant>/ and release/zips/ (gitignored)
```

## Building

```
git submodule update --init
cmake -B build -G "Visual Studio 16 2019" -A Win32 -DSTEAMWORKS_SDK_DIR=D:/Projects/SteamworksSDK-1.62
cmake --build build --config Release
powershell -File tools/make_release.ps1
```

`STEAMWORKS_SDK_DIR` points at an unpacked Steamworks SDK (the directory that holds `public/steam`
and `redistributable_bin`). Without it the SDK's CMake fetches the rlabrecque mirror. The releases
build against SDK 1.62. The wrapper ships its own `steam_api.dll` so the version does not have to
match the client, and 1.62 latest SDK Proton 9 bridges, so we can use 1 build for win and linux. 
`-DXLS_SDK_DIR=<path>` builds against another SDK checkout instead of the
submodule, for work on both repositories at once.

Every release folder holds `xlive.dll`, the `steam_api.dll`, `xlive_steamworks.json`
and a README that says what to copy where.

## Releases

Every push to `main` builds all of it on GitHub Actions and keeps the zips as a workflow artifact.
A tag publishes:

```
git tag v0.2.0
git push origin v0.2.0
```

The workflow builds the standard setup once (the SDK `xlive.dll` plus the 1.62 `steam_api.dll`),
assembles one release folder per game from its `xlive_steamworks.json` and README, builds the games
that need more (the SFxT Steam build and its `steam_api.dll` shim), then zips each release folder,
packs them all into `xlive-steamworks-games-<tag>.zip`, writes `SHA256SUMS.txt` and publishes the
lot as the GitHub release for that tag. `tools/make_release.ps1` does the same packing locally.

## Titles

| Title | Folder | Variants |
| --- | --- | --- |
| Street Fighter X Tekken | `games/sfxt` | `sfxt-gfwl` (GFWL build: SDK dll + the SDK steam_api.dll), `sfxt-steam` (Steam build: adds the `steam_api.dll` shim that gives the 2012 exe its SDK 1.19 exports over the SDK dll). |
| Lost Planet 2 | `games/lp2` | `lp2-gfwl` (GFWL build: SDK dll + the SDK steam_api.dll, the game ships no steam_api.dll of its own) |

## Adding a title

Most titles only need the basics, so a new folder is two files and no CMake:

1. `games/<title>/xlive_steamworks.json`: the app id, the title id and the achievement and leaderboard
   maps as the title's own SPA numbers them, `games/lp2` shows the shape.
2. `games/<title>/README.md`: the install steps for a player or a studio, it ships in the release.

The root `CMakeLists.txt` finds the folder and assembles `release/<title>-gfwl/` from the SDK
`xlive.dll`, the SDK `steam_api.dll` and those two files.

A title that needs more (its own `steam_api.dll`, extra binaries, a second variant) gets a
`games/<title>/CMakeLists.txt` on top. Build the extras there, call
`xls_add_standard_release(<title>)` for the plain variant and `xls_add_release(<variant> FILES ...)`
for each other one, like `games/sfxt`.

## License

MIT, like the SDK. The Steamworks SDK redistributable is Valve's and ships under its own terms.
