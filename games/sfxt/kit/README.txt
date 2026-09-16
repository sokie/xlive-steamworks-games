================================================================================
 xlive-steamworks - Street Fighter X Tekken test kit
================================================================================

Thank you for helping test. This runs Street Fighter X Tekken's online code on
Steam instead of Games for Windows LIVE, using your Steam account and your copy
of the game.

You need:
  - Steam running and logged in.
  - This Steam account must OWN Street Fighter X Tekken (app 209120).
  - The game installed: either the GFWL build or the Steam build. The install
    script tells them apart by itself.

Nothing here is installed system-wide. It writes only inside its own folder and,
for the game test, inside your game folder, after backing up every file it
replaces.

--------------------------------------------------------------------------------
 STEP 1  -  Run the probe          (about one minute, always do this)
--------------------------------------------------------------------------------
Double-click:   1_run_probe.bat

It checks what Steam gives the game (your account, ownership, achievements,
Cloud, lobbies, the relay network) and writes  results\probe_report.txt .
It does NOT change the game and creates nothing permanent on Steam.

--------------------------------------------------------------------------------
 STEP 2  -  Install into the game  (optional, needs the game installed)
--------------------------------------------------------------------------------
Double-click:   2_install.bat

It asks for your Street Fighter X Tekken folder (the one with SFTK.exe), you
can also drag that folder onto the .bat file. It backs up the files it replaces
as  <name>.orig-backup  and copies the new ones in.

Then start the game the usual way (from Steam, or SFTK.exe), play a little, try
the online / ranked / lobby menus, and quit normally.

To put the game back exactly as it was:   3_restore.bat

--------------------------------------------------------------------------------
 STEP 3  -  Send the results back
--------------------------------------------------------------------------------
Double-click:   4_zip_results.bat

It copies the game's logs from the game folder, then makes  sfxt-results.zip
in this folder. Send that one file back. It contains the probe report, the game
logs and a short system-info text (Windows version, GPU, whether Steam was
running). It does not contain any password or personal file.

Questions or a crash? Send sfxt-results.zip and describe what you saw on screen.
