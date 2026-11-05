# Red Dead Redemption (PC) - Archipelago Randomizer 
<img width="586" height="322" alt="image" src="https://github.com/user-attachments/assets/ed496406-511e-4a3a-85d8-578699d2ce7b" />


Claude AI Assisted in creating this project about 70%. The rest was cleanup by me and testing by me. AI is close to being very impressive but still overlooks very simple things and issues. 

**Pre-release Beta 2.2.** APWorld + RedHook plugin (v6.3) + client for Red Dead Redemption 1 on PC. The Undead Nightmare DLC is not part of it for now, this will most likely change in the future.

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
- **Outfits are items** (13, cosmetic). Not included: the US Marshal, US Army and Bureau uniforms (the game itself keeps
  them locked until late in the story) and Gentleman's Attire (every PC copy already has it).
- **Filler:** Max Ammo, Dead Eye Refill, $15 / $35 / $55 cash, Fame +50, Honor +50 (cash, fame and honor are given again if
  a checkpoint reload takes them back), and consumables for your satchel: Snake Oil, Tonic, Medicine, Moonshine and
  Chewing Tobacco.
- **Traps** (every trap shows one big warning first and fires 3-5 seconds later): animal ambush, gang attack, lawmen posse,
  stampede, slow motion, drunk, out of bullets (half your ammo comes back after 20 seconds), Herbert Moon, lightning
  (three strikes land close to you, keep moving), Dead Eye drain.
- **DeathLink** with themed cowboy death messages. Like some high noon references and other fun things.
- **TrapLink**: traps are shared with other TrapLink games; their traps become the closest Red Dead trap.
- **"Found X for Player"** line at the bottom of the screen when one of your checks sends an item to someone else, and
  **sounds** with item pop-ups and trap warnings (both can be turned off in the YAML).
- **F9 tally** in game: checks done / total per category, items received, cash received and every trap you survived.
- A **PopTracker pack** (`RDR-AP_PopTracker.zip`) that tracks your checks and items automatically.
- **A separate save per multiworld**, and the mod's own progress per multiworld (`rdr1_ap_seeds\<seed>\`).
- Big centred pop-ups for every received item.

## What sends a check
| Category | Check | Count |
|---|---|---|
| Story missions | finishing the mission | 57 |
| Stranger missions | finishing each stranger's whole story (sent when the story ends) | 18 |
| Gang kills | every 5 kills of a gang, up to 50 (Walton's Gang, Banditos, Reyes' Rebels, Mexican Soldiers, Treasure Hunters, Cattle Rustlers, Indian Raiders, Smugglers, Bounty Hunters). Hideout outlaws count; story mission enemies do not | up to 90 |
| Gang hideouts | clearing Pike's Basin, Twin Rocks, Fort Mercer, Tumbleweed, Gaptooth Breach, Nosalida, Tesoro Azul | 7 |
| Hat tricks | shooting a hat off someone without killing them | up to 15 |
| Minigames | wins in poker and blackjack (each hand won), arm wrestling, five finger fillet, horseshoes, liar's dice, duels | up to 5 each |
| Bounties | turning in wanted-poster bounties, dead or alive, counted in the order you finish them ("Bounty 1", "Bounty 2", ...) | up to 20 |
| Shops | gear, maps, guns and tailor items in every general store, gunsmith and tailor (one check per item, buy it in any store that has it) | 30 |

## What is not allowed / not counted
- Guns and gear you pick up or buy without having received them from Archipelago are taken away after 5 seconds
  ("That weapon isn't yours to keep."). Thrown weapons, the knife and the lasso are never taken. (Not in vanilla weapon mode.)
- Horse deeds are blocked in every store. Horses are not part of the randomizer.
- Things you already found are blocked in every shop, so nothing is bought twice.
- Consumables (food, medicine, ammo, tonics) are left alone.
- Not tracked (yet): random encounters, fame/honor milestones, ambient challenges, treasure map
  treasure, the Buffalo Rifle (needs Master Hunter 5).

## In-game keys
- **F9**: Archipelago tally (stays up 30 seconds, press F9 again to close it): checks done / total for each category, all
  checks, items received, cash received and the traps you survived (needs the client connected for the totals). Gang kill
  and hat trick counters stop showing once you reach the limit set in your YAML.

## If a check did not send
- **In the client:** type `/resync`. Everything the game has found is sent to the server again (safe any time; repeats are
  ignored).
- **In the game:** if the game never noticed the check (for example a mission whose name could not be matched), open
  `rdr1_ap_in.txt` in the game folder (create it if it is missing), write `sendcheck ` followed by the exact location name,
  and save. Example: `sendcheck A Gentle Drive with Friends`. The game reads the file within a second.

## PopTracker
Install [PopTracker](https://github.com/black-sliver/PopTracker/releases) 0.18 or newer, put `RDR-AP_PopTracker.zip` in its
`packs` folder, open it and click the **AP** button to connect with your slot name. Checks that are not in your seed are
hidden; grey checks need more Dead Eye in logic. Checks someone hinted for you are highlighted (needs a recent PopTracker).

## Known beta issues
- Some story mission names are matched from script contents and may be wrong. Report the real title of any wrong check.
  ("A Gentle Drive with Friends" has no script of its own; the plugin sends it when a story mission it cannot identify ends.)
- Herbert Moon's voice lines have not been heard in game yet (the trap shows its warning; the voice is being worked on).
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
