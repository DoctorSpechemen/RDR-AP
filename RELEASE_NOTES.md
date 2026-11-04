# RDR-AP Beta 2.1 (pre-release)

Archipelago randomizer for Red Dead Redemption (PC). **Beta: expect bugs.** Your normal saves are swapped out and backed
up automatically (`rdr1_ap_saves\_backups` in the game folder), but please keep your own backup too.

## Download
`RDR-AP_Beta2.1.zip`: `rdr1.apworld`, `RDR-AP.red`, `rdr1_ap_hashmap.txt`, `rdr1_ap_scriptmap.txt`, the README, a template
YAML and `RDR-AP_PopTracker.zip`. Setup steps are in the README (needs RedHook and Archipelago 0.5.0+).

## New in Beta 2.1
- Bounties: every wanted-poster bounty you turn in is a check (up to 20, counted in order).
- Lightning Storm trap: a storm rolls in and three lightning strikes land close to you.
- TrapLink support (option): traps are shared with other TrapLink games.
- "Found X for Player" line when one of your checks sends an item to someone else (option).
- Sounds with item pop-ups and trap warnings (option).
- PopTracker pack with automatic tracking.
- F9 tally: checks done / total per category (stays 30 s, F9 again closes it). The old F9-F11 test keys are gone.
- Clean-up: old mission-cluster code removed, quieter log ("verbose 1" in rdr1_ap_in.txt for the full log).

## Reporting bugs
Open an issue with what you did, what you expected, and attach `rdr1_ap_test.log` from the game folder.
If a check did not send: `/resync` in the client, or `sendcheck <location name>` in `rdr1_ap_in.txt` (see README).
