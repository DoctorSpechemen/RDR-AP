# RDR-AP Beta 2 (pre-release)

Archipelago randomizer for Red Dead Redemption (PC). **Beta: expect bugs.** Your normal saves are swapped out and backed
up automatically (`rdr1_ap_saves\_backups` in the game folder), but please keep your own backup too.

## Download
`RDR-AP_Beta2.zip` contains `rdr1.apworld`, `RDR-AP.red`, `rdr1_ap_hashmap.txt`, `rdr1_ap_scriptmap.txt`, the README
and a template YAML. Setup steps are in the README (needs RedHook and Archipelago 0.5.0+).

## What's in it
- Every story and stranger mission is a check; missions play in the normal order.
- Checks for gang kills, gang hideouts, hat tricks (up to 15), minigame wins and shop purchases (general stores,
  gunsmiths, tailors).
- Items: Progressive Dead Eye, weapons, 14 outfits, cash, Max Ammo, Dead Eye Refill, and 9 kinds of traps.
- DeathLink, a separate save for every multiworld.

## Changes since Beta 1
- Story missions are no longer items: no "mission locked" any more, every finished mission sends its check.
- Missions that end inside a load screen (Exodus in America) now send their check.
- Hat tricks up to 15. Random encounters and fame/honor checks are off for now; Fame/Honor fillers removed.
- Cash fillers ($15/$35/$55) come back if a checkpoint reload takes them away.
- Outfit names corrected; the US Marshal, US Army and Bureau uniforms are not items (the game locks them until late).
- Options are grouped in the YAML (Goal and Weapons, Checks, Items, Traps, Multiplayer).

## Reporting bugs
Open an issue with what you did, what you expected, and attach `rdr1_ap_test.log` from the game folder.
If a check did not send: `/resync` in the client, or `sendcheck <location name>` in `rdr1_ap_in.txt` (see README).
