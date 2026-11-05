# RDR-AP Beta 2.2 (pre-release)

Archipelago randomizer for Red Dead Redemption (PC). **Beta: expect bugs.** Your normal saves are swapped out and backed
up automatically (`rdr1_ap_saves\_backups` in the game folder), but please keep your own backup too.

## Download
`RDR-AP_Beta2.2.zip`: `rdr1.apworld`, `RDR-AP.red`, `rdr1_ap_hashmap.txt`, `rdr1_ap_scriptmap.txt`, the README, a template
YAML and `RDR-AP_PopTracker.zip`. Setup steps are in the README (needs RedHook and Archipelago 0.5.0+).
Updating from 2.1: replace all files. Your current seed keeps working; the new fillers need a new seed.

## New in Beta 2.2
- Blackjack: every winning hand is a check (no more "leave the table up money").
- New fillers: Fame +50, Honor +50 (given again if a checkpoint reload takes them back) and satchel consumables:
  Snake Oil, Tonic, Medicine, Moonshine, Chewing Tobacco.
- PopTracker: original icon set, a pin per hideout, an "Anywhere" panel, Dead Eye levels shown one by one, AP button fixed.
- F9 tally also shows "Traps survived" and "Herbert Moon visits".

## Fixes
- Progress counters (gang kills, hat tricks, minigame wins) were lost when the game restarted. Fixed.
- Five Finger Fillet and Arm Wrestling wins were not tracked at all. Fixed.
- A minigame win could repeat an old "Win N" after a reload. Each win now sends the next unsent one.
- Stranger stories: the check went to the wrong stranger, or not at all, when the last stranger met was a different one.
  The check is now sent when the game marks the story complete.
- Gang, posse and animal ambushes no longer spawn in the air on hilly ground.
- Traps show one message instead of two.
- Lightning trap: just the strikes, no slow-arriving storm.
- Gentleman's Attire is no longer an item (every PC copy has it).

## Reporting bugs
Open an issue with what you did, what you expected, and attach `rdr1_ap_test.log` from the game folder.
If a check did not send: `/resync` in the client, or `sendcheck <location name>` in `rdr1_ap_in.txt` (see README).
