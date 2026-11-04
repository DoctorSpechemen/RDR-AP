# Red Dead Redemption (PC) - Archipelago Randomizer - BETA 2

APWorld + RedHook plugin (v6.1) + client for Red Dead Redemption 1 on PC. The Undead Nightmare DLC is not part of it.

## What you need
- Red Dead Redemption (PC) with RedHook, and `RedHookExample.red` (this plugin) in the game folder.
- `rdr1_ap_hashmap.txt` and `rdr1_ap_scriptmap.txt` in the game folder (mission data).
- `rdr1.apworld` in Archipelago's `custom_worlds` folder.

## How to play
1. Generate a multiworld with a YAML (`game: Red Dead Redemption`).
2. Start **Red Dead Redemption Client** from the Archipelago Launcher and connect. `/gamedir <path>` if the game is not in
   the default Steam folder.
3. Start the game **after** connecting. The client swaps in this multiworld's own saves (start a NEW GAME the first time).
4. Play. Close the game first, then the client: your normal saves come back automatically.

## Features
- **Story missions play in the vanilla order.** Nothing is locked: every story mission you finish sends a check.
- **Progressive Dead Eye** (4 levels: Dead Eye, auto lock-on, target painting, fast recovery). Dead Eye is off until the first.
  Logic expects more Dead Eye later in the story (1 for Mexico up to 4 for the last mission); the game never stops you.
- **Weapons are items** (Cattleman start, random start or vanilla pickups).
- **Outfits are items** (17, cosmetic).
- **Filler:** Max Ammo, Dead Eye Refill (rarer), $15 / $35 / $55 cash, Fame +20, Honor +20.
- **Traps** (every trap shows a big warning first and fires 3-5 seconds later): animal ambush, gang attack, lawmen posse,
  stampede, slow motion, drunk, out of bullets (half your ammo comes back after 20 seconds), Herbert Moon (he whispers in your
  ear), Dead Eye drain.
- **DeathLink** with cowboy death messages.
- **A separate save per multiworld**, and the mod's own progress per multiworld (`rdr1_ap_seeds\<seed>\`).
- Big centred pop-ups for every received item.

## What sends a check
| Category | Check | Count |
|---|---|---|
| Story missions | finishing the mission | 57 |
| Stranger missions | finishing the last part of each stranger's story | 18 |
| Gang kills | every 5 kills of a gang, up to 50 (Walton's Gang, Banditos, Reyes' Rebels, Mexican Soldiers, Treasure Hunters, Cattle Rustlers, Indian Raiders, Smugglers, Bounty Hunters). Hideout outlaws count; story mission enemies do not | up to 90 |
| Gang hideouts | clearing Pike's Basin, Twin Rocks, Fort Mercer, Tumbleweed, Gaptooth Breach, Nosalida, Tesoro Azul | 7 |
| Hat tricks | shooting a hat off someone without killing them | up to 15 |
| Minigames | wins in poker (each hand won), blackjack, arm wrestling, five finger fillet, horseshoes, liar's dice, duels | up to 5 each |
| Shops | gear, maps, guns and tailor items in every general store, gunsmith and tailor (one check per item, buy it in any store that has it) | 30 |

## What is not allowed / not counted
- Guns and gear you pick up or buy without having received them from Archipelago are taken away after 5 seconds
  ("That weapon isn't yours to keep."). Thrown weapons, the knife and the lasso are never taken. (Not in vanilla weapon mode.)
- Horse deeds are blocked in every store. Horses are not part of the randomizer.
- Things you already found are blocked in every shop, so nothing is bought twice.
- Consumables (food, medicine, ammo, tonics) are left alone.
- Not tracked (yet): random encounters, fame/honor milestones, wanted-poster bounties, ambient challenges, treasure map
  treasure, the Buffalo Rifle (needs Master Hunter 5).
- Mission replays and reloads never send anything twice; a reload cannot count the same win twice.

## If a check did not send
- **In the client:** type `/resync`. Everything the game has found is sent to the server again (safe any time; repeats are
  ignored).
- **In the game:** if the game never noticed the check (for example a mission whose name could not be matched), open
  `rdr1_ap_in.txt` in the game folder (create it if it is missing), write `sendcheck ` followed by the exact location name,
  and save. Example: `sendcheck A Gentle Drive with Friends`. The game reads the file within a second.

## Known beta issues
- Some story mission names are matched from script contents and may be wrong; "A Gentle Drive with Friends" has no match
  yet (use `sendcheck`). Report the real title of any wrong check.
- Stranger completion is read from the game's "stranger missions completed" stat (not confirmed yet in play).
- Blackjack counts a win when you leave the table up money (per-hand detection is being worked on).
- Outfit names other than Gentleman's Attire are guesses from the game's internal names.
- Turn Rockstar cloud saves off for RDR1 while using the save swap.

## Credits
This randomizer is built on the work of these projects and their authors. Thank you!

- **[Archipelago](https://github.com/ArchipelagoMW/Archipelago)** (ArchipelagoMW and contributors): the multiworld
  framework, generator, server and the CommonClient this client is built on.
- **[RedHook](https://www.nexusmods.com/reddeadredemption/mods/192)** and the
  **[RedHook SDK](https://github.com/K3rhos/RedHookSDK)** by **K3rhos**: the script hook that loads the plugin, and the
  native, enum and struct headers (`Natives.h`, `Enums.h`, `Structs.h`) it is written against.
- **[RDR-PC-Natives-DB](https://github.com/K3rhos/RDR-PC-Natives-DB)** by **K3rhos**: the native function database for
  RDR1 on PC.
- **RDR2 Decompiled Scripts** (decompiled scripts of Red Dead Redemption 1, codename "RDR2", PC and Xbox 360): the source
  of the mission objective hashes (`rdr1_ap_hashmap.txt`), mission script names, stat ids, the outfit table and the
  hideout and minigame stats. Made with a modified [Magic RDR](https://github.com/K3rhos/Magic-RDR).
- **[Magic RDR](https://github.com/Foxxyyy/Magic-RDR)** by **Foxxyyy**: the original RDR1 file and script tool behind
  those decompiled scripts.
- **[RDR-Strings-and-Enums](https://github.com/EvilBlunt/RDR-Strings-and-Enums)** by **EvilBlunt**: the faction, weapon
  and other game enums used for gang kill tracking and weapon handling.
- **Claude** (Anthropic's AI): helped write the plugin, APWorld and client code, alongside the in-game testing, design and
  direction of the project's author.

Red Dead Redemption is a trademark of Rockstar Games / Take-Two Interactive. This is an unofficial fan project, not
affiliated with or endorsed by Rockstar Games, and it contains no game files.
