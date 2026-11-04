# Red Dead Redemption (PC) - Archipelago Randomizer 
Claude AI Assisted in creating this project about 70%. The rest was cleanup by me and testing by me. AI is close to being very impressive but still overlooks very simple things and issues. 

APWorld + RedHook plugin (v6.1) + client for Red Dead Redemption 1 on PC. The Undead Nightmare DLC is not part of it for now, this will most likely change in the future.

## What you need
- Red Dead Redemption (PC, Steam) and [Archipelago](https://github.com/ArchipelagoMW/Archipelago/releases) 0.5.0 or newer.
- [RedHook](https://www.nexusmods.com/reddeadredemption/mods/192) installed in the game folder.
- The [Microsoft Visual C++ Redistributable (x64)](https://aka.ms/vs/17/release/vc_redist.x64.exe). Most PCs already have it.
- From the release download: `RDR-AP.red`, `rdr1_ap_hashmap.txt`, `rdr1_ap_scriptmap.txt` (copy all three into the game
  folder, next to `RDR.exe`) and `rdr1.apworld` (Archipelago Launcher > **Install APWorld**).

## Turn off cloud saves (do this once)
The Steam version of Red Dead Redemption has no Steam Cloud; its cloud saves go through the **Rockstar Games Launcher**.
Turn them off so the cloud does not put your normal saves back over the Archipelago ones (or the other way around):
1. Start the game and wait for the Rockstar Games Launcher overlay to finish loading.
2. Press **Home** (on a controller: **Back/View + RB**) to open the overlay.
3. Click the **gear icon** next to your username (top right), then **Settings > Game Settings**, and turn **Cloud Saves** off.
4. If the game ever asks whether to turn cloud saves on, choose **Not Right Now**.

The setting has been reported to switch itself back on for some players, so check it again if your saves ever look wrong.

## How to play
1. Generate a multiworld with a YAML.
2. Start **Red Dead Redemption Client** from the Archipelago Launcher and connect. After you enter your slot name the client
   can hang for a second or two: that is normal, it is moving your save files into place. `/gamedir <path>` if the game is not in
   the default Steam folder.
3. Start the game **after** connecting. The client swaps in this multiworld's own saves (start a NEW GAME the first time).
4. Play.

  ** P.S. Upon finishing up a session, close the game first, then the client: your normal saves come back automatically.**

## Features
- **Story missions play in the vanilla order.** Nothing is locked: every story mission you finish sends a check.
- **Progressive Dead Eye** (4 levels: Dead Eye, auto lock-on, target painting, fast recovery). Dead Eye is off until the first one is sent.
- **Weapons are items** (Cattleman start, random start or vanilla pickups).
    - Cattleman Start, you start with the cattleman and get sent random weapons from items
    - Random Start, you start with a random weapon and get sent random weapons from items
    - Vanilla Pickups, vanilla experience
- **Outfits are items** (14, cosmetic). The US Marshal, US Army and Bureau uniforms are not, because the game itself keeps
  them locked until late in the story.
- **Filler:** Max Ammo, Dead Eye Refill, $15 / $35 / $55 cash (given again if a checkpoint reload takes it back).
- **Traps** (every trap shows a big warning first and fires 3-5 seconds later): animal ambush, gang attack, lawmen posse,
  stampede, slow motion, drunk, out of bullets (half your ammo comes back after 20 seconds), Herbert Moon, Dead Eye drain.
- **DeathLink** with themed cowboy death messages. Like some high noon references and other fun things.
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

## If a check did not send
- **In the client:** type `/resync`. Everything the game has found is sent to the server again (safe any time; repeats are
  ignored).
- **In the game:** if the game never noticed the check (for example a mission whose name could not be matched), open
  `rdr1_ap_in.txt` in the game folder (create it if it is missing), write `sendcheck ` followed by the exact location name,
  and save. Example: `sendcheck A Gentle Drive with Friends`. The game reads the file within a second.

## Known beta issues
- Some story mission names are matched from script contents and may be wrong; "A Gentle Drive with Friends" has no match
  yet. Report the real title of any wrong check. 
- Stranger completion is read from the game's "stranger missions completed" stat (not confirmed yet in play).
- Blackjack counts a win when you leave the table up money (per-hand detection is being worked on).
- Keep Rockstar cloud saves off while using the save swap (see "Turn off cloud saves").

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
