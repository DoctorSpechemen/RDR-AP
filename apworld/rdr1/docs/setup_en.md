# Red Dead Redemption (PC) Setup Guide

> BETA. Back up your saves before you start (the client also makes a backup every time it swaps saves).

## Requirements

- [Archipelago](https://github.com/ArchipelagoMW/Archipelago/releases) 0.5.0 or newer
- Red Dead Redemption on PC (base game; Undead Nightmare is not used)
- [RedHook](https://www.nexusmods.com/reddeadredemption/mods/192) installed in the game folder
- From this release: `rdr1.apworld`, `RDR-AP.red`, `rdr1_ap_hashmap.txt`, `rdr1_ap_scriptmap.txt`

## Installing

1. Open the Archipelago Launcher, choose **Install APWorld** and select `rdr1.apworld`.
2. Copy `RDR-AP.red`, `rdr1_ap_hashmap.txt` and `rdr1_ap_scriptmap.txt` into the Red Dead Redemption game folder
   (the folder with the game's .exe; for Steam usually
   `C:\Program Files (x86)\Steam\steamapps\common\Red Dead Redemption`).
3. Turn off cloud saves, so they do not fight with the save swap. The Steam version uses the Rockstar Games Launcher for
   cloud saves, not Steam Cloud: in game press **Home** (controller: **Back/View + RB**) to open the Rockstar overlay, click the
   **gear icon** by your username, go to **Settings > Game Settings** and turn **Cloud Saves** off. If the game asks to turn
   them on, choose **Not Right Now**.

## Creating your options file

1. In the Archipelago Launcher choose **Generate Template Options** and open the
   `Red Dead Redemption.yaml` template, or use the **Options Creator**.
2. Set your `name`, then adjust goal, weapon mode, check categories and traps to taste.

## Joining a multiworld

1. With the game **closed**, open **Red Dead Redemption Client** from the Archipelago Launcher and connect to the server
   with your slot name. If the game is not in the default Steam folder, type `/gamedir <path to the game folder>` once.
2. The client puts this multiworld's own save files in place (your normal saves are kept safe and come back later).
   It can hang for a second or two after you enter your slot name while it moves the files; that is normal.
3. Start the game and choose **New Game** the first time. "Connected to a new multiworld" appears on screen.
4. When you are done: close the game first, then the client. Your normal saves come back automatically.

## If a check did not send

- Type `/resync` in the client to send everything the game has found again.
- If the game never noticed it, write `sendcheck <exact location name>` into `rdr1_ap_in.txt` in the game folder and save.
