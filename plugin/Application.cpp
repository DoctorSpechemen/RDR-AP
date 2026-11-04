#include <cstring>
#include <pch.h>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <cstdlib>
#include <deque>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <filesystem>

// RDR1 Archipelago plugin v6.1 BETA 2 (grew out of the feasibility test plugin; test commands are still available).
//
// Keys (in game, once you have control of John):
//   F7                 log player position
//   End (or Numpad +)  stat snapshot (F8 is taken by RedHook's own console).
//                      FIRST press: stand still ~2 seconds while the plugin calibrates, which finds stats that
//                      change by themselves and ignores them. EVERY LATER press logs which of the remaining
//                      stats changed since the previous press.
//   F9                 give the next weapon in the list (+ some ammo). The log says which attempt (step 1-3) worked.
//   F10                fill ammo to max for the common ammo types
//   F11                spawn a RANDOM animal 4 metres away (cougar, bears, wolf, coyote, boar, bobcat, snake, or a harmless
//                      rabbit). Hostile ones use the NEXT "recipe" (0,1,2,3) for making them attack, and the log then
//                      tracks the animal for 25 seconds (distance, fighting, health).
//
// NEW in v5.14: hat trick rule: player fired within 1.5 s + hat off + actor alive (and still alive 1.5 s later) = "HAT TRICK SUCCESS (prototype)"; dead = failed.
// NEW in v5.14: five finger fillet win/loss from the objective hash (MINIGAME WIN / MINIGAME LOSS log lines).
// NEW in v5.14: "shove [explosion name] [distance] [size]" (shove trap test, logs ragdoll and health lost).
// NEW in v5.14: "drunk <seconds>" (drunk trap test, "drunk 0" ends it).
// NEW in v5.14: "after <seconds> <command>" delays a command (the game closes menus when you alt-tab).
// NEW in v5.14: stray-weapon watch only runs in normal play and ignores bulk inventory restores (a save load had marked all 21 guns).
// NEW in v5.18: gang kill counter by faction id ('GANG KILL' lines, 'gangkills'), 'gangattack [gang] [count]' trap (models from eActorEnum).
// NEW in v5.17: objective/cutscene start+end rows are always written to rdr1_ap_objectives.csv (play normally to build the final-objective table); 'journaldump'; journal probe lines; cutscene-end mission completion for table entries.
// NEW in v5.15: 'shoprules 1|0|reset' (Armadillo gun/general store slots are locations: buying sends a check to rdr1_ap_outbox.txt, bought weapons are removed unless AP-given).
// NEW in v5.13: 'shot' death category (non-lawman gun kill); 'shopprice <handle> <price>', 'shopwatch 1|0' (re-applies shopblock/shopprice, logs price/quantity changes), 'shopclear'; shopblock now rejects bad syntax.
// NEW in v5.14: "removeweapon <name|number>" and "removeall [keep a, b]" take weapons away again (relock + forget).
// v5.12: the in-game menu / log overlay (Insert, F5, F6, Home, arrows, Enter) was REMOVED; the console probe no longer
//   logs when nothing was typed; 'ui console 1' became 'consolemirror 1'.
// NEW in v5.11: stray-weapon reclaim (guns gained outside AP are marked and removed after the mission / 'reclaim'),
//   shop probe ('shopscan <n>' with a shop menu open, 'shopblock <handle> <0|1>').
// NEW in v5.10: every line of the input file is its own command; actor-scan timing ("perf:" lines); Dead Eye level re-applied
//   every 5s and never during Dead Eye; watchers cutscene_playing / player_controllable_*; menu status shows safe=; commands
//   'weaponlock <w> <0|1>' and 'equip <w>' (tests for gun-shop locking and for putting an owned weapon back on the wheel).
// NEW in v5.9c: menu also opens with F5 (the other RedHook trainer uses F5 + Enter); console probe (type into the F8
//   console, see 'console command:' lines), 'consolelog <text>' and 'ui console 1'.
// NEW in v5.9: DeathLink cause detection + cowboy death messages ('player died: cause=...'), 'deathlink <text>' to receive,
//   'deathmsg <category>' to preview.
// NEW in v5.8: in-game menu (Insert) and log overlay (Home); Dead Eye progression 'deadeyelevel 0-4' (0 = Dead Eye disabled,
//   -1 = game decides) which is re-applied every 2 seconds; 'ui <setting> <value>' to tune the overlay.
// NEW in v5.7: objective-hash mission recorder (rdr1_ap_objectives.csv) and completion-detector prototype
//   (rdr1_ap_missions.txt); commands: mission <name>, missionend passed|failed, loadmissions.
// NEW in v5.6: 'quiet 1' / 'quiet 0' switches the plugin's own on-screen messages off/on (use it during mission tests so
//   stat_message_showing only reacts to the game). The bow was removed from the weapon list (not in the base game).
// NEW in v5.5: 'deadeye <multilock|paint|head|disable|infinite|locks|points|maxpoints|regen|timescale|status> <value>',
//   'die' (kills the player, for DeathLink) and 'statfind <number>' (looks for a stat equal to a number on screen, e.g. cash).
// NEW in v5.4: 'statdump' command (input file) lists stats that look like mission medals (values 1-4).
// NEW in v5.3: is_mission_script watcher removed (it reports on THIS script, so it never changes); added
//   stat_message_showing (the fame / mission-passed popup) as a possible mission-complete signal.
// NEW in v5.2: locked weapons are unlocked before giving (fixes the Schofield etc.), throwables/knife/lasso/bow added to
//   the weapon list, spawned animals are cleaned up (spawns started failing after ~60), failed spawns are logged,
//   'player hit: ... by <actor>' lines show what actually hurt you, and 'animalclear' removes spawned animals.
// NEW in v5.1: extra mission watchers (is_mission_script, last_printed_objective, last_note_objective, targeted entry progress).
// NEW in v4.2: also logs "hat change: ..." whenever any actor gains or loses its hat (hat trick test).
//
// NEW in v4 - kill / hit watcher (always on):
//   Every 0.2s all nearby actors are checked. The log gets
//     "hit by player: ..."  whenever YOU damage an actor (with hit zone, hit flags, weapon, damage, faction...)
//     "kill: by_player=1 ..." whenever an actor dies (by_player says whether you were the last attacker)
//   Use it to test kills and hat tricks: do the thing, then drop a note file (see below) so I can match the
//   log line to what you did.
//
// Files in the game folder (same folder as rdr1_ap_test.log):
//   rdr1_ap_in.txt       drop a file containing one of: weapon / ammo / spawn  (acts like pressing F9/F10/F11),
//                        NEW in v5 commands:
//                          weapon <number|name>     give a specific weapon, e.g. "weapon schofield" or "weapon 2"
//                          weapon swap <number|name> same, but if it still fails, REMOVE other held weapons of the same
//                                                   category first (you lose them!) to test the slot theory
//                          weapons                  log what you carry, lock state and category of every test weapon
//                          spawn <animal> [recipe]  cougar bear grizzly wolf coyote boar bobcat snake rabbit, recipe 0-3
//                          notoriety <number>       bounty probe: SET_PLAYER_CURRENT_NOTORIETY (try 0.5, 1, 10, 100...)
//                          wantedfx <number|off>    bounty probe: wanted-poster screen effect level
//                          paybounty <number>       bounty probe: SET_PAYING_BOUNTY (try last)
//                        or any other text. Any other text is shown on screen AND written to the log with a
//                        timestamp, so use it to label what you just did (for example "shot hat off").
//                        More commands for that file (v4.1):
//                          additem <Name> <count>   try INVENTORY::ADD_ITEM, e.g. "additem Cash 50". The log
//                                                   records the return value; watch your money on screen.
//                          statmax <n>              scan stat ids 0..n-1 (16..4096). Raises crash risk if the
//                                                   game has fewer stats; the log shows the last id read.
//                          statreset                throw away the stat baseline and recalibrate
//   rdr1_ap_scripts.txt  OPTIONAL. One script name per line. Each is polled with
//                        _IS_ANY_NAMED_SCRIPT_RUNNING and logged when it starts or stops.

static const char* LOG_PATH = "rdr1_ap_test.log";

// v5.22: every multiworld gets its own save folder, rdr1_ap_seeds\<seed>\ (progress, received weapons, shop purchases, outbox).
// Old test files in the game folder are never sent; switching seeds switches folders, so an old run can be picked up again.
static std::string g_seedDir;

static std::string ApFile(const char* _Name)
{
	return g_seedDir.empty() ? std::string(_Name) : g_seedDir + "\\" + _Name;
}
static const char* IN_PATH = "rdr1_ap_in.txt";
static const char* SCRIPTS_PATH = "rdr1_ap_scripts.txt";

// How many stat ids to scan with the stat key. 256 ran fine. If the game ever crashes during a scan, the last
// "stat scan: reading id N" line in the log shows how far it got.
// Can be changed while playing: put "statmax 512" in rdr1_ap_in.txt (the stat tracking then recalibrates).
static int g_statScanMax = 256;

// How far from the player the test cougar spawns.
static const float COUGAR_SPAWN_DISTANCE = 14.0f;   // Beta: was 4 m, ambushes were too close to fight back

static const int MAX_ACTORS = 4096;



struct WeaponEntry
{
	WeaponModel id;
	const char* name;
};

static const WeaponEntry TEST_WEAPONS[] =
{
	{ WEAPON_REVOLVER_Cattleman, "Cattleman Revolver" },
	{ WEAPON_REVOLVER_Schofield, "Schofield Revolver" },
	{ WEAPON_REVOLVER_DoubleAction, "Double Action Revolver" },
	{ WEAPON_REVOLVER_Lemat, "LeMat Revolver" },
	{ WEAPON_PISTOL_Volcanic, "Volcanic Pistol" },
	{ WEAPON_PISTOL_SemiAuto, "Semi-Auto Pistol" },
	{ WEAPON_PISTOL_HighPower, "High Power Pistol" },
	{ WEAPON_PISTOL_Mauser, "Mauser Pistol" },
	{ WEAPON_REPEATER_Carbine, "Carbine Repeater" },
	{ WEAPON_REPEATER_Winchester, "Winchester Repeater" },
	{ WEAPON_REPEATER_Henry, "Henry Repeater" },
	{ WEAPON_REPEATER_Evans, "Evans Repeater" },
	{ WEAPON_RIFLE_Springfield, "Springfield Rifle" },
	{ WEAPON_RIFLE_BoltAction, "Bolt Action Rifle" },
	{ WEAPON_RIFLE_Buffalo, "Buffalo Rifle" },
	{ WEAPON_SHOTGUN_SawedOff, "Sawed-Off Shotgun" },
	{ WEAPON_SHOTGUN_DoubleBarrel, "Double Barrel Shotgun" },
	{ WEAPON_SHOTGUN_PumpAction, "Pump Action Shotgun" },
	{ WEAPON_SHOTGUN_SemiAuto, "Semi-Auto Shotgun" },
	{ WEAPON_SNIPERRIFLE_RollingBlock, "Rolling Block Rifle" },
	{ WEAPON_SNIPERRIFLE_Carcano, "Carcano Rifle" },
	{ WEAPON_THROWN_ThrowingKnife, "Throwing Knives" },
	{ WEAPON_THROWN_FireBottle, "Fire Bottles" },
	{ WEAPON_THROWN_Dynamite, "Dynamite" },
	{ WEAPON_THROWN_Tomahawk, "Tomahawk" },
	{ WEAPON_MELEE_Knife, "Knife" },
	{ WEAPON_LASSO_Lasso, "Lasso" },
	{ WEAPON_BOW_ShortBow, "Short Bow" },   // #28: Undead Nightmare weapon, not part of the randomizer; listed so it can be removed
};

static const int TEST_WEAPON_COUNT = (int)(sizeof(TEST_WEAPONS) / sizeof(TEST_WEAPONS[0]));

static int g_weaponIndex = 0;

// Weapons the (future) client gave us, weapons that showed up some other way (mission rewards, pickups), and
// the guns the player owned last time we looked.
static std::set<int> g_apGranted;
static std::string g_lastMissionScript;   // script of the story mission in progress ("" = free roam)
static bool g_apMode = false;            // v5.21: true once the client has written an items file for a seed ("apmode 1" for tests)
static bool g_vanillaWeapons = false;    // v5.21: option weapon_mode vanilla: weapons are not items, nothing is taken away
static ULONGLONG g_syncQuietUntil = 0;   // v5.20: the weapon watch ignores changes until this time (set after a sync)
static std::set<int> g_pendingReclaim;
static std::set<int> g_prevOwned;
static bool g_ownedKnown = false;



static std::deque<std::string> g_recentLog;
static bool g_consoleMirror = false;   // "ui console 1": also print every log line into the F8 console (CORE::_LOG_MESSAGE)

static void Log(const std::string& _Line)
{
	// v5.20k: keep the log stream open (was reopened on every line) and flush every line so a crash loses nothing.
	static std::ofstream file(LOG_PATH, std::ios::app);

	file << GetTickCount64() << " " << _Line << std::endl;

	g_recentLog.push_back(_Line);

	if (g_consoleMirror)
	{
		CORE::_LOG_MESSAGE(_Line.c_str());
	}

	while (g_recentLog.size() > 12)
	{
		g_recentLog.pop_front();
	}
}



// "quiet 1" in the input file turns all on-screen messages off (they trip stat_message_showing, which we want clean).
static bool g_quiet = false;

static void Say(const std::string& _Text)
{
	if (g_quiet)
	{
		return;
	}

	HUD::PRINT_HELP_B(_Text.c_str(), 6.0f, true, 1, 0, 0, 0, 0);
}

// v5.21 alpha: Archipelago styled messages. "uitags 0" strips the colour tags if the game ever prints them literally.
static bool g_uiTags = false;   // v5.22: off by default, the help box only showed the first tagged line

static std::string Tag(const char* _Color, const std::string& _Text)
{
	return g_uiTags ? std::string("<") + _Color + ">" + _Text + "</" + _Color + ">" : _Text;
}

static void ApSay(const std::string& _Body)
{
	// v5.22: one plain line. The tagged header + new lines only ever showed "ARCHIPELAGO" in game.
	std::string line = _Body;

	for (size_t pos = line.find('\n'); pos != std::string::npos; pos = line.find('\n', pos)) { line.replace(pos, 1, " - "); }

	Say(line);
}

// Big centred title (the game's own "mission" style text) plus a help line, used for traps.
// Beta: only the big centred text (no top-left help box). PRINT_BIG has no size argument; its 4th argument is a style the
// scripts use as 0 or 2, so "bigstyle <n>" lets us try the others for a smaller look.
static int g_bigStyle = 2;

static void ApBig(const std::string& _Title, const std::string& _Body)
{
	if (g_quiet) { return; }

	std::string text = _Title + (_Body.empty() ? "" : " - " + _Body);

	HUD::PRINT_BIG(text.c_str(), 3.0f, true, g_bigStyle, 0);
}



static std::string Trim(const std::string& _Text)
{
	size_t a = _Text.find_first_not_of(" \t\r\n");
	size_t b = _Text.find_last_not_of(" \t\r\n");

	if (a == std::string::npos)
	{
		return "";
	}

	return _Text.substr(a, b - a + 1);
}



static bool KeyEdge(KeyCode _Key, bool& _WasDown)
{
	bool down = REDHOOK::IS_KEY_PRESSED(_Key);

	bool edge = down && !_WasDown;

	_WasDown = down;

	return edge;
}



static float Distance(const Vector3& _A, const Vector3& _B)
{
	float dx = _A.x - _B.x;
	float dy = _A.y - _B.y;
	float dz = _A.z - _B.z;

	return std::sqrt(dx * dx + dy * dy + dz * dz);
}



// ---------------------------------------------------------------------------------------------
// Change watcher: logs a value the first time it is seen and every time it changes afterwards.
// ---------------------------------------------------------------------------------------------

static std::map<std::string, long long> g_lastValues;

static void WatchValue(const std::string& _Name, long long _Value)
{
	auto it = g_lastValues.find(_Name);

	if (it == g_lastValues.end())
	{
		g_lastValues[_Name] = _Value;

		Log("watch " + _Name + " = " + std::to_string(_Value));

		return;
	}

	if (it->second != _Value)
	{
		Log("watch " + _Name + ": " + std::to_string(it->second) + " -> " + std::to_string(_Value));

		it->second = _Value;
	}
}



// ---------------------------------------------------------------------------------------------
// Item tests
// ---------------------------------------------------------------------------------------------

static std::string WeaponLabel(WeaponModel _Weapon)
{
	const char* name = WEAPON::GET_WEAPON_INTERNAL_NAME(_Weapon);

	return std::to_string((int)_Weapon) + "(" + (name ? name : "?") + ")";
}



// What the game says the player is carrying: the weapon wheel slots and the weapon in hand.
static std::string WheelDump(Actor _Player)
{
	std::string text = "in_hand=" + WeaponLabel(INVENTORY::GET_WEAPON_IN_HAND(_Player)) + " wheel:";

	for (int i = 0; i < 12; i++)
	{
		text += " [" + std::to_string(i) + "]=" + WeaponLabel(INVENTORY::GET_WEAPON_EQUIPPED(_Player, i));
	}

	return text;
}



// Logs, for every weapon in the test list, whether the player has it, whether it is locked, its category and ammo.
static void DumpWeapons()
{
	Actor player = ACTOR::GET_PLAYER_ACTOR(-1);

	Log("weapon dump: " + WheelDump(player));

	for (int i = 0; i < TEST_WEAPON_COUNT; i++)
	{
		const WeaponEntry& weapon = TEST_WEAPONS[i];

		Log("weapon dump: #" + std::to_string(i + 1) + " " + weapon.name
			+ " id=" + WeaponLabel(weapon.id)
			+ " has=" + std::to_string(INVENTORY::ACTOR_HAS_WEAPON(player, weapon.id))
			+ " locked=" + std::to_string(WEAPON::IS_WEAPONENUM_LOCKED(weapon.id))
			+ " category=" + std::to_string((int)WEAPON::GET_WEAPON_CATEGORY_FROM_ENUM(weapon.id))
			+ " ammo_type=" + std::to_string((int)WEAPON::GET_AMMO_ENUM(weapon.id))
			+ " weapon_ammo=" + std::to_string(INVENTORY::ACTOR_GET_WEAPON_AMMO(player, weapon.id)));
	}

	Say("Weapon dump written to the log");
}



// Gives one weapon and tells the log exactly which attempt worked.
//   step 1: plain give
//   step 2: unlock the weapon first (if the game says it is locked), then give
//   step 3: give with the third argument set to 1
//   step 4 (only when _Swap is true): delete every OTHER test weapon of the same category from the player, then give.
//           This removes weapons from the player! It tests whether a category slot is already taken.
static void GiveWeapon(int _Index, bool _Swap)
{
	Actor player = ACTOR::GET_PLAYER_ACTOR(-1);

	const WeaponEntry& weapon = TEST_WEAPONS[_Index];

	bool locked = WEAPON::IS_WEAPONENUM_LOCKED(weapon.id);
	WeaponCategory category = WEAPON::GET_WEAPON_CATEGORY_FROM_ENUM(weapon.id);
	bool hadBefore = INVENTORY::ACTOR_HAS_WEAPON(player, weapon.id);

	Log(std::string("give weapon: begin ") + weapon.name + " id=" + WeaponLabel(weapon.id)
		+ " locked=" + std::to_string(locked) + " category=" + std::to_string((int)category)
		+ " had_before=" + std::to_string(hadBefore) + " swap=" + std::to_string(_Swap)
		+ " " + WheelDump(player));

	int step = 1;

	// v5 log: locked weapons (the Schofield and most others) only arrive after being unlocked, so unlock first.
	if (locked)
	{
		WEAPON::SET_WEAPONENUM_LOCKED(weapon.id, false);
	}

	INVENTORY::GIVE_WEAPON_TO_ACTOR(player, weapon.id, 0, true, false);

	bool has = INVENTORY::ACTOR_HAS_WEAPON(player, weapon.id);

	if (!has)
	{
		step = 2;

		if (locked)
		{
			WEAPON::SET_WEAPONENUM_LOCKED(weapon.id, false);
		}

		INVENTORY::GIVE_WEAPON_TO_ACTOR(player, weapon.id, 0, true, false);

		has = INVENTORY::ACTOR_HAS_WEAPON(player, weapon.id);
	}

	if (!has)
	{
		step = 3;

		INVENTORY::GIVE_WEAPON_TO_ACTOR(player, weapon.id, 1, true, false);

		has = INVENTORY::ACTOR_HAS_WEAPON(player, weapon.id);
	}

	if (!has && _Swap)
	{
		step = 4;

		for (int i = 0; i < TEST_WEAPON_COUNT; i++)
		{
			const WeaponEntry& other = TEST_WEAPONS[i];

			if (i != _Index && WEAPON::GET_WEAPON_CATEGORY_FROM_ENUM(other.id) == category && INVENTORY::ACTOR_HAS_WEAPON(player, other.id))
			{
				Log(std::string("give weapon: swap removes ") + other.name);

				INVENTORY::DELETE_WEAPON_FROM_ACTOR(player, other.id);
			}
		}

		INVENTORY::GIVE_WEAPON_TO_ACTOR(player, weapon.id, 0, true, false);

		has = INVENTORY::ACTOR_HAS_WEAPON(player, weapon.id);
	}

	if (has)
	{
		g_apGranted.insert((int)weapon.id);
		g_pendingReclaim.erase((int)weapon.id);
	}

	AmmoType ammo = WEAPON::GET_AMMO_ENUM(weapon.id);

	if (has && (int)ammo > 0)
	{
		INVENTORY::ACTOR_ADD_INV_AMMO(player, ammo, 20.0f, false, true);
	}

	Log(std::string("give weapon: result ") + weapon.name + " has_after=" + std::to_string(has)
		+ " worked_at_step=" + (has ? std::to_string(step) : std::string("none"))
		+ " ammo_type=" + std::to_string((int)ammo)
		+ " weapon_ammo=" + std::to_string(INVENTORY::ACTOR_GET_WEAPON_AMMO(player, weapon.id))
		+ " " + WheelDump(player));

	if (!has) { ApSay(Tag("red", "Could not give ") + weapon.name + " (see log)"); }   // v5.21: success is announced by the caller
}



static void GiveNextWeapon()
{
	GiveWeapon(g_weaponIndex, false);

	g_weaponIndex = (g_weaponIndex + 1) % TEST_WEAPON_COUNT;
}



static std::string Lower(std::string _Text)
{
	for (char& c : _Text)
	{
		c = (char)std::tolower((unsigned char)c);
	}

	return _Text;
}



// "weapon", "weapon 2", "weapon schofield", "weapon swap schofield", "weapon swap 2"
static void WeaponCommand(const std::string& _Args)
{
	std::string args = _Args;

	bool swap = false;

	if (args.rfind("swap", 0) == 0)
	{
		swap = true;

		size_t start = args.find_first_not_of(" \t", 4);

		args = start == std::string::npos ? std::string() : args.substr(start);
	}

	if (args.empty())
	{
		GiveNextWeapon();

		return;
	}

	int index = -1;

	if (args.find_first_not_of("0123456789") == std::string::npos)
	{
		index = std::atoi(args.c_str()) - 1;
	}
	else
	{
		std::string needle = Lower(args);

		for (int i = 0; i < TEST_WEAPON_COUNT; i++)
		{
			if (Lower(TEST_WEAPONS[i].name).find(needle) != std::string::npos)
			{
				index = i;

				break;
			}
		}
	}

	if (index < 0 || index >= TEST_WEAPON_COUNT)
	{
		Log("weapon command: no weapon matches '" + args + "'");

		Say("No weapon matches: " + args);

		return;
	}

	GiveWeapon(index, swap);
}



static void MaxAmmo()
{
	Actor player = ACTOR::GET_PLAYER_ACTOR(-1);

	const AmmoType types[] =
	{
		AMMO_TYPE_PISTOL, AMMO_TYPE_REVOLVER, AMMO_TYPE_REPEATER, AMMO_TYPE_RIFLE, AMMO_TYPE_SHOTGUN,
		AMMO_TYPE_SNIPERRIFLE, AMMO_TYPE_THROWING_KNIFE, AMMO_TYPE_FIRE_BOTTLE, AMMO_TYPE_DYNAMITE, AMMO_TYPE_TOMAHAWK,
	};

	for (AmmoType type : types)
	{
		float maxAmount = INVENTORY::ACTOR_GET_INV_AMMO_MAX_AMOUNT(player, type);

		float before = INVENTORY::ACTOR_GET_INV_AMMO(player, type, false);

		if (maxAmount > 0.0f)
		{
			INVENTORY::ACTOR_SET_INV_AMMO(player, type, maxAmount, false);
		}

		float after = INVENTORY::ACTOR_GET_INV_AMMO(player, type, false);

		Log("max ammo: type=" + std::to_string((int)type) + " max=" + std::to_string(maxAmount) + " before=" + std::to_string(before) + " after=" + std::to_string(after));
	}


}



// v5.15 gear probe: "itemdump [n]" lists the first n inventory entries (default 64) by index with their CRC, HAS_ITEM and
// count. Run it before and after "additem Bandolier" (or after buying gear) and compare. "itemdel <crc> [count]" tries
// DELETE_ITEM(player, crc, count) on a CRC taken from the dump.
static void ItemDump(int _Max)
{
	Actor player = ACTOR::GET_PLAYER_ACTOR(-1);

	Log("itemdump: begin, indexes 0.." + std::to_string(_Max - 1));

	char hex[16];

	for (int i = 0; i < _Max; i++)
	{
		int crc = INVENTORY::ACTOR_GET_ITEM_CRC_AT_INDEX(player, i);

		if (crc == 0 || crc == -1)
		{
			continue;
		}

		snprintf(hex, sizeof(hex), "%08X", (unsigned)crc);

		Log("itemdump: index=" + std::to_string(i) + " crc=0x" + hex
			+ " has=" + std::to_string(INVENTORY::HAS_ITEM(player, crc))
			+ " count=" + std::to_string(INVENTORY::GET_ITEM_COUNT(player, crc))
			+ " is_weapon=" + std::to_string(INVENTORY::IS_ITEM_WEAPON_BY_CRC(crc)));
	}

	Log("itemdump: end");

	Say("Item dump written to the log");
}

// v5.19: items are looked up by a hash of their name (Jenkins one-at-a-time, lower case; this is the guess to be tested).
static int NameHash(const std::string& _Name)
{
	unsigned int hash = 0;

	for (char c : _Name)
	{
		hash += (unsigned char)std::tolower((unsigned char)c);
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}

	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);

	return (int)hash;
}

static int ItemCrcFromText(const std::string& _Text)
{
	if (_Text.rfind("0x", 0) == 0 || (!_Text.empty() && _Text.find_first_not_of("0123456789") == std::string::npos))
	{
		return (int)std::strtoul(_Text.c_str(), nullptr, 0);
	}

	return NameHash(_Text);
}

// v5.20k: the decompiled scripts show HAS_ITEM("HONOR_ROSARY", actor) and DELETE_ITEM("HONOR_ROSARY", actor, 0): the item NAME comes
// first and the actor second. The old hash-first wrappers read garbage and crashed the game, so these call the natives directly.
static int ItemHasByName(const char* _Name, Actor _Actor)
{
	return Invoke<0xB426267D, int>(_Name, _Actor);
}

static void ItemDeleteByName(const char* _Name, Actor _Actor, int _Count)
{
	Invoke<0xEFECF4F9, void>(_Name, _Actor, _Count);
}

static void ItemDelete(const std::string& _Args)
{
	Actor player = ACTOR::GET_PLAYER_ACTOR(-1);

	std::string name = Trim(_Args);
	int count = 1;

	size_t space = name.find_last_of(' ');

	if (space != std::string::npos)
	{
		std::string tail = Trim(name.substr(space + 1));

		if (!tail.empty() && tail.find_first_not_of("0123456789") == std::string::npos)
		{
			count = std::atoi(tail.c_str());

			name = Trim(name.substr(0, space));
		}
	}

	int before = ItemHasByName(name.c_str(), player);

	Log("itemdel: '" + name + "' has_before=" + std::to_string(before));

	if (!before)
	{
		Log("itemdel: not owned, nothing deleted");

		return;
	}

	ItemDeleteByName(name.c_str(), player, count);

	Log("itemdel: has_after=" + std::to_string(ItemHasByName(name.c_str(), player)));
}

static void ItemHas(const std::string& _Args)
{
	Actor player = ACTOR::GET_PLAYER_ACTOR(-1);

	std::string name = Trim(_Args);

	Log("itemhas: '" + name + "' has=" + std::to_string(ItemHasByName(name.c_str(), player)));
}

// v5.20k script globals. RedHook exposes GetGlobalPtr(); the decompiled scripts keep the stat table in Global_54086[stat id] as
// floats (0 money, 1 honor, 3 notoriety, 9 missions completed, 10 missions failed, 11 missions replayed, 26.. kills by type).
// The slot size is not known, so both 8-byte and 4-byte slots are logged and compared against a stat we know (money).
static void GlobalReadCommand(const std::string& _Args)
{
	int index = 0, count = 1;

	{
		char* end = nullptr;

		index = (int)std::strtol(_Args.c_str(), &end, 10);
		count = (int)std::strtol(end, nullptr, 10);
	}

	if (count < 1) { count = 1; }
	if (count > 64) { count = 64; }

	uintptr_t base = GetGlobalPtr();

	if (base == 0 || index < 0 || index > 200000)
	{
		Log("globalread: bad base or index, base=" + std::to_string((unsigned long long)base));

		return;
	}

	for (int i = 0; i < count; i++)
	{
		const unsigned char* p8 = (const unsigned char*)(base + (uintptr_t)(index + i) * 8);
		const unsigned char* p4 = (const unsigned char*)(base + (uintptr_t)(index + i) * 4);

		float f8, f4;
		long long i8;
		int i4;

		std::memcpy(&f8, p8, 4);
		std::memcpy(&i8, p8, 8);
		std::memcpy(&f4, p4, 4);
		std::memcpy(&i4, p4, 4);

		Log("global " + std::to_string(index + i) + ": slot8 int64=" + std::to_string(i8) + " float=" + std::to_string(f8)
			+ " | slot4 int=" + std::to_string(i4) + " float=" + std::to_string(f4));
	}
}

// v5.20m: the stat table is an array of 8-byte script slots (float in the low 4 bytes), element 0 is the count, so stat id N lives
// at 4-byte index 54086 + 2 * (N + 1). Money was confirmed this way (442 read as 442, missions completed 4).
static float* StatPtr(int _Id)
{
	uintptr_t base = GetGlobalPtr();

	if (base == 0) { return nullptr; }

	return (float*)(base + (uintptr_t)(54086 + 2 * (_Id + 1)) * 4);
}

static float StatGet(int _Id)
{
	float* p = StatPtr(_Id);

	return p ? *p : -1.0f;
}

// v5.20m: the game's scripts redraw the HUD cash with UI_SET_MONEY("nMoneyText", "Dmoney_amount", money) after every payout.
// The hash is the Jenkins hash of the native name (same scheme as GET_PLAYER_ACTOR = 0xE8CFDD53), not in the SDK header.
static void RefreshMoneyHud()
{
	float* p = StatPtr(0);

	if (!p) { return; }

	Invoke<0xF71BD93A, void>("nMoneyText", "Dmoney_amount", (int)*p);

	Log("hud money redrawn with " + std::to_string((int)*p));
}

// v5.21 alpha: the game's own popups. Cash uses PRINT_MONEY exactly like the mission scripts do (label "sn_mny", 7.5 s,
// "HUD_MENU_MESSAGE_BOX_ALERT_MASTER"). Honor and fame use SET_STAT_MESSAGE like the scripts' STAT_INCREASE helper
// (label hash, "+75" text, 3 s, ...). "statpopups 0" switches them off if one ever misbehaves.
static bool g_statPopups = true;

static void StatPopup(int _Id, float _Before, float _After)
{
	if (g_quiet || !g_statPopups) { return; }

	int delta = (int)std::lround(_After - _Before);

	if (delta == 0) { return; }

	if (_Id == 0)
	{
		HUD::PRINT_MONEY("sn_mny", (int)std::lround(_Before), delta, 7.5f, 0.0f, "HUD_MENU_MESSAGE_BOX_ALERT_MASTER");

		return;
	}

	const char* label = _Id == 1 ? "sn_hnrr" : (_Id == 3 ? "sn_ntrr" : nullptr);

	if (label == nullptr) { return; }

	std::string text = std::string(delta > 0 ? "+" : "-") + std::to_string(delta > 0 ? delta : -delta);

	int percent = _Id == 3 ? (int)(std::min)(100.0f, (std::max)(0.0f, _After / 50.0f)) : -1;

	HUD::SET_STAT_MESSAGE(NameHash(label), text.c_str(), 3.0f, "", 2, (float)percent, 0, delta > 0 ? 0 : 1, _Id == 3 ? 1 : 0, 0, 0, 0, 0);
}

// "statadd|statsub|statset <name> <value>": name is cash, honor, fame, bounty or a raw stat id. Logs before and after.
static int StatIdFromName(const std::string& _Name)
{
	std::string n = Lower(_Name);

	if (n == "cash" || n == "money") return 0;
	if (n == "honor") return 1;
	if (n == "fame") return 3;   // sn_ntrr (notoriety raw, +1 on a civilian kill)
	if (n == "bounty") return 223;   // sn_btus (US bounty); sn_bttl (222) and sn_bbo (252) moved with it, StatChangeCommand keeps them in step
	if (!n.empty() && n.find_first_not_of("0123456789") == std::string::npos) return std::atoi(n.c_str());

	return -1;
}

static void StatChangeCommand(const std::string& _Mode, const std::string& _Args)
{
	std::string args = Trim(_Args);
	size_t space = args.find(' ');

	if (space == std::string::npos) { Log("stat" + _Mode + ": usage <name> <value>"); return; }

	int id = StatIdFromName(Trim(args.substr(0, space)));
	float value = (float)std::atof(Trim(args.substr(space + 1)).c_str());
	float* p = (id >= 0) ? StatPtr(id) : nullptr;

	if (!p) { Log("stat" + _Mode + ": unknown stat or no global pointer"); return; }

	float before = *p;

	if (_Mode == "add") *p = before + value;
	else if (_Mode == "sub") *p = (id == 1) ? (before - value) : (std::max)(0.0f, before - value);   // honor may go negative
	else *p = value;

	Log("stat" + _Mode + ": id " + std::to_string(id) + " " + std::to_string(before) + " -> " + std::to_string(*p));

	if (id == 0 || id == 1 || id == 3) { StatPopup(id, before, *p); }

	if (id == 0) { RefreshMoneyHud(); }

	if (id == 1)
	{
		// the game keeps honor between -5000 and 5000 and tells the HUD with SET_PLAYER_CURRENT_HONOR(player 0, raw) (decompiled scripts)
		if (*p > 5000.0f) { *p = 5000.0f; }
		if (*p < -5000.0f) { *p = -5000.0f; }

		Invoke<0x4D918005, void>(0, *p);

		Log("stat" + _Mode + ": honor refreshed with SET_PLAYER_CURRENT_HONOR(0, " + std::to_string(*p) + ")");
	}
	else if (id == 3)
	{
		Invoke<0x4B0D6152, void>(0, *p);

		Log("stat" + _Mode + ": fame refreshed with SET_PLAYER_CURRENT_NOTORIETY(0, " + std::to_string(*p) + ")");
	}

	if (id == 223)
	{
		// bounty: the total (222) and the other counter (252) were 40 together with sn_btus after one $40 crime
		for (int other : { 222, 252 })
		{
			float* q = StatPtr(other);

			if (q) { *q = *p; Log("stat" + _Mode + ": id " + std::to_string(other) + " kept in step -> " + std::to_string(*q)); }
		}
	}
}

// "cashadd <n>": experimental cash filler. Writes money into the stat slot and logs before/after (watch the HUD).
static void CashAddCommand(int _Amount)
{
	float* p = StatPtr(0);

	if (!p) { Log("cashadd: no global pointer"); return; }

	float before = *p;

	*p = before + (float)_Amount;

	Log("cashadd: " + std::to_string(before) + " -> " + std::to_string(*p));

	RefreshMoneyHud();

	StatPopup(0, before, *p);
}

// "stats": money, honor, notoriety, missions completed/failed/replayed and the first kill counters, from Global_54086.
static void StatsCommand()
{
	static const int IDS[] = { 0, 1, 2, 3, 4, 8, 9, 10, 11, 26, 27, 28, 29, 30, 31, 408, 409 };

	for (int id : IDS)
	{
		Log("stat " + std::to_string(id) + ": " + std::to_string(StatGet(id)));
	}
}

// Cash probe: INVENTORY::ADD_ITEM takes an item NAME. We do not know the name of the money item yet, so this lets
// you try names from the input file without rebuilding.
static void AddItem(const std::string& _Name, int _Count)
{
	Actor player = ACTOR::GET_PLAYER_ACTOR(-1);

	int result = INVENTORY::ADD_ITEM(_Name.c_str(), player, _Count);

	Log("additem: name=" + _Name + " count=" + std::to_string(_Count) + " result=" + std::to_string(result));

	Say("additem " + _Name + " x" + std::to_string(_Count) + " -> " + std::to_string(result));
}



static int FindWeaponIndex(const std::string& _Text)
{
	if (_Text.empty())
	{
		return -1;
	}

	if (_Text.find_first_not_of("0123456789") == std::string::npos)
	{
		int index = std::atoi(_Text.c_str()) - 1;

		return (index >= 0 && index < TEST_WEAPON_COUNT) ? index : -1;
	}

	std::string needle = Lower(_Text);

	for (int i = 0; i < TEST_WEAPON_COUNT; i++)
	{
		if (Lower(TEST_WEAPONS[i].name).find(needle) != std::string::npos)
		{
			return i;
		}
	}

	return -1;
}

static bool OnWheel(Actor _Player, WeaponModel _Weapon)
{
	for (int i = 0; i < 12; i++)
	{
		if (INVENTORY::GET_WEAPON_EQUIPPED(_Player, i) == _Weapon)
		{
			return true;
		}
	}

	return false;
}

// "weaponlock schofield 1" locks it again, "weaponlock schofield 0" unlocks it. Question this answers: if the plugin
// re-locks every weapon the player has not received yet, can they still buy it in a gun shop?
static void WeaponLockCommand(const std::string& _Args)
{
	size_t space = _Args.find_last_of(' ');

	if (space == std::string::npos)
	{
		Log("weaponlock: use  weaponlock <name|number> <0|1>");

		return;
	}

	int index = FindWeaponIndex(Trim(_Args.substr(0, space)));

	if (index < 0)
	{
		Log("weaponlock: no weapon matches '" + _Args + "'");

		return;
	}

	bool lock = Trim(_Args.substr(space + 1)) != "0";

	WEAPON::SET_WEAPONENUM_LOCKED(TEST_WEAPONS[index].id, lock);

	Log(std::string("weaponlock: ") + TEST_WEAPONS[index].name + " locked=" + std::to_string(WEAPON::IS_WEAPONENUM_LOCKED(TEST_WEAPONS[index].id))
		+ " owned=" + std::to_string(INVENTORY::ACTOR_HAS_WEAPON(ACTOR::GET_PLAYER_ACTOR(-1), TEST_WEAPONS[index].id)));

	Say(std::string(TEST_WEAPONS[index].name) + (lock ? " locked" : " unlocked"));
}

// v5.14: takes one weapon away and puts it back in the "never received" state: deleted from the inventory, locked again
// (so gun shops block it like any weapon that has not been received yet), and forgotten by the AP-granted list and the
// stray-weapon list. Returns true if the weapon was owned before.
static bool RemoveWeaponByIndex(int _Index)
{
	Actor player = ACTOR::GET_PLAYER_ACTOR(-1);

	const WeaponEntry& entry = TEST_WEAPONS[_Index];

	bool owned = INVENTORY::ACTOR_HAS_WEAPON(player, entry.id);

	if (owned)
	{
		INVENTORY::DELETE_WEAPON_FROM_ACTOR(player, entry.id);
	}

	WEAPON::SET_WEAPONENUM_LOCKED(entry.id, true);

	g_apGranted.erase((int)entry.id);
	g_pendingReclaim.erase((int)entry.id);

	g_ownedKnown = false;

	// v5.19e: a weapon that was removed while it was the selected one (a bought, still-locked gun) leaves the holster stuck, so
	// the Knife is put back in hand to reset it. The bow is skipped (it is never in hand).
	if (owned && GetTickCount64() >= g_syncQuietUntil && entry.id != WEAPON_MELEE_Knife && entry.id != WEAPON_LASSO_Lasso && entry.id != WEAPON_BOW_ShortBow
		&& GAME::GET_GAME_STATE() == 3 && !CUTSCENE::CUTSCENE_MANAGER_IS_CUTSCENE_PLAYING())
	{
		int inHand = (int)INVENTORY::GET_WEAPON_IN_HAND(player);

		if (inHand == -1 || inHand == (int)entry.id)
		{
			INVENTORY::ACTOR_PUT_WEAPON_IN_HAND(player, WEAPON_MELEE_Knife, true);

			Log("removeweapon: holster reset, Knife put in hand");
		}
	}

	Log(std::string("removeweapon: ") + entry.name + " was_owned=" + std::to_string(owned)
		+ " still_owned=" + std::to_string(INVENTORY::ACTOR_HAS_WEAPON(player, entry.id))
		+ " locked=" + std::to_string(WEAPON::IS_WEAPONENUM_LOCKED(entry.id)));

	return owned;
}

// "removeweapon schofield" or "removeweapon 7".
static void RemoveWeaponCommand(const std::string& _Args)
{
	int index = FindWeaponIndex(_Args);

	if (index < 0)
	{
		Log("removeweapon: no weapon matches '" + _Args + "'  (use  removeweapon <name|number>)");

		return;
	}

	bool owned = RemoveWeaponByIndex(index);

	Say(std::string(TEST_WEAPONS[index].name) + (owned ? " removed" : " was not owned"));
}

// "removeall" removes every weapon in the list. "removeall keep cattleman, lasso" keeps the ones named (comma separated,
// names or numbers).
static void RemoveAllCommand(const std::string& _Args)
{
	std::set<int> keep;

	std::string rest = Trim(_Args);

	if (rest.rfind("keep", 0) == 0)
	{
		rest = Trim(rest.substr(4));

		size_t start = 0;

		while (start <= rest.size())
		{
			size_t comma = rest.find(',', start);

			std::string token = Trim(rest.substr(start, comma == std::string::npos ? std::string::npos : comma - start));

			if (!token.empty())
			{
				int index = FindWeaponIndex(token);

				if (index >= 0) { keep.insert(index); } else { Log("removeall: keep '" + token + "' matches no weapon, ignored"); }
			}

			if (comma == std::string::npos)
			{
				break;
			}

			start = comma + 1;
		}
	}
	else if (!rest.empty())
	{
		Log("removeall: use  removeall  or  removeall keep <name>, <name>");

		return;
	}

	int removed = 0;

	for (int i = 0; i < TEST_WEAPON_COUNT; i++)
	{
		if (keep.count(i))
		{
			continue;
		}

		if (RemoveWeaponByIndex(i))
		{
			removed++;
		}
	}

	Log("removeall: removed " + std::to_string(removed) + " weapons, kept " + std::to_string(keep.size()));

	Say("Removed " + std::to_string(removed) + " weapons");
}

// Giving a weapon you already own does not move it back onto the wheel (seen in the v5.1 log). This tries to equip an
// OWNED weapon: step 1 puts it in hand, step 3 deletes it and gives it again.
static void EquipWeapon(int _Index)
{
	Actor player = ACTOR::GET_PLAYER_ACTOR(-1);

	const WeaponEntry& weapon = TEST_WEAPONS[_Index];

	if (!INVENTORY::ACTOR_HAS_WEAPON(player, weapon.id))
	{
		Log(std::string("equip: ") + weapon.name + " is not owned, giving it");

		GiveWeapon(_Index, false);

		return;
	}

	Log(std::string("equip: begin ") + weapon.name + " on_wheel=" + std::to_string(OnWheel(player, weapon.id)) + " " + WheelDump(player));

	int step = 0;

	if (!OnWheel(player, weapon.id))
	{
		step = 1;

		INVENTORY::ACTOR_PUT_WEAPON_IN_HAND(player, weapon.id, true);

		ScriptWait(100);
	}

	if (!OnWheel(player, weapon.id))
	{
		step = 3;

		INVENTORY::DELETE_WEAPON_FROM_ACTOR(player, weapon.id);

		WEAPON::SET_WEAPONENUM_LOCKED(weapon.id, false);

		INVENTORY::GIVE_WEAPON_TO_ACTOR(player, weapon.id, 0, true, false);
	}

	Log(std::string("equip: result ") + weapon.name + " on_wheel=" + std::to_string(OnWheel(player, weapon.id)) + " owned=" + std::to_string(INVENTORY::ACTOR_HAS_WEAPON(player, weapon.id))
		+ " last_step=" + std::to_string(step) + " " + WheelDump(player));

	Say(std::string("Equip: ") + weapon.name);
}



// ---------------------------------------------------------------------------------------------
// Trap test: random animal ambush, with several different ways of trying to make it hostile, plus tracking
// ---------------------------------------------------------------------------------------------

struct AnimalEntry
{
	const char* key;
	const char* label;
	ActorModel model;
	Species species;
	bool hostile;
	int weight;
};

static const AnimalEntry ANIMALS[] =
{
	{ "cougar",  "Cougar",       ACTOR_ANIMAL_NOR_Cougar,        SPECIES_COUGAR, true,  3 },
	{ "bear",    "Black Bear",   ACTOR_ANIMAL_NOR_Bear,          SPECIES_BEAR,   true,  2 },
	{ "grizzly", "Grizzly Bear", ACTOR_ANIMAL_NOR_GrizzlyBear,   SPECIES_BEAR,   true,  2 },
	{ "wolf",    "Wolf",         ACTOR_ANIMAL_NOR_Wolf,          SPECIES_WOLF,   true,  3 },
	{ "coyote",  "Coyote",       ACTOR_ANIMAL_Coyote,            SPECIES_COYOTE, true,  3 },
	{ "boar",    "Wild Boar",    ACTOR_ANIMAL_NOR_WildBoar,      SPECIES_BOAR,   true,  2 },
	{ "bobcat",  "Bobcat",       ACTOR_ANIMAL_MEX_BobCat,        SPECIES_BOBCAT, true,  2 },
	{ "snake",   "Snake",        ACTOR_ANIMAL_Snake,             SPECIES_SNAKE,  true,  1 },
	{ "rabbit",  "Rabbit",       ACTOR_ANIMAL_Rabbit02,          SPECIES_RABBIT, false, 1 },
};

static const int ANIMAL_COUNT = (int)(sizeof(ANIMALS) / sizeof(ANIMALS[0]));

static int g_animalRecipe = 0;
static const int ANIMAL_RECIPES = 4;
static int g_spawnCounter = 0;

static Actor g_trackedAnimal = -1;
static std::string g_trackedLabel;
static int g_trackedRecipe = -1;
static ULONGLONG g_trackStart = 0;
static ULONGLONG g_trackNext = 0;

// Every animal we create is remembered so it can be removed again. The v5.1 test showed spawns starting to
// return actor id 0 after ~60 animals, so old ones are destroyed after ANIMAL_LIFETIME_MS or once more than
// MAX_ALIVE_ANIMALS exist.
struct SpawnedAnimal
{
	Actor actor;
	ULONGLONG time;
};

static std::vector<SpawnedAnimal> g_spawned;
static const ULONGLONG ANIMAL_LIFETIME_MS = 60000;
static const size_t MAX_ALIVE_ANIMALS = 12;

static void DestroySpawned(size_t _Index, const char* _Why)
{
	Actor actor = g_spawned[_Index].actor;

	if (ENTITY::IS_ACTOR_VALID(actor))
	{
		OBJECT::DESTROY_ACTOR(actor);
	}

	Log(std::string("animal cleanup: destroyed actor ") + std::to_string((int)actor) + " (" + _Why + ")");

	g_spawned.erase(g_spawned.begin() + _Index);
}

static void CleanupAnimals(ULONGLONG _Now, bool _All)
{
	for (size_t i = 0; i < g_spawned.size();)
	{
		bool old = _Now > g_spawned[i].time + ANIMAL_LIFETIME_MS;

		if (_All || old || !ENTITY::IS_ACTOR_VALID(g_spawned[i].actor))
		{
			DestroySpawned(i, _All ? "animalclear" : old ? "lifetime" : "gone");

			continue;
		}

		i++;
	}

	while (g_spawned.size() > MAX_ALIVE_ANIMALS)
	{
		DestroySpawned(0, "too many alive");
	}
}

static int PickRandomAnimal()
{
	int total = 0;

	for (int i = 0; i < ANIMAL_COUNT; i++)
	{
		total += ANIMALS[i].weight;
	}

	int roll = std::rand() % total;

	for (int i = 0; i < ANIMAL_COUNT; i++)
	{
		if (roll < ANIMALS[i].weight)
		{
			return i;
		}

		roll -= ANIMALS[i].weight;
	}

	return 0;
}

// _Recipe -1 = next one in the cycle (0,1,2,3).
static void SpawnAnimal(int _AnimalIndex, int _Recipe)
{
	const AnimalEntry& animal = ANIMALS[_AnimalIndex];

	STREAM::STREAMING_REQUEST_ACTOR(animal.model, true, true);

	ULONGLONG start = GetTickCount64();

	while (!STREAM::STREAMING_IS_ACTOR_LOADED(animal.model, 0) && GetTickCount64() < start + 3000)
	{
		ScriptWait(0);
	}

	if (!STREAM::STREAMING_IS_ACTOR_LOADED(animal.model, 0))
	{
		Log(std::string("spawn ") + animal.key + ": model did not load");

		Say(std::string(animal.label) + " model did not load (see log)");

		return;
	}

	Actor player = ACTOR::GET_PLAYER_ACTOR(-1);

	Vector3 position = ACTOR::GET_POSITION(player);

	// Random direction around the player.
	float angle = (float)(std::rand() % 628) / 100.0f;

	float x = position.x + std::cos(angle) * COUGAR_SPAWN_DISTANCE;
	float y = position.y + std::sin(angle) * COUGAR_SPAWN_DISTANCE;

	Layout layout = OBJECT::FIND_NAMED_LAYOUT("PlayerLayout");

	std::string objectName = "ap_trap_animal_" + std::to_string(g_spawnCounter++);

	Actor actor = OBJECT::CREATE_ACTOR_IN_LAYOUT(layout, objectName.c_str(), animal.model, Vector2(x, y), position.z + 1.0f, Vector2(), 0.0f);

	if ((int)actor <= 0)
	{
		Log(std::string("spawn ") + animal.key + ": FAILED, CREATE_ACTOR_IN_LAYOUT returned " + std::to_string((int)actor) + " (spawned animals still alive: " + std::to_string(g_spawned.size()) + ")");

		Say(std::string(animal.label) + " spawn failed (see log)");

		return;
	}

	g_spawned.push_back({ actor, GetTickCount64() });

	int recipe = _Recipe;

	if (recipe < 0)
	{
		recipe = g_animalRecipe;

		g_animalRecipe = (g_animalRecipe + 1) % ANIMAL_RECIPES;
	}

	Log(std::string("spawn ") + animal.key + ": actor id " + std::to_string((int)actor) + " hostile=" + std::to_string(animal.hostile)
		+ " recipe " + (animal.hostile ? std::to_string(recipe) : std::string("none")) + " distance " + std::to_string(COUGAR_SPAWN_DISTANCE));

	if (animal.hostile)
	{
		AI_ANIMAL::ANIMAL_ACTOR_SET_DOCILE(actor, false);

		// Every parameter below is a guess (attacker, victim) taken from the SDK's unnamed arguments.
		switch (recipe)
		{
			case 0:
			{
				ENTITY::SET_ANIMAL_CAN_ATTACK(actor, 1);
				AI_MEMORY::MEMORY_CONSIDER_AS_ENEMY(actor, player);
				AI_MEMORY::MEMORY_ATTACK_ON_SIGHT(actor, player);
				TASKS::TASK_KILL_CHAR(actor, player);
			}
			break;
			case 1:
			{
				// Species-wide: affects all animals of this species until the game restarts.
				AI_ANIMAL::ANIMAL_SPECIES_REL_SET_CAN_ATTACK((int)animal.species, SPECIES_HUMAN, 1);
				TASKS::TASK_ANIMAL_CIRCLE_AGGRESSIVELY(actor, player);
			}
			break;
			case 2:
			{
				ENTITY::SET_ANIMAL_CAN_ATTACK(actor, 1);
				TASKS::TASK_MELEE_ATTACK(actor, player, 0);
			}
			break;
			default:
			{
				ENTITY::SET_ANIMAL_CAN_ATTACK(actor, 1);
				AI_ANIMAL::ANIMAL_SPECIES_REL_SET_CAN_ATTACK((int)animal.species, SPECIES_HUMAN, 1);
				AI_MEMORY::MEMORY_CONSIDER_AS_ENEMY(actor, player);
				AI_MEMORY::MEMORY_ATTACK_ON_SIGHT(actor, player);
				TASKS::TASK_MELEE_ATTACK(actor, player, 0);
			}
			break;
		}
	}

	g_trackedAnimal = actor;
	g_trackedLabel = animal.key;
	g_trackedRecipe = animal.hostile ? recipe : -1;
	g_trackStart = GetTickCount64();
	g_trackNext = 0;

	if (animal.hostile) { ApBig(std::string(animal.label) + " Ambush!", "Something is hunting you..."); }
	else { ApSay("A " + std::string(animal.label) + " wanders by. Harmless."); }
}


// v5.18 gang attack trap: "gangattack [walton|bandito|rebel|rustler|raider|miner|criminal] [count]" spawns count armed outlaws
// of that gang 10-14 m away and sets them on the player. Models come from the community eActorEnum list.
struct GangSpawn
{
	const char* key;
	int faction;               // -1 = leave the model's own faction
	ActorModel models[4];
};

static const GangSpawn GANG_SPAWNS[] =
{
	{ "walton",   25, { ACTOR_GANG_DRUNKNDIRTY_Medium01,  ACTOR_GANG_DRUNKNDIRTY_Medium02,  ACTOR_GANG_DRUNKNDIRTY_Medium03,  ACTOR_GANG_DRUNKNDIRTY_Easy01 } },
	{ "bandito",  12, { ACTOR_GANG_BANDITO_Medium01,      ACTOR_GANG_BANDITO_Medium02,      ACTOR_GANG_BANDITO_Medium03,      ACTOR_GANG_BANDITO_Easy01 } },
	{ "rebel",    27, { ACTOR_GANG_MEXICANREBEL_Medium01, ACTOR_GANG_MEXICANREBEL_Easy01,   ACTOR_GANG_MEXICANREBEL_Easy02,   ACTOR_GANG_MEXICANREBEL_Easy03 } },
	{ "rustler",  14, { ACTOR_GANG_CATTLERUSTLER_Medium01, ACTOR_GANG_CATTLERUSTLER_Medium02, ACTOR_GANG_CATTLERUSTLER_Medium03, ACTOR_GANG_CATTLERUSTLER_Easy01 } },
	{ "raider",   15, { ACTOR_GANG_INDIANRAIDER_Medium01, ACTOR_GANG_INDIANRAIDER_Medium02, ACTOR_GANG_INDIANRAIDER_Medium03, ACTOR_GANG_INDIANRAIDER_Easy01 } },
	{ "miner",    -1, { ACTOR_GANG_CRAZYMINER_Medium01,   ACTOR_GANG_CRAZYMINER_Medium02,   ACTOR_GANG_CRAZYMINER_Medium03,   ACTOR_GANG_CRAZYMINER_Easy01 } },
	{ "criminal", 13, { ACTOR_CAUCASIAN_GENERICCRIMINAL_Medium01, ACTOR_CAUCASIAN_GENERICCRIMINAL_Medium02, ACTOR_CAUCASIAN_GENERICCRIMINAL_Medium03, ACTOR_CAUCASIAN_GENERICCRIMINAL_Easy01 } },
	{ "lawmen",    8, { ACTOR_LAW_CAUCASIAN_SHERIFF_Medium01, ACTOR_LAW_CAUCASIAN_SHERIFF_Medium02, ACTOR_LAW_CAUCASIAN_SHERIFF_Medium03, ACTOR_LAW_CAUCASIAN_SHERIFF_Medium04 } },
	{ "zombie",   -1, { ACTOR_UN_ZOMBIE_Bruiser_01, ACTOR_UN_ZOMBIE_Fast_01, ACTOR_UN_ZOMBIE_Bruiser_02, ACTOR_UN_ZOMBIE_Fast_02 } },
};

static const int GANG_SPAWN_COUNT = (int)(sizeof(GANG_SPAWNS) / sizeof(GANG_SPAWNS[0]));

// v5.20m "crimeprobe [explosion] [size]": tries to make the game think John killed a civilian (so the law system creates a real crime and
// bounty). Spawns a victim and a witness (two farmers, passive) about 10 m away, then an explosion OWNED BY JOHN goes off on the victim.
// The explosion owner is the only native we know that attributes damage to a chosen actor. Watch stat_bounty / stat_honor in the log.
static void CrimeProbe(const std::string& _Args)
{
	std::string name = "ManureExplosion";
	float size = 1.5f;

	{
		std::string a = Trim(_Args);

		if (!a.empty())
		{
			size_t sp = a.find(' ');
			name = a.substr(0, sp);

			if (sp != std::string::npos) { size = (float)std::atof(Trim(a.substr(sp + 1)).c_str()); }
		}
	}

	Actor player = ACTOR::GET_PLAYER_ACTOR(-1);
	Vector3 position = ACTOR::GET_POSITION(player);
	Layout layout = OBJECT::FIND_NAMED_LAYOUT("PlayerLayout");

	ActorModel model = ACTOR_CAUCASIAN_FEMALE_Farmer01;

	STREAM::STREAMING_REQUEST_ACTOR(model, true, true);

	ULONGLONG start = GetTickCount64();

	while (!STREAM::STREAMING_IS_ACTOR_LOADED(model, 0) && GetTickCount64() < start + 3000) { ScriptWait(0); }

	if (!STREAM::STREAMING_IS_ACTOR_LOADED(model, 0)) { Log("crimeprobe: model did not load"); return; }

	float angle = (float)(std::rand() % 628) / 100.0f;
	Actor victim = 0, witness = 0;
	Vector3 victimPos = position;

	for (int n = 0; n < 2; n++)
	{
		float a = angle + 0.5f * (float)n;
		float d = 10.0f + 2.0f * (float)n;
		float x = position.x + std::cos(a) * d;
		float y = position.y + std::sin(a) * d;

		std::string objectName = "ap_crime_" + std::to_string(g_spawnCounter++);

		Actor actor = OBJECT::CREATE_ACTOR_IN_LAYOUT(layout, objectName.c_str(), model, Vector2(x, y), position.z + 1.0f, Vector2(), 0.0f);

		if ((int)actor <= 0) { Log("crimeprobe: spawn failed for actor " + std::to_string(n)); continue; }

		g_spawned.push_back({ actor, GetTickCount64() });

		if (n == 0) { victim = actor; victimPos.x = x; victimPos.y = y; } else { witness = actor; }
	}

	if ((int)victim <= 0) { Log("crimeprobe: no victim"); return; }

	Log("crimeprobe: victim " + std::to_string((int)victim) + " witness " + std::to_string((int)witness) + ", waiting 2 s before the blast");

	ULONGLONG t0 = GetTickCount64();

	while (GetTickCount64() < t0 + 2000) { ScriptWait(0); }

	Vector3 sphere;

	sphere.x = size; sphere.y = size; sphere.z = size;

	Vector3 blast = ACTOR::GET_POSITION(victim);

	Log("crimeprobe: explosion '" + name + "' size " + std::to_string(size) + " owner=player on the victim, victim health before="
		+ std::to_string(HEALTH::GET_ACTOR_HEALTH(victim)));

	EXPLOSION::_CREATE_EXPLOSION(&blast, name.c_str(), player, &sphere, false);

	Say("crimeprobe fired");
}

static float g_gangDistanceBase = 22.0f;   // v5.20m: metres from John where an attack group spawns (the bounty trap uses 40)

static void GangAttack(const std::string& _Args)
{
	std::string text = Lower(_Args);
	int count = 3;

	size_t space = text.find_last_of(' ');

	if (space != std::string::npos)
	{
		std::string tail = Trim(text.substr(space + 1));

		if (!tail.empty() && tail.find_first_not_of("0123456789") == std::string::npos)
		{
			count = std::atoi(tail.c_str());

			text = Trim(text.substr(0, space));
		}
	}
	else if (!text.empty() && text.find_first_not_of("0123456789") == std::string::npos)
	{
		count = std::atoi(text.c_str());

		text.clear();
	}

	if (count < 1) { count = 1; }
	if (count > 5) { count = 5; }

	int gang = text.empty() ? std::rand() % GANG_SPAWN_COUNT : -1;

	for (int i = 0; i < GANG_SPAWN_COUNT && gang < 0; i++)
	{
		if (text == GANG_SPAWNS[i].key) { gang = i; }
	}

	if (gang < 0)
	{
		Log("gangattack: unknown gang '" + text + "'  (walton bandito rebel rustler raider miner criminal lawmen zombie)");

		return;
	}

	const GangSpawn& info = GANG_SPAWNS[gang];

	Actor player = ACTOR::GET_PLAYER_ACTOR(-1);

	Vector3 position = ACTOR::GET_POSITION(player);

	Layout layout = OBJECT::FIND_NAMED_LAYOUT("PlayerLayout");

	for (int n = 0; n < count; n++)
	{
		ActorModel model = info.models[n % 4];

		STREAM::STREAMING_REQUEST_ACTOR(model, true, true);

		ULONGLONG start = GetTickCount64();

		while (!STREAM::STREAMING_IS_ACTOR_LOADED(model, 0) && GetTickCount64() < start + 3000)
		{
			ScriptWait(0);
		}

		if (!STREAM::STREAMING_IS_ACTOR_LOADED(model, 0))
		{
			Log(std::string("gangattack ") + info.key + ": model " + std::to_string((int)model) + " did not load");

			continue;
		}

		float angle = (float)(std::rand() % 628) / 100.0f;
		float distance = g_gangDistanceBase + (float)(std::rand() % 5);

		float x = position.x + std::cos(angle) * distance;
		float y = position.y + std::sin(angle) * distance;

		std::string objectName = "ap_trap_gang_" + std::to_string(g_spawnCounter++);

		Actor actor = OBJECT::CREATE_ACTOR_IN_LAYOUT(layout, objectName.c_str(), model, Vector2(x, y), position.z + 1.0f, Vector2(), 0.0f);

		if ((int)actor <= 0)
		{
			Log(std::string("gangattack ") + info.key + ": spawn FAILED, id " + std::to_string((int)actor));

			continue;
		}

		g_spawned.push_back({ actor, GetTickCount64() });

		if (info.faction >= 0)
		{
			FACTION::SET_ACTOR_FACTION(actor, info.faction);
		}

		INVENTORY::GIVE_WEAPON_TO_ACTOR(actor, WEAPON_REVOLVER_Cattleman, 1, false, false);
		INVENTORY::ACTOR_PUT_WEAPON_IN_HAND(actor, WEAPON_REVOLVER_Cattleman, true);

		AI_MEMORY::MEMORY_CONSIDER_AS_ENEMY(actor, player);
		AI_MEMORY::MEMORY_ATTACK_ON_SIGHT(actor, player);
		TASKS::TASK_KILL_CHAR(actor, player);

		Log(std::string("gangattack ") + info.key + ": spawned model " + std::to_string((int)model) + " actor " + std::to_string((int)actor)
			+ " faction now " + std::to_string(FACTION::GET_ACTOR_FACTION(actor)) + " at " + std::to_string((int)distance) + " m");
	}

	if (g_gangDistanceBase > 20.0f) { ApBig("Lawmen Posse", "The law is coming for you!"); }
	else { ApBig("Ambush!", std::string(info.key) == "lawmen" ? "Lawmen are attacking!" : "A gang is attacking!"); }
}

// "spawn" = random animal, next recipe.  "spawn wolf" = that animal.  "spawn wolf 2" = that animal and recipe 2.
static void SpawnCommand(const std::string& _Args)
{
	if (_Args.empty())
	{
		SpawnAnimal(PickRandomAnimal(), -1);

		return;
	}

	std::string name = Lower(_Args);
	int recipe = -1;

	size_t space = name.find_last_of(' ');

	if (space != std::string::npos)
	{
		std::string tail = Trim(name.substr(space + 1));

		if (!tail.empty() && tail.find_first_not_of("0123456789") == std::string::npos)
		{
			recipe = std::atoi(tail.c_str()) % ANIMAL_RECIPES;

			name = Trim(name.substr(0, space));
		}
	}

	for (int i = 0; i < ANIMAL_COUNT; i++)
	{
		if (name == ANIMALS[i].key)
		{
			SpawnAnimal(i, recipe);

			return;
		}
	}

	Log("spawn command: unknown animal '" + name + "'");

	Say("Unknown animal: " + name);
}

static void TrackAnimal(ULONGLONG _Now)
{
	if (g_trackedAnimal < 0 || _Now < g_trackNext)
	{
		return;
	}

	g_trackNext = _Now + 1000;

	if (_Now > g_trackStart + 25000)
	{
		Log("animal track: end (" + g_trackedLabel + " recipe " + std::to_string(g_trackedRecipe) + ")");

		g_trackedAnimal = -1;

		return;
	}

	if (!ENTITY::IS_ACTOR_VALID(g_trackedAnimal))
	{
		Log("animal track: actor no longer valid (" + g_trackedLabel + " recipe " + std::to_string(g_trackedRecipe) + ")");

		g_trackedAnimal = -1;

		return;
	}

	Actor player = ACTOR::GET_PLAYER_ACTOR(-1);

	float distance = Distance(ACTOR::GET_POSITION(player), ACTOR::GET_POSITION(g_trackedAnimal));

	Log("animal track: " + g_trackedLabel + " recipe=" + std::to_string(g_trackedRecipe) + " t=" + std::to_string((_Now - g_trackStart) / 1000) + "s dist=" + std::to_string(distance) + " engaged_in_combat=" + std::to_string(AI_MISC::IS_AI_ACTOR_ENGAGED_IN_COMBAT(g_trackedAnimal)) + " player_health=" + std::to_string(HEALTH::GET_ACTOR_HEALTH(player)) + " animal_health=" + std::to_string(HEALTH::GET_ACTOR_HEALTH(g_trackedAnimal)));
}



// ---------------------------------------------------------------------------------------------
// Bounty probes. There is NO known getter, so the result has to be read off the game itself:
// pause menu (bounty / notoriety), wanted poster, how lawmen and townspeople react.
// ---------------------------------------------------------------------------------------------

static void BountyCommand(const std::string& _Kind, const std::string& _Value)
{
	float value = (float)std::atof(_Value.c_str());

	if (_Kind == "notoriety")
	{
		ACTOR::SET_PLAYER_CURRENT_NOTORIETY(value);

		Log("bounty probe: SET_PLAYER_CURRENT_NOTORIETY(" + std::to_string(value) + ")");

		Say("Notoriety set to " + std::to_string(value) + ".\nCheck the pause menu / lawmen.");
	}
	else if (_Kind == "wantedfx")
	{
		if (_Value == "off")
		{
			HUD::WANTEDFX_SET_LEVEL(0.0f);
			HUD::WANTEDFX_ENABLED(false);

			Log("bounty probe: wantedfx off");

			Say("Wanted effect off");
		}
		else
		{
			HUD::WANTEDFX_ENABLED(true);
			HUD::WANTEDFX_SET_LEVEL(value);

			Log("bounty probe: WANTEDFX_ENABLED(true) WANTEDFX_SET_LEVEL(" + std::to_string(value) + ")");

			Say("Wanted effect level " + std::to_string(value));
		}
	}
	else if (_Kind == "paybounty")
	{
		int result = PC::SET_PAYING_BOUNTY((int)value);

		Log("bounty probe: SET_PAYING_BOUNTY(" + std::to_string((int)value) + ") returned " + std::to_string(result));

		Say("SET_PAYING_BOUNTY(" + std::to_string((int)value) + ") -> " + std::to_string(result));
	}
}



// ---------------------------------------------------------------------------------------------
// Kill / hit watcher
// ---------------------------------------------------------------------------------------------

static Actor g_actorBuffer[MAX_ACTORS];
static double g_scanMaxMs = 0.0;
static double g_scanTotalMs = 0.0;
static int g_scanCount = 0;
static int g_lastActorCount = 0;

struct ActorMemo
{
	bool dead = false;
	int lastHitTime = 0;
	bool hasHat = false;
	bool hatEquipped = false;
};

static std::map<int, ActorMemo> g_actorMemo;

// v5.14 hat trick rule (set by the user): the player fired recently + the hat came off + the actor is NOT dead = success.
// An actor that is dead (at that moment, or still dead a moment later) = failure.
static ULONGLONG g_lastShotMs = 0;
static const ULONGLONG HAT_FIRED_WINDOW_MS = 1500;
static const ULONGLONG HAT_CONFIRM_MS = 1500;

struct HatCandidate
{
	int actor;
	ULONGLONG due;
};

static std::vector<HatCandidate> g_hatCandidates;

// v5.14: drunk trap. "drunk <seconds>" makes John drunk for that long ("drunk 0" sobers him up). The state is re-applied
// once a second while it lasts, in case the game clears it, and ended when the time is up.
static ULONGLONG g_drunkUntil = 0;
static bool g_mgProbe = false;   // v5.20k: "mgprobe 1|0"

// v5.14: shove trap test. "shove [explosion name] [distance] [size]" creates a small explosion <distance> metres from John
// in a random direction (defaults: Dynamite, 3, 2) and watches for 4 seconds whether he went ragdoll and how much health
// it cost. The explosion names the game accepts are unknown, so try a few; a wrong name probably does nothing.
static ULONGLONG g_shoveUntil = 0;
static float g_shoveHealth0 = 0.0f;
static bool g_shoveRagdoll = false;
static bool g_shoveSafe = false;
static bool g_firstActorScan = true;


// v5.18: gang kill counter. Faction ids come from the community eFaction enum (index = id). Kills made under faction 19
// (PlayerEnemy = scripted mission enemies) are deliberately not counted. Counts survive until the game restarts.
struct GangInfo
{
	int faction;
	const char* key;
	const char* name;
};

static const GangInfo GANGS[] =
{
	{ 25, "walton",   "Walton's Gang" },
	{ 12, "bandito",  "Banditos" },
	{ 24, "treasure", "Treasure Hunters" },
	{ 27, "rebel",    "Reyes' Rebels" },
	{ 26, "smuggler", "Smugglers" },
	{ 14, "rustler",  "Cattle Rustlers" },
	{ 15, "raider",   "Indian Raiders" },
	{ 16, "soldier",  "Mexican Soldiers" },
	{ 11, "bounty",   "Bounty Hunters" },
};


// ---------------------------------------------------------------------------------------------
// v5.20m saved progress ("rdr1_ap_progress.txt", lines "key value"). Two jobs:
//   1. gang kill totals used to live in memory only and started from 0 at every launch; they are saved now.
//   2. high-water marks for game counters that a save reload sets back (random encounters = sn_btin). A check for the Nth
//      encounter is only sent when the counter climbs ABOVE the highest value ever seen, so a reload to an older save has to
//      get past the old number again before anything new is sent. The first run (no entry) starts at the current value, so the
//      encounters already in the save do not all fire at once. "progress" logs it, "progress reset" starts over.
// ---------------------------------------------------------------------------------------------
#define PROGRESS_PATH (ApFile("rdr1_ap_progress.txt").c_str())
static std::map<std::string, long long> g_progress;
static bool g_progressLoaded = false;

static void ProgressLoad()
{
	if (g_progressLoaded) { return; }

	g_progressLoaded = true;

	std::ifstream in(PROGRESS_PATH);
	std::string key;
	long long value;

	while (in >> key >> value) { g_progress[key] = value; }
}

static void ProgressSave()
{
	std::ofstream out(PROGRESS_PATH, std::ios::trunc);

	for (const auto& kv : g_progress) { out << kv.first << " " << kv.second << "\n"; }
}

static void ProgressSet(const std::string& _Key, long long _Value)
{
	ProgressLoad();

	g_progress[_Key] = _Value;

	ProgressSave();
}

static bool ProgressHas(const std::string& _Key)
{
	ProgressLoad();

	return g_progress.count(_Key) != 0;
}

static long long ProgressGet(const std::string& _Key)
{
	ProgressLoad();

	auto it = g_progress.find(_Key);

	return it == g_progress.end() ? 0 : it->second;
}

static const int GANG_COUNT = (int)(sizeof(GANGS) / sizeof(GANGS[0]));
static int g_gangKills[GANG_COUNT];
static bool g_gangKillsLoaded = false;

static void GangKillsLoad()
{
	if (g_gangKillsLoaded) { return; }

	g_gangKillsLoaded = true;

	for (int i = 0; i < GANG_COUNT; i++) { g_gangKills[i] = (int)ProgressGet(std::string("gang:") + GANGS[i].name); }
}
static std::set<int> g_gangKilledActors;

static void SendCheck(const std::string& _Location);

// Beta: hideout and gang camp outlaws are spawned by scripts and carry faction 19, like story mission enemies, so the faction
// alone missed every kill at Twin Rocks. Outside story missions a faction-19 kill is credited to the gang its MODEL belongs to.
struct GangModel { ActorModel model; const char* gang; };

static const GangModel GANG_MODELS[] =
{
	{ ACTOR_MEXICAN_ARMY_Easy01, "Mexican Soldiers" },
	{ ACTOR_MEXICAN_ARMY_Easy02, "Mexican Soldiers" },
	{ ACTOR_MEXICAN_ARMY_Easy03, "Mexican Soldiers" },
	{ ACTOR_MEXICAN_ARMY_Easy04, "Mexican Soldiers" },
	{ ACTOR_MEXICAN_ARMY_Easy05, "Mexican Soldiers" },
	{ ACTOR_MEXICAN_ARMY_Easy06, "Mexican Soldiers" },
	{ ACTOR_MEXICAN_ARMY_Easy07, "Mexican Soldiers" },
	{ ACTOR_MEXICAN_ARMY_Easy08, "Mexican Soldiers" },
	{ ACTOR_MEXICAN_ARMY_Easy09, "Mexican Soldiers" },
	{ ACTOR_MEXICAN_ARMY_Easy10, "Mexican Soldiers" },
	{ ACTOR_MEXICAN_ARMY_Easy11, "Mexican Soldiers" },
	{ ACTOR_MEXICAN_ARMY_Easy12, "Mexican Soldiers" },
	{ ACTOR_MEXICAN_ARMY_Medium01, "Mexican Soldiers" },
	{ ACTOR_MEXICAN_ARMY_Medium02, "Mexican Soldiers" },
	{ ACTOR_MEXICAN_ARMY_Medium03, "Mexican Soldiers" },
	{ ACTOR_MEXICAN_ARMY_Hard01, "Mexican Soldiers" },
	{ ACTOR_MEXICAN_ARMY_Hard02, "Mexican Soldiers" },
	{ ACTOR_MEXICAN_ARMY_Hard03, "Mexican Soldiers" },
	{ ACTOR_GANG_INDIANRAIDER_Easy01, "Indian Raiders" },
	{ ACTOR_GANG_INDIANRAIDER_Easy02, "Indian Raiders" },
	{ ACTOR_GANG_INDIANRAIDER_Easy03, "Indian Raiders" },
	{ ACTOR_GANG_INDIANRAIDER_Medium01, "Indian Raiders" },
	{ ACTOR_GANG_INDIANRAIDER_Medium02, "Indian Raiders" },
	{ ACTOR_GANG_INDIANRAIDER_Medium03, "Indian Raiders" },
	{ ACTOR_GANG_INDIANRAIDER_Medium04, "Indian Raiders" },
	{ ACTOR_GANG_INDIANRAIDER_Hard01, "Indian Raiders" },
	{ ACTOR_GANG_INDIANRAIDER_Hard02, "Indian Raiders" },
	{ ACTOR_GANG_INDIANRAIDER_Hard03, "Indian Raiders" },
	{ ACTOR_GANG_CATTLERUSTLER_Easy01, "Cattle Rustlers" },
	{ ACTOR_GANG_CATTLERUSTLER_Easy02, "Cattle Rustlers" },
	{ ACTOR_GANG_CATTLERUSTLER_Easy03, "Cattle Rustlers" },
	{ ACTOR_GANG_CATTLERUSTLER_Medium01, "Cattle Rustlers" },
	{ ACTOR_GANG_CATTLERUSTLER_Medium02, "Cattle Rustlers" },
	{ ACTOR_GANG_CATTLERUSTLER_Medium03, "Cattle Rustlers" },
	{ ACTOR_GANG_CATTLERUSTLER_Hard01, "Cattle Rustlers" },
	{ ACTOR_GANG_CATTLERUSTLER_Hard02, "Cattle Rustlers" },
	{ ACTOR_GANG_CATTLERUSTLER_Hard03, "Cattle Rustlers" },
	{ ACTOR_GANG_DRUNKNDIRTY_Easy01, "Walton's Gang" },
	{ ACTOR_GANG_DRUNKNDIRTY_Easy02, "Walton's Gang" },
	{ ACTOR_GANG_DRUNKNDIRTY_Easy03, "Walton's Gang" },
	{ ACTOR_GANG_DRUNKNDIRTY_Medium01, "Walton's Gang" },
	{ ACTOR_GANG_DRUNKNDIRTY_Medium02, "Walton's Gang" },
	{ ACTOR_GANG_DRUNKNDIRTY_Medium03, "Walton's Gang" },
	{ ACTOR_GANG_DRUNKNDIRTY_Hard01, "Walton's Gang" },
	{ ACTOR_GANG_DRUNKNDIRTY_Hard02, "Walton's Gang" },
	{ ACTOR_GANG_DRUNKNDIRTY_Hard03, "Walton's Gang" },
	{ ACTOR_GANG_DRUNKNDIRTY_Hard03_W, "Walton's Gang" },
	{ ACTOR_GANG_BANDITO_Easy01, "Banditos" },
	{ ACTOR_GANG_BANDITO_Easy02, "Banditos" },
	{ ACTOR_GANG_BANDITO_Easy03, "Banditos" },
	{ ACTOR_GANG_BANDITO_Medium01, "Banditos" },
	{ ACTOR_GANG_BANDITO_Medium02, "Banditos" },
	{ ACTOR_GANG_BANDITO_Medium03, "Banditos" },
	{ ACTOR_GANG_BANDITO_Hard01, "Banditos" },
	{ ACTOR_GANG_BANDITO_Hard02, "Banditos" },
	{ ACTOR_GANG_BANDITO_Hard03, "Banditos" },
	{ ACTOR_GANG_CRAZYMINER_Easy01, "Treasure Hunters" },
	{ ACTOR_GANG_CRAZYMINER_Easy02, "Treasure Hunters" },
	{ ACTOR_GANG_CRAZYMINER_Easy03, "Treasure Hunters" },
	{ ACTOR_GANG_CRAZYMINER_Medium01, "Treasure Hunters" },
	{ ACTOR_GANG_CRAZYMINER_Medium02, "Treasure Hunters" },
	{ ACTOR_GANG_CRAZYMINER_Medium03, "Treasure Hunters" },
	{ ACTOR_GANG_CRAZYMINER_Medium04, "Treasure Hunters" },
	{ ACTOR_GANG_CRAZYMINER_Medium05, "Treasure Hunters" },
	{ ACTOR_GANG_CRAZYMINER_Hard01, "Treasure Hunters" },
	{ ACTOR_GANG_CRAZYMINER_Hard02, "Treasure Hunters" },
	{ ACTOR_GANG_CRAZYMINER_Hard03, "Treasure Hunters" },
	{ ACTOR_GANG_MEXICANREBEL_Easy01, "Reyes' Rebels" },
	{ ACTOR_GANG_MEXICANREBEL_Easy02, "Reyes' Rebels" },
	{ ACTOR_GANG_MEXICANREBEL_Easy03, "Reyes' Rebels" },
	{ ACTOR_GANG_MEXICANREBEL_Easy04, "Reyes' Rebels" },
	{ ACTOR_GANG_MEXICANREBEL_Easy05, "Reyes' Rebels" },
	{ ACTOR_GANG_MEXICANREBEL_Medium01, "Reyes' Rebels" },
	{ ACTOR_GANG_MEXICANREBEL_Medium02, "Reyes' Rebels" },
	{ ACTOR_GANG_MEXICANREBEL_Medium03, "Reyes' Rebels" },
	{ ACTOR_GANG_MEXICANREBEL_Medium04, "Reyes' Rebels" },
	{ ACTOR_GANG_MEXICANREBEL_Medium05, "Reyes' Rebels" },
	{ ACTOR_GANG_MEXICANREBEL_Medium06, "Reyes' Rebels" },
	{ ACTOR_GANG_MEXICANREBEL_Medium07, "Reyes' Rebels" },
	{ ACTOR_GANG_MEXICANREBEL_Medium08, "Reyes' Rebels" },
	{ ACTOR_GANG_MEXICANREBEL_Hard01, "Reyes' Rebels" },
	{ ACTOR_GANG_MEXICANREBEL_Hard02, "Reyes' Rebels" },
	{ ACTOR_GANG_MEXICANREBEL_Hard03, "Reyes' Rebels" },
	{ ACTOR_GANG_MEXICANREBEL_Hard04, "Reyes' Rebels" },

};

static void CountGangKill(Actor _Actor, bool _ByPlayer)
{
	if (!_ByPlayer || g_gangKilledActors.count((int)_Actor))
	{
		return;
	}

	if (g_gangKilledActors.size() > 2000)
	{
		g_gangKilledActors.clear();   // v5.20k: actor ids are recycled, so the set is bounded
	}

	int faction = FACTION::GET_ACTOR_FACTION(_Actor);

	if (faction == 19 && g_lastMissionScript.empty())
	{
		ActorModel model = OBJECT::GET_ACTOR_ENUM(_Actor);

		for (const GangModel& gm : GANG_MODELS)
		{
			if (gm.model != model) { continue; }

			for (int i = 0; i < GANG_COUNT; i++)
			{
				if (std::string(GANGS[i].name) == gm.gang) { faction = GANGS[i].faction; Log(std::string("gang kill by model: ") + gm.gang); }
			}
		}
	}

	for (int i = 0; i < GANG_COUNT; i++)
	{
		if (GANGS[i].faction == faction)
		{
			g_gangKilledActors.insert((int)_Actor);

			GangKillsLoad();

			g_gangKills[i]++;

			ProgressSet(std::string("gang:") + GANGS[i].name, g_gangKills[i]);

			Log(std::string("GANG KILL: ") + GANGS[i].name + " (faction " + std::to_string(faction) + ") total=" + std::to_string(g_gangKills[i]));

			// v5.23: small progress line (the milestone kill shows the check instead)
			if (g_gangKills[i] % 5 != 0 && g_gangKills[i] < 50)
			{
				ApSay(std::string(GANGS[i].name) + ": " + std::to_string(g_gangKills[i] % 5) + "/5");
			}

			// v5.20k: a check every 5 kills, up to 50 (location "<Gang>: <n> Kills").
			if (g_gangKills[i] % 5 == 0 && g_gangKills[i] <= 50)
			{
				SendCheck(std::string(GANGS[i].name) + ": " + std::to_string(g_gangKills[i]) + " Kills");
			}

			return;
		}
	}

	if (faction == 19)
	{
		Log("gang kill ignored: faction 19 (scripted mission enemy)");
	}
}

static std::string DescribeActor(Actor _Actor, Actor _Player)
{
	float distance = Distance(ACTOR::GET_POSITION(_Player), ACTOR::GET_POSITION(_Actor));

	int zone = -1;

	bool zoneOk = HEALTH::GET_LAST_HIT_ZONE(_Actor, &zone);

	return "actor=" + std::to_string((int)_Actor)
		+ " faction=" + std::to_string(FACTION::GET_ACTOR_FACTION(_Actor))
		+ " species=" + std::to_string(AI_ANIMAL::ANIMAL_ACTOR_GET_SPECIES(_Actor))
		+ " weapon=" + std::to_string((int)HEALTH::GET_LAST_HIT_WEAPON(_Actor))
		+ " zone=" + (zoneOk ? std::to_string(zone) : std::string("none"))
		+ " flags=" + std::to_string(HEALTH::GET_LAST_HIT_FLAGS(_Actor))
		+ " damage=" + std::to_string(HEALTH::GET_LAST_DAMAGE(_Actor))
		+ " health=" + std::to_string(HEALTH::GET_ACTOR_HEALTH(_Actor))
		+ " dist=" + std::to_string(distance);
}

static void ScanActors()
{
	Actor player = ACTOR::GET_PLAYER_ACTOR(-1);

	int count = REDHOOK::WORLD_GET_ALL_ACTORS(g_actorBuffer);

	if (count < 0)
	{
		count = 0;
	}

	if (count > MAX_ACTORS)
	{
		Log("actor scan: count " + std::to_string(count) + " exceeds buffer " + std::to_string(MAX_ACTORS));

		count = MAX_ACTORS;
	}

	g_lastActorCount = count;

	if (g_firstActorScan)
	{
		Log("actor scan: first scan found " + std::to_string(count) + " actors");

		g_firstActorScan = false;
	}

	// Did something hurt the player? Logs who (this is the real "did the animal attack me" signal).
	{
		static int playerHitTime = -1;

		int hitTime = HEALTH::GET_LAST_HIT_TIME(player);

		if (playerHitTime != -1 && hitTime != playerHitTime)
		{
			Actor attacker = HEALTH::GET_LAST_ATTACKER(player);

			Log("player hit: health=" + std::to_string(HEALTH::GET_ACTOR_HEALTH(player))
				+ (((int)attacker > 0 && ENTITY::IS_ACTOR_VALID(attacker)) ? " by " + DescribeActor(attacker, player) : std::string(" attacker unknown")));
		}

		playerHitTime = hitTime;
	}

	std::map<int, ActorMemo> next;

	for (int i = 0; i < count; i++)
	{
		Actor actor = g_actorBuffer[i];

		if (actor == player || !ENTITY::IS_ACTOR_VALID(actor))
		{
			continue;
		}

		bool dead = HEALTH::IS_ACTOR_DEAD(actor);
		int hitTime = HEALTH::GET_LAST_HIT_TIME(actor);

		// Hat trick candidates: does this actor still own / wear a hat?
		bool hasHat = INVENTORY::HAS_ACCESSORY_ENUM(actor, ACCESSORY_HAT);
		bool hatEquipped = INVENTORY::IS_ACCESSORY_EQUIPPPED(actor, ACCESSORY_HAT);

		ActorMemo memo;

		auto it = g_actorMemo.find(actor);

		if (it == g_actorMemo.end())
		{
			// First time we see this actor: remember its state silently.
			memo.dead = dead;
			memo.lastHitTime = hitTime;
			memo.hasHat = hasHat;
			memo.hatEquipped = hatEquipped;
		}
		else
		{
			memo = it->second;

			if (hitTime != memo.lastHitTime)
			{
				memo.lastHitTime = hitTime;

				if (HEALTH::GET_LAST_ATTACKER(actor) == player)
				{
					Log("hit by player: " + DescribeActor(actor, player));
				}
			}

			if (dead && !memo.dead)
			{
				bool byPlayer = HEALTH::GET_LAST_ATTACKER(actor) == player;

				Log(std::string("kill: by_player=") + (byPlayer ? "1" : "0") + " " + DescribeActor(actor, player));

				CountGangKill(actor, byPlayer);
			}

			// Anything that changes about the hat gets logged, with who hit this actor last, so a shot-off hat
			// shows up here even if it does not count as a normal hit.
			if (hasHat != memo.hasHat || hatEquipped != memo.hatEquipped)
			{
				bool byPlayer = HEALTH::GET_LAST_ATTACKER(actor) == player;

				Log("hat change: actor=" + std::to_string((int)actor)
					+ " has_hat " + std::to_string(memo.hasHat) + "->" + std::to_string(hasHat)
					+ " equipped " + std::to_string(memo.hatEquipped) + "->" + std::to_string(hatEquipped)
					+ " dead=" + std::to_string(dead)
					+ " last_attacker_is_player=" + std::to_string(byPlayer)
					+ " " + DescribeActor(actor, player));

				if (memo.hasHat && !hasHat)
				{
					ULONGLONG nowMs = GetTickCount64();
					bool firedRecently = g_lastShotMs != 0 && nowMs - g_lastShotMs <= HAT_FIRED_WINDOW_MS;

					if (!firedRecently)
					{
						Log("HAT TRICK ignored: actor=" + std::to_string((int)actor) + " lost its hat but the player had not fired in the last "
							+ std::to_string(HAT_FIRED_WINDOW_MS) + " ms");
					}
					else if (dead)
					{
						Log("HAT TRICK failed: actor=" + std::to_string((int)actor) + " is dead");
					}
					else
					{
						Log("HAT TRICK candidate: actor=" + std::to_string((int)actor) + " player fired " + std::to_string(nowMs - g_lastShotMs)
							+ " ms ago, hat off, actor alive (confirming in " + std::to_string(HAT_CONFIRM_MS) + " ms)");

						g_hatCandidates.push_back({ (int)actor, nowMs + HAT_CONFIRM_MS });
					}
				}

				memo.hasHat = hasHat;
				memo.hatEquipped = hatEquipped;
			}

			memo.dead = dead;
		}

		next[actor] = memo;
	}

	g_actorMemo.swap(next);

	for (size_t i = 0; i < g_hatCandidates.size();)
	{
		if (GetTickCount64() >= g_hatCandidates[i].due)
		{
			Actor actor = (Actor)g_hatCandidates[i].actor;

			if (!ENTITY::IS_ACTOR_VALID(actor))
			{
				Log("HAT TRICK failed: actor=" + std::to_string(g_hatCandidates[i].actor) + " no longer exists");
			}
			else if (HEALTH::IS_ACTOR_DEAD(actor))
			{
				Log("HAT TRICK failed: actor=" + std::to_string(g_hatCandidates[i].actor) + " died right after");
			}
			else
			{
				Log("HAT TRICK SUCCESS: actor=" + std::to_string(g_hatCandidates[i].actor));

				// hat tricks are checks "Hat Trick 1".."Hat Trick 15" (count saved in the progress file; beta 2: 15, was 5).
				long long tricks = ProgressGet("hattricks") + 1;

				ProgressSet("hattricks", tricks);

				if (tricks <= 15) { SendCheck("Hat Trick " + std::to_string(tricks)); }
				else { ApSay("Hat trick! (" + std::to_string(tricks) + ")"); }
			}

			g_hatCandidates.erase(g_hatCandidates.begin() + i);
		}
		else
		{
			i++;
		}
	}
}



// ---------------------------------------------------------------------------------------------
// Stat snapshot / diff (find which stat ids hold cash, kills, hat tricks, missions...)
// ---------------------------------------------------------------------------------------------

static bool g_hasSnapshot = false;
static std::vector<int> g_prevInts;
static std::vector<float> g_prevFloats;
static std::vector<bool> g_noisy;

static bool IntChanged(int _A, int _B)
{
	return _A != _B;
}

static bool FloatChanged(float _A, float _B)
{
	return std::isfinite(_A) && std::isfinite(_B) && std::fabs(_A - _B) > 0.0001f;
}

static void ReadStats(std::vector<int>& _Ints, std::vector<float>& _Floats, bool _Verbose)
{
	_Ints.assign(g_statScanMax, 0);
	_Floats.assign(g_statScanMax, 0.0f);

	for (int i = 0; i < g_statScanMax; i++)
	{
		if (_Verbose && i % 32 == 0)
		{
			Log("stat scan: reading id " + std::to_string(i));
		}

		_Ints[i] = STAT::GET_SAGPLAYER_STAT_INT(i);
		_Floats[i] = STAT::GET_SAGPLAYER_STAT_FLOAT(i);
	}
}

static void StatSnapshot()
{
	if (!g_hasSnapshot)
	{
		Say("Calibrating stats, stand still for 2 seconds...");

		std::vector<int> ints;
		std::vector<float> floats;

		ReadStats(ints, floats, true);

		g_noisy.assign(g_statScanMax, false);

		for (int sample = 0; sample < 4; sample++)
		{
			ULONGLONG until = GetTickCount64() + 500;

			while (GetTickCount64() < until)
			{
				ScriptWait(0);
			}

			std::vector<int> newInts;
			std::vector<float> newFloats;

			ReadStats(newInts, newFloats, false);

			for (int i = 0; i < g_statScanMax; i++)
			{
				if (IntChanged(newInts[i], ints[i]) || FloatChanged(newFloats[i], floats[i]))
				{
					g_noisy[i] = true;
				}
			}

			ints = newInts;
			floats = newFloats;
		}

		std::string list;
		int count = 0;

		for (int i = 0; i < g_statScanMax; i++)
		{
			if (g_noisy[i])
			{
				list += " " + std::to_string(i);

				count++;
			}
		}

		Log("stat calibration: " + std::to_string(count) + " self-changing ids will be ignored:" + list);

		g_prevInts = ints;
		g_prevFloats = floats;
		g_hasSnapshot = true;

		Log("stat snapshot: baseline taken for ids 0.." + std::to_string(g_statScanMax - 1));

		Say("Stat baseline ready");

		return;
	}

	std::vector<int> ints;
	std::vector<float> floats;

	ReadStats(ints, floats, false);

	int changes = 0;

	for (int i = 0; i < g_statScanMax; i++)
	{
		if (g_noisy[i])
		{
			continue;
		}

		if (IntChanged(ints[i], g_prevInts[i]))
		{
			Log("stat " + std::to_string(i) + " int: " + std::to_string(g_prevInts[i]) + " -> " + std::to_string(ints[i]));

			changes++;
		}

		if (FloatChanged(floats[i], g_prevFloats[i]))
		{
			Log("stat " + std::to_string(i) + " float: " + std::to_string(g_prevFloats[i]) + " -> " + std::to_string(floats[i]));

			changes++;
		}
	}

	Log("stat diff: " + std::to_string(changes) + " changes (self-changing ids ignored)");

	Say("Stat diff logged: " + std::to_string(changes) + " changes");

	g_prevInts = ints;
	g_prevFloats = floats;
}



// Lists every stat whose value looks like a medal: an int of 1..4, or a float that is exactly 1.0 .. 4.0.
// Compare the counts with the medals on the Stats page (how many bronze / silver / gold).
static void StatDump()
{
	std::vector<int> ints;
	std::vector<float> floats;

	ReadStats(ints, floats, false);

	int intCount[5] = {};
	int floatCount[5] = {};
	int hits = 0;

	for (int i = 0; i < g_statScanMax; i++)
	{
		bool intHit = ints[i] >= 1 && ints[i] <= 4;
		bool floatHit = std::isfinite(floats[i]) && floats[i] >= 1.0f && floats[i] <= 4.0f && floats[i] == std::floor(floats[i]);

		if (intHit || floatHit)
		{
			Log("statdump: id=" + std::to_string(i) + " int=" + std::to_string(ints[i]) + " float=" + std::to_string(floats[i]));

			hits++;
		}

		if (intHit)
		{
			intCount[ints[i]]++;
		}

		if (floatHit)
		{
			floatCount[(int)floats[i]]++;
		}
	}

	Log("statdump: scanned " + std::to_string(g_statScanMax) + " ids, " + std::to_string(hits) + " hits. int values 1/2/3/4 = "
		+ std::to_string(intCount[1]) + "/" + std::to_string(intCount[2]) + "/" + std::to_string(intCount[3]) + "/" + std::to_string(intCount[4])
		+ ", float values 1/2/3/4 = "
		+ std::to_string(floatCount[1]) + "/" + std::to_string(floatCount[2]) + "/" + std::to_string(floatCount[3]) + "/" + std::to_string(floatCount[4]));

	Say("Stat dump written to the log");
}



// Dead Eye probes. "deadeye multilock 0" etc. Each call logs the points / active state afterwards.
static void DeadEyeCommand(const std::string& _Args)
{
	Actor player = ACTOR::GET_PLAYER_ACTOR(-1);

	std::string sub = _Args;
	std::string value;

	size_t space = _Args.find(' ');

	if (space != std::string::npos)
	{
		sub = Trim(_Args.substr(0, space));
		value = Trim(_Args.substr(space + 1));
	}

	float number = (float)std::atof(value.c_str());
	bool flag = number != 0.0f;

	if (sub == "multilock") { ACTOR::SET_DEADEYE_MULTILOCK_ENABLE(player, flag); }
	else if (sub == "paint") { ACTOR::SET_DEADEYE_TARGETPAINT_ENABLE(player, flag); }
	else if (sub == "head") { ACTOR::SET_DEADEYE_LOCKS_ON_HEAD_ONLY(player, flag); }
	else if (sub == "disable") { ACTOR::SET_DISABLE_DEADEYE(player, flag); }
	else if (sub == "infinite") { ACTOR::SET_INFINITE_DEADEYE(player, flag); }
	else if (sub == "locks") { ACTOR::SET_ALLOW_DEADEYE_LOCKS(player, flag ? 1 : 0); }
	else if (sub == "points") { ACTOR::SET_PLAYER_DEADEYE_POINTS(player, number, true); }
	else if (sub == "maxpoints") { ACTOR::SET_MAX_DEADEYE_POINTS(player, (int)number); }
	else if (sub == "regen") { ACTOR::SET_DEADEYE_REGENERATION_RATE(player, number, false); }
	else if (sub == "timescale") { ACTOR::SET_DEADEYE_TIMESCALE(player, number); }
	else if (sub != "status")
	{
		Log("deadeye: unknown sub-command '" + sub + "'");

		Say("Unknown deadeye option: " + sub);

		return;
	}

	Log("deadeye: " + sub + " " + value + " -> points=" + std::to_string(ACTOR::GET_PLAYER_DEADEYE_POINTS(player))
		+ " active=" + std::to_string(ACTOR::IS_PLAYER_DEADEYE(player)));

	Say("deadeye " + sub + " " + value);
}



// Finds stats equal to a number you can see on screen (for example your cash). Tries the number as an int and a float,
// and the same number x100 (in case money is stored in cents).
static void StatFind(double _Target)
{
	std::vector<int> ints;
	std::vector<float> floats;

	ReadStats(ints, floats, false);

	const double targets[2] = { _Target, _Target * 100.0 };

	int hits = 0;

	for (int i = 0; i < g_statScanMax; i++)
	{
		for (double target : targets)
		{
			bool intHit = (double)ints[i] == target;
			bool floatHit = std::isfinite(floats[i]) && std::fabs((double)floats[i] - target) < 0.01;

			if (intHit || floatHit)
			{
				Log("statfind: id=" + std::to_string(i) + " matches " + std::to_string(target) + " int=" + std::to_string(ints[i]) + " float=" + std::to_string(floats[i]));

				hits++;
			}
		}
	}

	Log("statfind: " + std::to_string(hits) + " hits for " + std::to_string(_Target) + " (also tried x100)");

	Say("statfind: " + std::to_string(hits) + " hits");
}



// ---------------------------------------------------------------------------------------------
// Mission recorder / detector prototype (objective-hash approach)
//
// rdr1_ap_objectives.csv  every objective-hash change, every game popup (stat message) and every label you set.
//                         Lines look like: time_ms,event,value,x,y,z,label
// rdr1_ap_missions.txt    OPTIONAL table, one line per mission:  Mission Name=<final objective hash>
//                         When that hash shows up and a game popup follows within 3 minutes, the log gets
//                         "MISSION COMPLETE (prototype): <name>".
// Input-file commands:    mission <name>        label what you are about to do (written to the CSV)
//                         missionend passed|failed   close the label
//                         loadmissions          re-read rdr1_ap_missions.txt
// ---------------------------------------------------------------------------------------------

static const char* OBJECTIVES_PATH = "rdr1_ap_objectives.csv";
static const char* MISSIONS_PATH = "rdr1_ap_missions.txt";

static std::string g_missionLabel;
static std::map<long long, std::string> g_missionTable;
static long long g_prevObjectiveHash = 0;
static bool g_objectiveHashKnown = false;
static int g_prevPopup = 0;
static ULONGLONG g_finalSeenAt = 0;
static int g_freeRoamHash = 0;                       // v5.20m: journal objective hash left behind by the last completed mission = free roam
static std::map<int, ULONGLONG> g_strayDue;          // v5.20m: stray weapon id -> tick when a free roam pickup is taken away (5 s)
static std::string g_finalSeenName;

static void CsvLine(const std::string& _Event, const std::string& _Value)
{
	Vector3 p = ACTOR::GET_POSITION(ACTOR::GET_PLAYER_ACTOR(-1));

	std::ofstream file(OBJECTIVES_PATH, std::ios::app);

	file << GetTickCount64() << "," << _Event << "," << _Value << "," << p.x << "," << p.y << "," << p.z << "," << g_missionLabel << "\n";
}

static void LoadMissionTable()
{
	g_missionTable.clear();

	std::ifstream in(MISSIONS_PATH);

	std::string line;

	while (std::getline(in, line))
	{
		size_t eq = line.find_last_of('=');

		if (eq == std::string::npos)
		{
			continue;
		}

		std::string name = Trim(line.substr(0, eq));
		std::string hash = Trim(line.substr(eq + 1));

		if (!name.empty() && !hash.empty())
		{
			g_missionTable[std::atoll(hash.c_str())] = name;
		}
	}

	Log("mission table: " + std::to_string(g_missionTable.size()) + " entries loaded");
}

static void ReclaimWeapons(const char* _Reason);

// v5.17 journal probe: logs what the journal says about the targeted entry (progress, type, flags, detail count).
// Called on every objective change and at every cutscene end, so a journal value that flips at mission completion shows up.
static float BitsToFloat(int _Bits);
static void JournalProbe(const char* _Why)
{
	int entry = JOURNAL::GET_TARGETED_JOURNAL_ENTRY();

	std::string text = std::string("journal probe (") + _Why + "): targeted=" + std::to_string(entry);

	if (entry != 0 && entry != -1)
	{
		text += " progress=" + std::to_string(BitsToFloat(JOURNAL::GET_JOURNAL_ENTRY_PROGRESS(entry)))
			+ " type=" + std::to_string(JOURNAL::GET_JOURNAL_ENTRY_TYPE(entry))
			+ " misc=" + std::to_string(JOURNAL::GET_JOURNAL_ENTRY_MISC_FLAG(entry))
			+ " updated=" + std::to_string(JOURNAL::IS_JOURNAL_ENTRY_UPDATED(entry))
			+ " details=" + std::to_string(JOURNAL::GET_JOURNAL_ENTRY_NUM_DETAILS(entry));
	}

	Log(text);
}

// "journaldump": the community native database says GET_NUM_JOURNAL_ENTRIES_IN_LIST takes an include-hidden flag (not a
// list id) and GET_JOURNAL_ENTRY_IN_LIST takes (index, include-hidden). Progress is a float whose bits come back in an int.
static float BitsToFloat(int _Bits)
{
	float value;

	std::memcpy(&value, &_Bits, sizeof(value));

	return value;
}

static void JournalDump()
{
	for (int hidden = 0; hidden < 2; hidden++)
	{
		int count = JOURNAL::GET_NUM_JOURNAL_ENTRIES_IN_LIST(hidden);

		Log("journaldump: include_hidden=" + std::to_string(hidden) + " entries=" + std::to_string(count));

		for (int i = 0; i < count && i < 400; i++)
		{
			int entry = JOURNAL::GET_JOURNAL_ENTRY_IN_LIST(i, hidden);

			Log("journaldump: hidden=" + std::to_string(hidden) + " index=" + std::to_string(i) + " entry=" + std::to_string(entry)
				+ " type=" + std::to_string(JOURNAL::GET_JOURNAL_ENTRY_TYPE(entry))
				+ " progress=" + std::to_string(BitsToFloat(JOURNAL::GET_JOURNAL_ENTRY_PROGRESS(entry)))
				+ " misc=" + std::to_string(JOURNAL::GET_JOURNAL_ENTRY_MISC_FLAG(entry))
				+ " updated=" + std::to_string(JOURNAL::IS_JOURNAL_ENTRY_UPDATED(entry))
				+ " details=" + std::to_string(JOURNAL::GET_JOURNAL_ENTRY_NUM_DETAILS(entry)));
		}
	}

	Say("Journal dump written to the log");
}

static ULONGLONG g_cutEndAt = 0;
static long long g_cutEndHash = 0;

static bool g_prevCutscene = false;
static bool g_cutsceneKnown = false;

static void WatchMissionSignals(ULONGLONG _Now)
{
	long long hash = JOURNAL::GET_LAST_NOTE_OBJECTIVE();

	if (!g_objectiveHashKnown)
	{
		g_prevObjectiveHash = hash;
		g_objectiveHashKnown = true;
	}
	else if (hash != g_prevObjectiveHash)
	{
		g_prevObjectiveHash = hash;

		CsvLine("objective", std::to_string(hash));

		JournalProbe("objective");

		auto it = g_missionTable.find(hash);

		if (it != g_missionTable.end())
		{
			g_finalSeenAt = _Now;
			g_finalSeenName = it->second;

			Log("mission table: final objective of '" + it->second + "' seen, waiting for the completion popup");
		}
	}

	bool cutscene = CUTSCENE::CUTSCENE_MANAGER_IS_CUTSCENE_PLAYING() != 0;

	if (!g_cutsceneKnown)
	{
		g_prevCutscene = cutscene;
		g_cutsceneKnown = true;
	}
	else if (cutscene != g_prevCutscene)
	{
		g_prevCutscene = cutscene;

		CsvLine(cutscene ? "cutscene_start" : "cutscene_end", std::to_string(g_prevObjectiveHash));

		if (!cutscene)
		{
			g_cutEndAt = _Now;
			g_cutEndHash = g_prevObjectiveHash;

			JournalProbe("cutscene end");

			// v5.17: the story missions end with a cutscene, not a popup. Final objective seen + cutscene end = complete.
			if (g_finalSeenAt != 0 && _Now < g_finalSeenAt + 180000)
			{
				Log("MISSION COMPLETE (prototype, cutscene): " + g_finalSeenName);

				g_freeRoamHash = (int)JOURNAL::GET_LAST_NOTE_OBJECTIVE();

				Log("legacy completion rule agrees: " + g_finalSeenName);   // v5.21: checks come from MissionTrackerTick now

				ReclaimWeapons("mission complete");


				g_finalSeenAt = 0;
			}
		}
	}

	int popup = HUD::IS_STAT_MESSAGE_SHOWING();

	if (popup != g_prevPopup)
	{
		g_prevPopup = popup;

		CsvLine(popup ? "popup_on" : "popup_off", std::to_string(popup));

		// v5.20k: learning mode. A popup within 30 s of a cutscene end, with the objective hash unchanged since, and no known final
		// objective pending, is probably the completion of a mission this table does not know yet.
		if (popup && g_finalSeenAt == 0 && g_cutEndAt != 0 && _Now < g_cutEndAt + 30000 && g_prevObjectiveHash == g_cutEndHash
			&& g_missionTable.find(g_prevObjectiveHash) == g_missionTable.end())
		{
			Log("MISSION CANDIDATE: final objective hash " + std::to_string(g_prevObjectiveHash)
				+ "  (add  <Mission Name>=" + std::to_string(g_prevObjectiveHash) + "  to rdr1_ap_missions.txt)");

			CsvLine("mission_candidate", std::to_string(g_prevObjectiveHash));


			g_cutEndAt = 0;
		}

		if (popup && g_finalSeenAt != 0)
		{
			if (_Now < g_finalSeenAt + 180000)
			{
				Log("MISSION COMPLETE (prototype): " + g_finalSeenName);

				g_freeRoamHash = (int)JOURNAL::GET_LAST_NOTE_OBJECTIVE();

				Log("legacy completion rule agrees: " + g_finalSeenName);   // v5.21: checks come from MissionTrackerTick now

				ReclaimWeapons("mission complete");

			}

			g_finalSeenAt = 0;
		}
	}
}

static void MissionCommand(const std::string& _Command)
{
	if (_Command == "loadmissions")
	{
		LoadMissionTable();
	}
	else if (_Command.rfind("missionend", 0) == 0)
	{
		std::string result = Trim(_Command.substr(10));

		CsvLine("label_end", result.empty() ? "unspecified" : result);

		Log("mission label ended: " + g_missionLabel + " " + result);

		g_missionLabel.clear();
	}
	else
	{
		g_missionLabel = Trim(_Command.substr(7));

		CsvLine("label_start", g_missionLabel);

		Log("mission label started: " + g_missionLabel);
	}
}



// Will be used by the client to decide when it is safe to hand over items and traps.
static bool SafeToDeliver()
{
	Actor player = ACTOR::GET_PLAYER_ACTOR(-1);

	return GAME::GET_GAME_STATE() == 3 && !HEALTH::IS_ACTOR_DEAD(player) && !CUTSCENE::CUTSCENE_MANAGER_IS_CUTSCENE_PLAYING() && !MINIGAME::IS_MINIGAME_RUNNING();
}



// ---------------------------------------------------------------------------------------------
// Stray weapons: guns that arrive outside Archipelago (mission rewards, pickups) are marked when they appear and taken
// away again after the mission is over, unless the client gave them to the player in the meantime.
//   weaponwatch runs once a second; "reclaim" (input file) forces the removal; the mission-complete prototype
//   calls it automatically. Removal only happens when SafeToDeliver() is true, otherwise it waits.
// ---------------------------------------------------------------------------------------------

static bool IsGun(WeaponModel _Weapon)
{
	int category = (int)WEAPON::GET_WEAPON_CATEGORY_FROM_ENUM(_Weapon);

	return category >= (int)WEAPON_CATEGORY_PISTOL && category <= (int)WEAPON_CATEGORY_SNIPERRIFLE;
}

static void ShopWeaponBought(int _WeaponId);
static void ShopWeaponBought2(int _WeaponId);

static void WeaponWatch()
{
	// v5.14: a baseline taken while the game is still loading (game_state 5) saw 0 guns, and the whole save's inventory
	// then looked "gained outside AP". Only watch in normal play, and start over after every load.
	if (GAME::GET_GAME_STATE() != 3 || GetTickCount64() < g_syncQuietUntil)
	{
		g_ownedKnown = false;

		return;
	}

	Actor player = ACTOR::GET_PLAYER_ACTOR(-1);

	// v5.19e: the Short Bow (Undead Nightmare content, excluded from the randomizer) comes back after every respawn/checkpoint
	// restore, so it is removed again whenever it is seen.
	{
		int bow = FindWeaponIndex("short bow");

		if (bow >= 0 && INVENTORY::ACTOR_HAS_WEAPON(player, TEST_WEAPONS[bow].id))
		{
			Log("excluded weapon: Short Bow found, removing it");

			RemoveWeaponByIndex(bow);
		}
	}

	std::set<int> owned;

	for (int i = 0; i < TEST_WEAPON_COUNT; i++)
	{
		if (IsGun(TEST_WEAPONS[i].id) && INVENTORY::ACTOR_HAS_WEAPON(player, TEST_WEAPONS[i].id))
		{
			owned.insert((int)TEST_WEAPONS[i].id);
		}
	}

	if (!g_ownedKnown)
	{
		g_prevOwned = owned;
		g_ownedKnown = true;

		Log("weapon watch: baseline, player owns " + std::to_string(owned.size()) + " of the test guns");

		return;
	}

	// Several guns appearing in one second is an inventory restore (load, respawn), not a pickup.
	int appeared = 0;

	for (int id : owned)
	{
		if (!g_prevOwned.count(id) && !g_apGranted.count(id)) { appeared++; }
	}

	if (appeared >= 3)
	{
		Log("weapon watch: " + std::to_string(appeared) + " guns appeared at once, treated as an inventory restore (not marked)");

		g_prevOwned = owned;

		return;
	}

	for (int id : owned)
	{
		if (!g_prevOwned.count(id) && !g_apGranted.count(id) && !g_pendingReclaim.count(id))
		{
			g_pendingReclaim.insert(id);

			Log("weapon gained outside AP: id=" + std::to_string(id) + " (marked, will be removed after the mission). objective_hash=" + std::to_string(JOURNAL::GET_LAST_NOTE_OBJECTIVE()));

			ShopWeaponBought2(id);

			// v5.20m: every stray gun is taken away 5 s after it appears, in free roam and in missions alike
			// (thrown weapons such as dynamite are not guns and are never marked).
			if (g_pendingReclaim.count(id) && !g_apGranted.count(id) && g_apMode && !g_vanillaWeapons)
			{
				g_strayDue[id] = GetTickCount64() + 5000;

				Log("stray gun: id=" + std::to_string(id) + " will be removed in 5 s");
			}
		}
	}

	g_prevOwned = owned;

	// due free roam pickups
	if (!g_strayDue.empty() && SafeToDeliver())
	{
		bool removed = false;

		for (auto it = g_strayDue.begin(); it != g_strayDue.end();)
		{
			int id = it->first;

			if (GetTickCount64() < it->second) { ++it; continue; }

			WeaponModel weapon = (WeaponModel)id;

			if (!g_apGranted.count(id) && INVENTORY::ACTOR_HAS_WEAPON(player, weapon))
			{
				INVENTORY::DELETE_WEAPON_FROM_ACTOR(player, weapon);

				WEAPON::SET_WEAPONENUM_LOCKED(weapon, true);

				Log(std::string("stray gun removed: ") + WeaponLabel(weapon) + ", still_owned=" + std::to_string(INVENTORY::ACTOR_HAS_WEAPON(player, weapon)));

				removed = true;
			}

			g_pendingReclaim.erase(id);

			it = g_strayDue.erase(it);
		}

		if (removed)
		{
			ApSay(Tag("red", "That weapon isn't yours to keep."));

			g_ownedKnown = false;
		}
	}
}

static void ReclaimWeapons(const char* _Reason)
{
	if (g_vanillaWeapons) { g_pendingReclaim.clear(); return; }   // v5.21: vanilla weapon mode keeps every pickup

	if (g_pendingReclaim.empty())
	{
		return;
	}

	if (!SafeToDeliver())
	{
		Log(std::string("reclaim: deferred (") + _Reason + "), not safe right now");

		return;
	}

	Actor player = ACTOR::GET_PLAYER_ACTOR(-1);

	for (int id : g_pendingReclaim)
	{
		WeaponModel weapon = (WeaponModel)id;

		if (!g_apGranted.count(id) && INVENTORY::ACTOR_HAS_WEAPON(player, weapon))
		{
			INVENTORY::DELETE_WEAPON_FROM_ACTOR(player, weapon);

			WEAPON::SET_WEAPONENUM_LOCKED(weapon, true);

			Log(std::string("reclaim: removed ") + WeaponLabel(weapon) + " (" + _Reason + "), still_owned=" + std::to_string(INVENTORY::ACTOR_HAS_WEAPON(player, weapon)));
		}
	}

	g_pendingReclaim.clear();

	g_ownedKnown = false;
}



// Shop probe. The shop screen is built by the game's own script; these natives work on item handles whose numbers we do
// not know. Open a shop menu FIRST, then run "shopscan 16" (start small). It logs every handle with a price, quantity,
// blocked flag or misc flag. "shopblock <handle> <0|1>" then blocks/unblocks one item, so you can see whether it vanishes.
static void ShopScan(int _Max)
{
	if (_Max < 4) { _Max = 4; }
	if (_Max > 256) { _Max = 256; }

	Log("shop scan: begin, handles 0.." + std::to_string(_Max - 1));

	for (int i = 0; i < _Max; i++)
	{
		int price = SHOP::SHOP_GET_ITEM_PURCHASE_PRICE(i);
		int sell = SHOP::SHOP_GET_ITEM_SELL_PRICE(i);
		int quantity = SHOP::SHOP_GET_ITEM_QUANTITY(i);
		int blocked = SHOP::SHOP_IS_ITEM_BLOCKED(i);
		int flag = SHOP::SHOP_GET_ITEM_MISC_FLAG(i);

		if (price != 0 || sell != 0 || quantity != 0 || blocked != 0 || flag != 0)
		{
			Log("shop scan: handle=" + std::to_string(i) + " price=" + std::to_string(price) + " sell=" + std::to_string(sell) + " quantity=" + std::to_string(quantity) + " blocked=" + std::to_string(blocked) + " flag=" + std::to_string(flag));
		}
	}

	Log("shop scan: end");

	Say("Shop scan written to the log");
}

// v5.14: the shop rebuilds its item list each time it is opened, so a block or price set once is lost on re-entry.
// "shopblock <handle> <0|1>" and "shopprice <handle> <price>" remember the setting and apply it now. While
// "shopwatch 1" is on, a tick 4 times a second re-applies the remembered settings and logs every change of price,
// quantity or blocked state (that is how a purchase shows up). Turn shopwatch on only with a shop open, off afterwards.
// "shopclear" forgets the settings. Handles are different in every shop, so clear them when you change shops.
struct ShopOverride
{
	bool hasBlocked = false;
	int blocked = 0;
	bool hasPrice = false;
	int price = 0;
};

static std::map<int, ShopOverride> g_shopOverrides;
static bool g_shopWatch = false;

// v5.14: "after <seconds> <command>" runs a command later, so a command can be sent and then a menu (shop) opened in the
// game, since leaving the game window closes the menu. When due, the command is appended to the input file.
static std::vector<std::pair<ULONGLONG, std::string>> g_delayed;
static const int SHOP_WATCH_HANDLES = 32;
static int g_shopSnapshot[SHOP_WATCH_HANDLES][3];
static bool g_shopSnapshotValid = false;

static void ShopSetCommand(const std::string& _Command)
{
	bool isPrice = _Command.rfind("shopprice ", 0) == 0;

	std::string rest = Trim(_Command.substr(10));
	size_t sp = rest.find(' ');

	std::string handleText = sp == std::string::npos ? rest : Trim(rest.substr(0, sp));
	std::string valueText = sp == std::string::npos ? std::string() : Trim(rest.substr(sp + 1));

	if (handleText.empty() || handleText.find_first_not_of("0123456789") != std::string::npos
		|| valueText.empty() || valueText.find_first_not_of("0123456789") != std::string::npos)
	{
		Log(isPrice ? "shopprice: use  shopprice <handle number> <price>  (example: shopprice 3 10)"
					: "shopblock: use  shopblock <handle number> <0|1>  (example: shopblock 3 1; the word 'handle' is not typed)");

		return;
	}

	int handle = std::atoi(handleText.c_str());
	int value = std::atoi(valueText.c_str());

	ShopOverride& entry = g_shopOverrides[handle];

	if (isPrice)
	{
		entry.hasPrice = true;
		entry.price = value;

		SHOP::SHOP_SET_ITEM_PURCHASE_PRICE(handle, value);

		Log("shop price: handle=" + std::to_string(handle) + " set to " + std::to_string(value) + ", now price=" + std::to_string(SHOP::SHOP_GET_ITEM_PURCHASE_PRICE(handle)));
	}
	else
	{
		entry.hasBlocked = true;
		entry.blocked = value != 0;

		SHOP::SHOP_SET_ITEM_BLOCKED(handle, value != 0);

		Log("shop block: handle=" + std::to_string(handle) + " set to " + std::to_string(value != 0) + ", now blocked=" + std::to_string(SHOP::SHOP_IS_ITEM_BLOCKED(handle)));
	}
}

static void ShopWatchTick()
{
	for (int i = 0; i < SHOP_WATCH_HANDLES; i++)
	{
		auto found = g_shopOverrides.find(i);

		if (found != g_shopOverrides.end())
		{
			const ShopOverride& entry = found->second;

			if (entry.hasPrice && SHOP::SHOP_GET_ITEM_PURCHASE_PRICE(i) != entry.price)
			{
				SHOP::SHOP_SET_ITEM_PURCHASE_PRICE(i, entry.price);

				Log("shopwatch: re-applied price " + std::to_string(entry.price) + " on handle " + std::to_string(i));
			}

			if (entry.hasBlocked && SHOP::SHOP_IS_ITEM_BLOCKED(i) != entry.blocked)
			{
				SHOP::SHOP_SET_ITEM_BLOCKED(i, entry.blocked);

				Log("shopwatch: re-applied blocked=" + std::to_string(entry.blocked) + " on handle " + std::to_string(i));
			}
		}

		int now[3] = { SHOP::SHOP_GET_ITEM_PURCHASE_PRICE(i), SHOP::SHOP_GET_ITEM_QUANTITY(i), SHOP::SHOP_IS_ITEM_BLOCKED(i) };

		if (g_shopSnapshotValid)
		{
			if (now[0] != g_shopSnapshot[i][0]) { Log("shopwatch: handle=" + std::to_string(i) + " price " + std::to_string(g_shopSnapshot[i][0]) + " -> " + std::to_string(now[0])); }
			if (now[1] != g_shopSnapshot[i][1]) { Log("shopwatch: handle=" + std::to_string(i) + " quantity " + std::to_string(g_shopSnapshot[i][1]) + " -> " + std::to_string(now[1])); }
			if (now[2] != g_shopSnapshot[i][2]) { Log("shopwatch: handle=" + std::to_string(i) + " blocked " + std::to_string(g_shopSnapshot[i][2]) + " -> " + std::to_string(now[2])); }
		}

		for (int k = 0; k < 3; k++) { g_shopSnapshot[i][k] = now[k]; }
	}

	g_shopSnapshotValid = true;
}

// v5.15: shop rules. Each gear/weapon slot in a shop is an Archipelago location. Nothing is blocked (a blocked item cannot
// be bought, so it could never send a check); instead the item is made buyable, buying it sends the check, and a bought
// weapon is taken away again unless Archipelago already gave it. Consumables are never touched. Slots are found by
// their (unique) price because handles reshuffle; the shop itself is recognised by a consumable that is always there.
struct ShopRule
{
	int shop;               // 0 = Armadillo gun shop, 1 = Armadillo general store
	int origPrice;          // price the game uses
	int newPrice;           // unique price we set (same as origPrice unless two slots collide)
	const char* weapon;     // weapon name, or nullptr for gear (gear is not taken away yet)
	const char* location;   // Archipelago location name
	int handle = -1;        // v5.20m: fixed shop slot (from shopdump) when known; -1 = find the slot by price
	const char* town = "armadillo";   // v5.22: district name (CORE::GET_DISTRICTS_NAME) must contain this
};

static const ShopRule SHOP_RULES[] =
{
	{ 0, 125, 125, nullptr,                 "Armadillo Gun Shop - Bandolier" },
	{ 0, 100, 100, "Volcanic Pistol",       "Armadillo Gun Shop - Volcanic Pistol" },
	{ 0, 350, 350, "Winchester Repeater",   "Armadillo Gun Shop - Winchester Repeater" },
	{ 0, 475, 475, "Buffalo Rifle",         "Armadillo Gun Shop - Buffalo Rifle" },
	{ 0, 150, 150, "Double Barrel Shotgun", "Armadillo Gun Shop - Double Barrel Shotgun" },
	{ 1,  25,  25, nullptr,                 "Armadillo General Store - Rabbit's Foot", 0 },
	{ 1, 175, 175, nullptr,                 "Armadillo General Store - Improved Campsite", 1 },
	{ 1,  25,  25, nullptr,                 "Armadillo General Store - Survivalist Map", 2 },
	{ 1,  30,  30, nullptr,                 "Armadillo General Store - Chola Springs Map", 3 },
	{ 1,  40,  40, nullptr,                 "Armadillo General Store - Gaptooth Ridge Map", 4 },
};

static const int SHOP_RULE_COUNT = (int)(sizeof(SHOP_RULES) / sizeof(SHOP_RULES[0]));
#define SHOP_DONE_PATH (ApFile("rdr1_ap_shopdone.txt").c_str())
#define OUTBOX_PATH (ApFile("rdr1_ap_outbox.txt").c_str())

static bool g_shopRules = false;
static bool g_shopDoneLoaded = false;
static std::set<int> g_shopDone;                    // rule indices already bought (check sent)
static int g_shopActive = -1;                       // shop currently recognised, -1 = none
static bool g_shopSeen[SHOP_RULE_COUNT];            // rule slot visible on the previous tick
static int g_shopMissing[SHOP_RULE_COUNT];          // consecutive ticks a seen slot has been missing
static std::vector<std::pair<ULONGLONG, int>> g_shopRemovals;   // (time, weapon index) removal attempts for bought weapons

// v5.21 alpha: every location is written to the outbox once. Repeats (mission replays, reloads) are logged but not resent, and
// do not pop up again. The client reads the outbox and sends LocationChecks for every line.
static std::set<std::string> g_sentChecks;
static bool g_sentLoaded = false;

static void SendCheck(const std::string& _Location)
{
	if (!g_sentLoaded)
	{
		g_sentLoaded = true;

		std::ifstream in(OUTBOX_PATH);
		std::string line;

		while (std::getline(in, line)) { line = Trim(line); if (!line.empty()) { g_sentChecks.insert(line); } }
	}

	if (g_sentChecks.count(_Location))
	{
		Log("CHECK (already sent): " + _Location);

		return;
	}

	g_sentChecks.insert(_Location);

	Log("CHECK: " + _Location);

	std::ofstream out(OUTBOX_PATH, std::ios::app);

	out << _Location << "\n";

	// v5.24: no popup for found locations (only received items are announced)
}

// ---------------------------------------------------------------------------------------------
// v5.20 weapon sync. The game restores an older checkpoint inventory after a death/reload/respawn, which can lose weapons
// Archipelago gave and bring back ones it did not (the Short Bow). The received weapons are kept in rdr1_ap_received.txt (one
// weapon name per line) and re-applied after every load: missing received weapons are given, everything else is removed.
//   received add <weapon>     record it as received and give it now         received remove <weapon>   forget it and take it away
//   received list | received clear                                          sync 1|0   automatic sync after loads on/off
//   sync now                  run the sync immediately
// ---------------------------------------------------------------------------------------------
#define RECEIVED_PATH (ApFile("rdr1_ap_received.txt").c_str())
static std::set<int> g_received;           // TEST_WEAPONS indexes
static bool g_receivedLoaded = false;
static bool g_syncOn = false;
static int g_syncPrevState = -1;
static ULONGLONG g_syncAt = 0;
static ULONGLONG g_syncGiveUp = 0;

static void SaveReceived()
{
	std::ofstream out(RECEIVED_PATH, std::ios::trunc);

	for (int i : g_received) { out << TEST_WEAPONS[i].name << "\n"; }
}

static void LoadReceived()
{
	g_receivedLoaded = true;

	std::ifstream in(RECEIVED_PATH);

	std::string line;

	while (std::getline(in, line))
	{
		int index = FindWeaponIndex(Trim(line));

		if (index >= 0) { g_received.insert(index); }
	}

	Log("received: " + std::to_string(g_received.size()) + " weapons loaded from " + std::string(RECEIVED_PATH));
}

static bool IsProtectedWeapon(int _Index)
{
	return TEST_WEAPONS[_Index].id == WEAPON_MELEE_Knife || TEST_WEAPONS[_Index].id == WEAPON_LASSO_Lasso;
}

static void SyncNow(const char* _Reason)
{
	if (!g_receivedLoaded) { LoadReceived(); }

	Actor player = ACTOR::GET_PLAYER_ACTOR(-1);

	int given = 0;
	int removed = 0;

	g_syncQuietUntil = GetTickCount64() + 6000;   // v5.20k: set first so removals during the sync skip the knife holster reset

	for (int i = 0; i < TEST_WEAPON_COUNT; i++)
	{
		if (IsProtectedWeapon(i)) { continue; }

		bool has = INVENTORY::ACTOR_HAS_WEAPON(player, TEST_WEAPONS[i].id) != 0;

		if (g_received.count(i))
		{
			if (!has)
			{
				GiveWeapon(i, false);

				given++;
			}
			else
			{
				g_apGranted.insert((int)TEST_WEAPONS[i].id);
				g_pendingReclaim.erase((int)TEST_WEAPONS[i].id);
			}
		}
		else if (has)
		{
			RemoveWeaponByIndex(i);

			removed++;
		}
	}

	g_ownedKnown = false;
	g_syncQuietUntil = GetTickCount64() + 6000;

	Log(std::string("SYNC (") + _Reason + "): received=" + std::to_string(g_received.size()) + " given=" + std::to_string(given) + " removed=" + std::to_string(removed)
		+ " " + WheelDump(player));
}

static void ReceivedCommand(const std::string& _Args)
{
	if (!g_receivedLoaded) { LoadReceived(); }

	std::string args = Trim(_Args);

	if (args == "list")
	{
		std::string text = "received list:";

		for (int i : g_received) { text += std::string(" [") + TEST_WEAPONS[i].name + "]"; }

		Log(text);

		return;
	}

	if (args == "clear")
	{
		g_received.clear();

		SaveReceived();

		Log("received: cleared");

		return;
	}

	bool add = args.rfind("add ", 0) == 0;
	bool remove = args.rfind("remove ", 0) == 0;

	if (!add && !remove)
	{
		Log("received: use  received add|remove <weapon>,  received list,  received clear");

		return;
	}

	int index = FindWeaponIndex(Trim(args.substr(add ? 4 : 7)));

	if (index < 0 || IsProtectedWeapon(index))
	{
		Log("received: no weapon matches '" + args + "'");

		return;
	}

	if (add)
	{
		g_received.insert(index);

		SaveReceived();

		if (!INVENTORY::ACTOR_HAS_WEAPON(ACTOR::GET_PLAYER_ACTOR(-1), TEST_WEAPONS[index].id)) { GiveWeapon(index, false); }
		else { g_apGranted.insert((int)TEST_WEAPONS[index].id); }

		Log(std::string("received: added ") + TEST_WEAPONS[index].name);
	}
	else
	{
		g_received.erase(index);

		SaveReceived();

		RemoveWeaponByIndex(index);

		Log(std::string("received: removed ") + TEST_WEAPONS[index].name);
	}
}

static void SyncTick(ULONGLONG _Now)
{
	int state = GAME::GET_GAME_STATE();

	if (g_syncPrevState == 5 && state == 3 && g_syncOn)
	{
		g_syncAt = _Now + 3000;
		g_syncGiveUp = _Now + 90000;

		Log("sync: load finished, sync scheduled");
	}

	g_syncPrevState = state;

	if (!g_syncOn || g_syncAt == 0 || _Now < g_syncAt)
	{
		return;
	}

	bool safe = state == 3 && !CUTSCENE::CUTSCENE_MANAGER_IS_CUTSCENE_PLAYING() && ACTOR::IS_PLAYER_CONTROLLABLE(-1)
		&& !HEALTH::IS_ACTOR_DEAD(ACTOR::GET_PLAYER_ACTOR(-1));

	if (safe)
	{
		SyncNow("after load");

		g_syncAt = 0;
	}
	else if (_Now > g_syncGiveUp)
	{
		Log("sync: gave up waiting for a safe moment");

		g_syncAt = 0;
	}
	else
	{
		g_syncAt = _Now + 1000;
	}
}

// ---------------------------------------------------------------------------------------------
// v5.20 brawl trap: "brawl [brute|undertaker|preacher|miner|zombie]". John gets drunk for 3 s, a melee-only enemy is spawned
// next to him and set on him, and the enemy is removed again after 7 s. The shove tracker logs ragdoll and health lost.
// ---------------------------------------------------------------------------------------------
static Actor g_brawlActor = -1;
static ULONGLONG g_brawlDespawnAt = 0;

static void BrawlCommand(const std::string& _Args)
{
	std::string key = Lower(Trim(_Args));

	ActorModel model = ACTOR_GANG_DRUNKNDIRTY_Hard01;

	if (key == "undertaker") { model = ACTOR_CAUCASIAN_MALE_Undertaker01; }
	else if (key == "preacher") { model = ACTOR_CAUCASIAN_MALE_Preacher01; }
	else if (key == "miner") { model = ACTOR_GANG_CRAZYMINER_Hard01; }
	else if (key == "zombie") { model = ACTOR_UN_ZOMBIE_Bruiser_01; }

	STREAM::STREAMING_REQUEST_ACTOR(model, true, true);

	ULONGLONG start = GetTickCount64();

	while (!STREAM::STREAMING_IS_ACTOR_LOADED(model, 0) && GetTickCount64() < start + 3000)
	{
		ScriptWait(0);
	}

	if (!STREAM::STREAMING_IS_ACTOR_LOADED(model, 0))
	{
		Log("brawl: model " + std::to_string((int)model) + " (" + key + ") did not load");

		return;
	}

	Actor player = ACTOR::GET_PLAYER_ACTOR(-1);

	Vector3 position = ACTOR::GET_POSITION(player);

	float angle = (float)(std::rand() % 628) / 100.0f;

	Layout layout = OBJECT::FIND_NAMED_LAYOUT("PlayerLayout");

	std::string objectName = "ap_trap_brawl_" + std::to_string(g_spawnCounter++);

	Actor actor = OBJECT::CREATE_ACTOR_IN_LAYOUT(layout, objectName.c_str(), model, Vector2(position.x + std::cos(angle) * 3.0f, position.y + std::sin(angle) * 3.0f), position.z + 1.0f, Vector2(), 0.0f);

	if ((int)actor <= 0)
	{
		Log("brawl: spawn FAILED, id " + std::to_string((int)actor));

		return;
	}

	g_spawned.push_back({ actor, GetTickCount64() });

	// v5.20k: the brute spawned with a gun and shot John (80 -> 22 health). Strip every weapon so only fists are left.
	INVENTORY::DELETE_ALL_WEAPONS_FROM_ACTOR(actor);

	g_brawlActor = actor;
	g_brawlDespawnAt = GetTickCount64() + 7000;

	HEALTH::SET_ACTOR_DRUNK(player, true);

	g_drunkUntil = GetTickCount64() + 3000;

	g_shoveHealth0 = HEALTH::GET_ACTOR_HEALTH(player);
	g_shoveRagdoll = false;
	g_shoveSafe = false;
	g_shoveUntil = GetTickCount64() + 7000;

	AI_MEMORY::MEMORY_CONSIDER_AS_ENEMY(actor, player);
	AI_MEMORY::MEMORY_ATTACK_ON_SIGHT(actor, player);
	TASKS::TASK_MELEE_ATTACK(actor, player, 0);

	Log("brawl: " + (key.empty() ? std::string("brute") : key) + " spawned, actor " + std::to_string((int)actor) + " faction " + std::to_string(FACTION::GET_ACTOR_FACTION(actor)) + ", health before=" + std::to_string(g_shoveHealth0));

	Say("Brawl!");
}

static void BrawlTick(ULONGLONG _Now)
{
	if (g_brawlActor < 0)
	{
		return;
	}

	// v5.20k: safety, the brawler leaves early when John is low (a trap should hurt, not kill).
	bool low = HEALTH::GET_ACTOR_HEALTH(ACTOR::GET_PLAYER_ACTOR(-1)) < 35.0f;

	if (_Now < g_brawlDespawnAt && !low)
	{
		return;
	}

	if (ENTITY::IS_ACTOR_VALID(g_brawlActor)) { OBJECT::DESTROY_ACTOR(g_brawlActor); }

	Log("brawl: enemy " + std::to_string((int)g_brawlActor) + " removed");

	g_brawlActor = -1;
}

// ---------------------------------------------------------------------------------------------
// v5.20 more traps (all untested):
//   slowmo [seconds] [scale]       REDHOOK::SET_TIME_SCALE for a while (default 6 s at 0.35), then back to 1.0
//   storm [seconds]                stormy weather with rain and lightning for a while (default 120 s), then the old weather
//   stampede [sheep|goat|pig|chicken] [count]   a herd (default 8) spawns around John and follows him; removed after 45 s
//   outofbullets                   empties the ammo of every owned gun and of every ammo type
//   (the posse of lawmen is "gangattack lawmen [count]"; "gangattack zombie" tests the Undead Nightmare zombie models)
// ---------------------------------------------------------------------------------------------
static ULONGLONG g_slowUntil = 0;
static ULONGLONG g_stormUntil = 0;
static Weather g_stormPrevWeather = WEATHER_CLEAR;
static ULONGLONG g_herdDespawnAt = 0;
static std::vector<Actor> g_herd;

static void SlowmoCommand(const std::string& _Args)
{
	float seconds = 6.0f;
	float scale = 0.35f;

	std::string args = Trim(_Args);

	if (!args.empty())
	{
		size_t space = args.find(' ');

		seconds = (float)std::atof(args.substr(0, space).c_str());

		if (space != std::string::npos) { scale = (float)std::atof(Trim(args.substr(space + 1)).c_str()); }
	}

	if (seconds <= 0.0f) { seconds = 6.0f; }
	if (scale <= 0.05f || scale > 1.0f) { scale = 0.35f; }

	REDHOOK::SET_TIME_SCALE(scale);

	g_slowUntil = GetTickCount64() + (ULONGLONG)(seconds * 1000.0f);

	Log("slowmo: time scale " + std::to_string(scale) + " for " + std::to_string(seconds) + " s, reads back " + std::to_string(REDHOOK::GET_TIME_SCALE()));

	ApBig("Slow Motion", "Time is dragging its boots...");
}

static void StormCommand(const std::string& _Args)
{
	int seconds = std::atoi(Trim(_Args).c_str());

	if (seconds <= 0) { seconds = 120; }

	if (g_stormUntil == 0) { g_stormPrevWeather = WEATHER::GET_WEATHER(); }

	WEATHER::SET_WEATHER(WEATHER_STORMY, 5.0f);
	WEATHER::SET_RAIN_AMOUNT(1.0f);
	WEATHER::SET_LIGHTNING_AMOUNT(1.0f);

	g_stormUntil = GetTickCount64() + (ULONGLONG)seconds * 1000;

	Log("storm: stormy weather for " + std::to_string(seconds) + " s, previous weather " + std::to_string((int)g_stormPrevWeather) + ", now " + std::to_string((int)WEATHER::GET_WEATHER()));
}

static void StampedeCommand(const std::string& _Args)
{
	std::string kind;   // v5.20k: no kind given = a random big animal
	int count = 8;

	{
		std::string args = Lower(Trim(_Args));
		size_t space = args.find(' ');
		std::string first = args.substr(0, space);

		if (!first.empty() && first.find_first_not_of("0123456789") == std::string::npos) { count = std::atoi(first.c_str()); }
		else if (!first.empty()) { kind = first; }

		if (space != std::string::npos) { count = std::atoi(Trim(args.substr(space + 1)).c_str()); }
	}

	if (count < 1) { count = 1; }
	if (count > 12) { count = 12; }

	if (kind.empty())
	{
		static const char* BIG[] = { "buffalo", "elk", "bighorn" };   // deer and boar removed from the random pick

		kind = BIG[std::rand() % 3];
	}

	ActorModel model = ACTOR_ANIMAL_Sheep;

	if (kind == "buffalo") { model = ACTOR_ANIMAL_NOR_Buffalo; }
	else if (kind == "elk") { model = ACTOR_ANIMAL_NOR_Elk; }
	else if (kind == "bighorn") { model = ACTOR_ANIMAL_BigHorn; }
	else if (kind == "boar") { model = ACTOR_ANIMAL_NOR_WildBoar; }
	else if (kind == "deer") { model = ACTOR_ANIMAL_Deer; }
	else if (kind == "goat") { model = ACTOR_ANIMAL_Goat01; }
	else if (kind == "pig") { model = ACTOR_ANIMAL_Pig; }
	else if (kind == "chicken") { model = ACTOR_ANIMAL_Chicken; }

	STREAM::STREAMING_REQUEST_ACTOR(model, true, true);

	ULONGLONG start = GetTickCount64();

	while (!STREAM::STREAMING_IS_ACTOR_LOADED(model, 0) && GetTickCount64() < start + 3000)
	{
		ScriptWait(0);
	}

	if (!STREAM::STREAMING_IS_ACTOR_LOADED(model, 0))
	{
		Log("stampede: model for '" + kind + "' did not load");

		return;
	}

	Actor player = ACTOR::GET_PLAYER_ACTOR(-1);

	Vector3 position = ACTOR::GET_POSITION(player);

	Layout layout = OBJECT::FIND_NAMED_LAYOUT("PlayerLayout");

	int made = 0;

	for (int i = 0; i < count; i++)
	{
		float angle = (float)(std::rand() % 628) / 100.0f;
		float distance = 4.0f + (float)(std::rand() % 30) / 10.0f;   // v5.20k: tighter, 4 to 7 m around John

		std::string objectName = "ap_trap_herd_" + std::to_string(g_spawnCounter++);

		Actor actor = OBJECT::CREATE_ACTOR_IN_LAYOUT(layout, objectName.c_str(), model, Vector2(position.x + std::cos(angle) * distance, position.y + std::sin(angle) * distance), position.z + 1.0f, Vector2(), 0.0f);

		if ((int)actor <= 0) { continue; }

		g_herd.push_back(actor);

		TASKS::TASK_FOLLOW_ACTOR(actor, player);

		// v5.20k: they arrive running (a stampede) instead of strolling.
		ENTITY::SET_ACTOR_MIN_SPEED_ABSOLUTE(actor, 6.0f);

		made++;
	}

	g_herdDespawnAt = GetTickCount64() + 45000;

	Log("stampede: " + kind + " x" + std::to_string(made) + " of " + std::to_string(count) + " spawned and following John");

	ApBig("Stampede!", "Get out of the way!");
}

// v5.20k: half of the emptied ammo comes back after 20 s ("You recovered some of your bullets...").
static std::map<int, float> g_obWeaponAmmo;   // weapon index -> loaded ammo before the trap
static float g_obInvAmmo[16];                 // ammo type -> reserve before the trap
static ULONGLONG g_obRestoreAt = 0;

static void OutOfBulletsCommand()
{
	Actor player = ACTOR::GET_PLAYER_ACTOR(-1);

	int emptied = 0;

	if (g_obRestoreAt == 0)   // a second trap while one is pending keeps the original amounts
	{
		g_obWeaponAmmo.clear();

		for (int i = 0; i < TEST_WEAPON_COUNT; i++)
		{
			if (INVENTORY::ACTOR_HAS_WEAPON(player, TEST_WEAPONS[i].id) && (int)WEAPON::GET_AMMO_ENUM(TEST_WEAPONS[i].id) > 0)
			{
				g_obWeaponAmmo[i] = INVENTORY::ACTOR_GET_WEAPON_AMMO(player, TEST_WEAPONS[i].id);
			}
		}

		for (int type = 3; type <= 12; type++)
		{
			g_obInvAmmo[type] = INVENTORY::ACTOR_GET_INV_AMMO(player, (AmmoType)type, false);
		}
	}

	g_obRestoreAt = GetTickCount64() + 20000;

	for (int i = 0; i < TEST_WEAPON_COUNT; i++)
	{
		if (INVENTORY::ACTOR_HAS_WEAPON(player, TEST_WEAPONS[i].id) && (int)WEAPON::GET_AMMO_ENUM(TEST_WEAPONS[i].id) > 0)
		{
			INVENTORY::ACTOR_SET_WEAPON_AMMO(player, TEST_WEAPONS[i].id, 0.0f);

			emptied++;
		}
	}

	for (int type = 3; type <= 12; type++)
	{
		INVENTORY::ACTOR_SET_INV_AMMO(player, (AmmoType)type, 0.0f, false);
	}

	std::string text = "outofbullets: emptied " + std::to_string(emptied) + " weapons;";

	for (int i = 0; i < TEST_WEAPON_COUNT; i++)
	{
		if (INVENTORY::ACTOR_HAS_WEAPON(player, TEST_WEAPONS[i].id) && (int)WEAPON::GET_AMMO_ENUM(TEST_WEAPONS[i].id) > 0)
		{
			text += std::string(" ") + TEST_WEAPONS[i].name + "=" + std::to_string(INVENTORY::ACTOR_GET_WEAPON_AMMO(player, TEST_WEAPONS[i].id));
		}
	}

	Log(text);

	ApBig("Out of Bullets", "Your guns are empty. Some ammo comes back in 20 seconds.");
}

static void OutOfBulletsRestoreTick(ULONGLONG _Now)
{
	if (g_obRestoreAt == 0 || _Now < g_obRestoreAt)
	{
		return;
	}

	g_obRestoreAt = 0;

	Actor player = ACTOR::GET_PLAYER_ACTOR(-1);

	std::string text = "outofbullets: restored half:";

	for (const auto& entry : g_obWeaponAmmo)
	{
		if (INVENTORY::ACTOR_HAS_WEAPON(player, TEST_WEAPONS[entry.first].id))
		{
			// v5.21: a Max Ammo received during the 20 s must not be undone, so the restore only ever raises the ammo.
			float half = (std::max)(std::floor(entry.second / 2.0f), INVENTORY::ACTOR_GET_WEAPON_AMMO(player, TEST_WEAPONS[entry.first].id));

			INVENTORY::ACTOR_SET_WEAPON_AMMO(player, TEST_WEAPONS[entry.first].id, half);

			text += std::string(" ") + TEST_WEAPONS[entry.first].name + "=" + std::to_string(half);
		}
	}

	for (int type = 3; type <= 12; type++)
	{
		if (g_obInvAmmo[type] > 0.0f)
		{
			float current = INVENTORY::ACTOR_GET_INV_AMMO(player, (AmmoType)type, false);

			INVENTORY::ACTOR_SET_INV_AMMO(player, (AmmoType)type, (std::max)(current, std::floor(g_obInvAmmo[type] / 2.0f)), false);

			text += " type" + std::to_string(type) + "=" + std::to_string(std::floor(g_obInvAmmo[type] / 2.0f));
		}
	}

	Log(text);

	ApBig("Bullets Recovered", "You recovered some of your bullets...");
}

// v5.20k "modelprobe <from> <to>": spawns each actor model id briefly, logs its species and removes it again. Used to find which
// ids are the zombie humans in this install (1204 and 1214 came out as cougars).
static void ModelProbeCommand(const std::string& _Args)
{
	int from = 0, to = 0;

	{
		char* end = nullptr;

		from = (int)std::strtol(_Args.c_str(), &end, 10);
		to = (int)std::strtol(end, nullptr, 10);
	}

	if (from <= 0 || to < from) { Log("modelprobe: use  modelprobe <from> <to>"); return; }
	if (to - from > 60) { to = from + 60; }

	Actor player = ACTOR::GET_PLAYER_ACTOR(-1);
	Vector3 position = ACTOR::GET_POSITION(player);
	Layout layout = OBJECT::FIND_NAMED_LAYOUT("PlayerLayout");

	for (int id = from; id <= to; id++)
	{
		ActorModel model = (ActorModel)id;

		STREAM::STREAMING_REQUEST_ACTOR(model, true, true);

		ULONGLONG start = GetTickCount64();

		while (!STREAM::STREAMING_IS_ACTOR_LOADED(model, 0) && GetTickCount64() < start + 400) { ScriptWait(0); }

		if (!STREAM::STREAMING_IS_ACTOR_LOADED(model, 0)) { Log("modelprobe " + std::to_string(id) + ": did not load"); continue; }

		std::string name = "ap_probe_" + std::to_string(g_spawnCounter++);

		Actor actor = OBJECT::CREATE_ACTOR_IN_LAYOUT(layout, name.c_str(), model, Vector2(position.x + 25.0f, position.y), position.z + 1.0f, Vector2(), 0.0f);

		if ((int)actor <= 0) { Log("modelprobe " + std::to_string(id) + ": spawn failed"); continue; }

		ScriptWait(0);

		Log("modelprobe " + std::to_string(id) + ": species=" + std::to_string(AI_ANIMAL::ANIMAL_ACTOR_GET_SPECIES(actor)) + " health=" + std::to_string(HEALTH::GET_ACTOR_HEALTH(actor)));

		OBJECT::DESTROY_ACTOR(actor);
	}

	Log("modelprobe: done");
}

static ULONGLONG g_passoutUntil = 0;

static void MoreTrapsTick(ULONGLONG _Now)
{
	if (g_passoutUntil != 0 && _Now >= g_passoutUntil)
	{
		Actor wakePlayer = ACTOR::GET_PLAYER_ACTOR(-1);

		g_passoutUntil = 0;

		HEALTH::SET_ACTOR_PASSED_OUT(wakePlayer, 0);
		HEALTH::SET_ACTOR_DRUNK(wakePlayer, false);

		Log("passout: timer over, woke John, is_drunk=" + std::to_string(HEALTH::IS_ACTOR_DRUNK(wakePlayer)));
	}

	OutOfBulletsRestoreTick(_Now);

	if (g_slowUntil != 0 && (_Now >= g_slowUntil || HEALTH::IS_ACTOR_DEAD(ACTOR::GET_PLAYER_ACTOR(-1))))
	{
		REDHOOK::SET_TIME_SCALE(1.0f);

		g_slowUntil = 0;

		Log("slowmo: back to normal speed, reads back " + std::to_string(REDHOOK::GET_TIME_SCALE()));
	}

	if (g_stormUntil != 0 && _Now < g_stormUntil)
	{
		// v5.20k: the first test left the weather at 1 (Fair), so scripted weather may overwrite it. Reapply every second.
		static ULONGLONG s_nextStorm = 0;

		if (_Now >= s_nextStorm)
		{
			s_nextStorm = _Now + 1000;

			WEATHER::SET_WEATHER(WEATHER_STORMY, 5.0f);
			WEATHER::SET_RAIN_AMOUNT(1.0f);
			WEATHER::SET_LIGHTNING_AMOUNT(1.0f);

			static int s_stormLogs = 0;

			if (s_stormLogs++ % 5 == 0)
			{
				Log("storm: weather reads " + std::to_string((int)WEATHER::GET_WEATHER()));
			}

			if (std::rand() % 6 == 0) { WEATHER::TRIGGER_CLOUD_LIGHTNING_SEQUENCE(); }
		}
	}

	if (g_stormUntil != 0 && _Now >= g_stormUntil)
	{
		WEATHER::SET_WEATHER(g_stormPrevWeather, 0.5f);
		WEATHER::SET_RAIN_AMOUNT(0.0f);
		WEATHER::SET_LIGHTNING_AMOUNT(0.0f);
		WEATHER::SET_AUTO_WIND();

		g_stormUntil = 0;

		Log("storm: over, weather set back to " + std::to_string((int)g_stormPrevWeather) + ", now " + std::to_string((int)WEATHER::GET_WEATHER()));
	}

	if (!g_herd.empty() && _Now >= g_herdDespawnAt)
	{
		for (Actor actor : g_herd)
		{
			if (ENTITY::IS_ACTOR_VALID(actor)) { OBJECT::DESTROY_ACTOR(actor); }
		}

		Log("stampede: " + std::to_string(g_herd.size()) + " animals removed");

		g_herd.clear();
	}
}

// v5.19: a weapon slot is not removed from the shelf when bought (the first test saw the weapon appear but the slot stay), so
// a weapon that shows up while a recognised shop is open is the purchase signal for its slot.
static std::string g_shopTown;   // v5.22: lower-case district name read when a shop was recognised

static bool RuleTownMatches(const ShopRule& _Rule)
{
	// the district must name the rule's town; if it names no town we know (e.g. a region name), Armadillo rules still apply
	static const char* OTHER_TOWNS[] = { "thieves", "blackwater", "chuparosa", "escalera", "casa", "manzanita", "tumbleweed",
		"presidio", "torquemada", "plainview", "rathskeller", "beecher", "macfarlane", "agave", "coot" };

	if (g_shopTown.find(_Rule.town) != std::string::npos) { return true; }

	if (std::string(_Rule.town) != "armadillo") { return false; }

	for (const char* t : OTHER_TOWNS) { if (g_shopTown.find(t) != std::string::npos) { return false; } }

	return true;
}

static void ShopWeaponBought(int _WeaponId)
{
	if (!g_shopRules || g_shopActive < 0)
	{
		return;
	}

	for (int r = 0; r < SHOP_RULE_COUNT; r++)
	{
		const ShopRule& rule = SHOP_RULES[r];

		if (rule.shop != g_shopActive || !RuleTownMatches(rule) || rule.weapon == nullptr || g_shopDone.count(r))
		{
			continue;
		}

		int index = FindWeaponIndex(rule.weapon);

		if (index < 0 || (int)TEST_WEAPONS[index].id != _WeaponId)
		{
			continue;
		}

		g_shopDone.insert(r);

		{
			std::ofstream out(SHOP_DONE_PATH, std::ios::app);

			out << r << "\n";
		}

		SendCheck(rule.location);

		if (!g_apGranted.count(_WeaponId) && !g_vanillaWeapons)
		{
			g_shopRemovals.push_back({ GetTickCount64() + 1500, index });
			g_shopRemovals.push_back({ GetTickCount64() + 4000, index });

			Log(std::string("shoprules: bought ") + rule.weapon + " will be removed again (not received from Archipelago)");
		}

		return;
	}
}

static void LoadShopDone()
{
	g_shopDoneLoaded = true;

	std::ifstream in(SHOP_DONE_PATH);

	int index;

	while (in >> index)
	{
		g_shopDone.insert(index);
	}

	Log("shoprules: " + std::to_string(g_shopDone.size()) + " slots already bought (from " + std::string(SHOP_DONE_PATH) + ")");
}

static std::set<std::string> g_apGearGranted;                       // v5.20m: gear names given through Archipelago (never removed)
static std::vector<std::pair<ULONGLONG, std::string>> g_gearRemovals;   // v5.20m: pending removals of bought gear


// =============================================================================================
// v5.23 hideouts + minigames (from the decompiled scripts)
//   Hideout cleared: each hideout script writes its own best-time stat when the last outlaw dies (pikesbasin.c: STAT_SET_VALUE
//   on 462 right after "all squads eliminated"). 459 Twin Rocks, 461 Tumbleweed, 462 Pike's Basin, 463 Gaptooth Breach,
//   464 Fort Mercer, 465 Tesoro Azul, 466 Nosalida. Any change to a positive value = cleared.
//   Minigame win: every minigame adds the winnings to its own "money won" stat on a win and subtracts on a loss
//   (armwrestling.c win branch: STAT_INCREASE 441 + 450). 424 poker hands won, 445 blackjack (session profit, per-hand TBD), 441 arm wrestling, 437 five finger fillet,
//   433 horseshoes, 429 liar's dice; 451 duels won. An increase = one win, sent as "<Game>: Win N" (count saved per run).
// =============================================================================================
struct StatCheck { int id; const char* name; };

static const StatCheck HIDEOUT_STATS[] =
{
	{ 459, "Twin Rocks" }, { 461, "Tumbleweed" }, { 462, "Pike's Basin" }, { 463, "Gaptooth Breach" },
	{ 464, "Fort Mercer" }, { 465, "Tesoro Azul" }, { 466, "Nosalida" },
};

static const StatCheck MINIGAME_STATS[] =
{
	{ 424, "Poker" }, { 445, "Blackjack" },   // v5.26: poker = hands won (424, +1 per pot won, tested); 411 was session profit { 441, "Arm Wrestling" }, { 437, "Five Finger Fillet" },
	{ 433, "Horseshoes" }, { 429, "Liar's Dice" }, { 451, "Duels" },
};

static std::map<int, float> g_statPrev;
static ULONGLONG g_statChecksStableAt = 0;
static bool OutfitGive(const std::string& _Name);

static void StatChecksTick(ULONGLONG _Now)
{
	static bool s_outfitsApplied = false;

	if (GAME::GET_GAME_STATE() != 3) { g_statChecksStableAt = _Now + 4000; g_statPrev.clear(); s_outfitsApplied = false; return; }

	bool stable = _Now >= g_statChecksStableAt;

	// v5.25: received outfits are given again after every load (an older save would not have them)
	if (stable && !s_outfitsApplied && g_apMode)
	{
		s_outfitsApplied = true;

		ProgressLoad();

		for (const auto& kv : g_progress)
		{
			if (kv.first.rfind("outfit:", 0) == 0 && kv.second > 0) { OutfitGive(kv.first.substr(7)); }
		}
	}

	for (const StatCheck& h : HIDEOUT_STATS)
	{
		float v = StatGet(h.id);
		auto it = g_statPrev.find(h.id);

		if (stable && it != g_statPrev.end() && v != it->second && v > 0.0f)
		{
			Log(std::string("HIDEOUT CLEARED: ") + h.name + " (stat " + std::to_string(h.id) + " " + std::to_string(it->second) + " -> " + std::to_string(v) + ")");

			SendCheck(std::string(h.name) + " Hideout: Cleared");
		}

		g_statPrev[h.id] = v;
	}

	// Beta: fame / honor milestones every 500 points (honor counts either way, good or bad). Sent once each.
	// v6.1 beta 2: turned off (not reliable enough yet); kMilestones = true brings them back.
	static const bool kMilestones = false;
	if (stable && kMilestones)
	{
		float fame = StatGet(3);
		float honor = std::fabs(StatGet(1));

		static std::set<std::string> s_tried;   // SendCheck dedups too, this just keeps the log quiet

		for (int k = 1; k <= 10; k++)
		{
			std::string f = "Fame Milestone " + std::to_string(k);
			std::string h = "Honor Milestone " + std::to_string(k);

			if (fame >= 500.0f * k && !s_tried.count(f)) { s_tried.insert(f); SendCheck(f); }
			if (honor >= 500.0f * k && !s_tried.count(h)) { s_tried.insert(h); SendCheck(h); }
		}
	}

	for (const StatCheck& m : MINIGAME_STATS)
	{
		float v = StatGet(m.id);
		auto it = g_statPrev.find(m.id);

		if (stable && it != g_statPrev.end() && v > it->second + 0.01f)
		{
			long long wins = ProgressGet(std::string("mg:") + m.name) + 1;

			ProgressSet(std::string("mg:") + m.name, wins);

			Log(std::string("MINIGAME WIN: ") + m.name + " #" + std::to_string(wins) + " (stat " + std::to_string(m.id) + " " + std::to_string(it->second) + " -> " + std::to_string(v) + ")");

			if (wins <= 5) { SendCheck(std::string(m.name) + ": Win " + std::to_string(wins)); }
		}

		g_statPrev[m.id] = v;
	}
}


// v5.25 outfits. Outfit table Global_42265: entry n starts at 4-byte index 42265 + 2 + 18*n; +0 = index, +2 = flags,
// +8..+15 = internal name ("social", "rancher", ...). Owning an outfit = flag bits 2 (discovered) and 4 (wearable): the
// Gentleman's Attire went from flags 9 to 15 when it was bought (tested). The pause menu shows it with
// SATCHEL_SET_OUTFIT_STATE(hash("out_" + name), 3), which we call too so it shows at once.
static int OutfitIndexByName(const std::string& _Name, uintptr_t _Base)
{
	for (int n = 0; n < 30; n++)
	{
		char label[33] = {};

		std::memcpy(label, (const void*)(_Base + (uintptr_t)(42265 + 2 + 18 * n + 8) * 4), 32);

		if (Lower(label) == Lower(_Name)) { return n; }
	}

	return -1;
}

static bool OutfitGive(const std::string& _Name)
{
	uintptr_t base = GetGlobalPtr();

	if (base == 0) { Log("outfitgive: no global pointer"); return false; }

	int n = OutfitIndexByName(Trim(_Name), base);

	if (n < 0) { Log("outfitgive: no outfit named '" + _Name + "'"); return false; }

	int* flags = (int*)(base + (uintptr_t)(42265 + 2 + 18 * n + 2) * 4);
	int before = *flags;

	*flags = before | 2 | 4;

	Invoke<0x0A87A573, void>(NameHash("out_" + Lower(Trim(_Name))), 3);   // SHOP::SATCHEL_SET_OUTFIT_STATE

	Log("outfitgive: " + _Name + " (entry " + std::to_string(n) + ") flags " + std::to_string(before) + " -> " + std::to_string(*flags));

	return true;
}

// v5.23 "outfitdump": raw dump of the outfit table (Global_42265, 30 outfits). Printable text runs are shown, so the layout
// (where each outfit's name and flags sit) can be found from the outfits you own.
static void OutfitDump()
{
	uintptr_t base = GetGlobalPtr();

	if (base == 0) { Log("outfitdump: no global pointer"); return; }

	const int COUNT = 30 * 9 * 2 + 64;
	std::string line;

	for (int i = 0; i < COUNT; i++)
	{
		int v;

		std::memcpy(&v, (const void*)(base + (uintptr_t)(42265 + i) * 4), 4);

		std::string text;

		for (int b = 0; b < 4; b++)
		{
			char c = (char)((v >> (8 * b)) & 0xFF);

			text += (c >= 32 && c < 127) ? c : '.';
		}

		char buf[64];

		std::snprintf(buf, sizeof(buf), " %d:%08X'%s'", i, (unsigned int)v, text.c_str());

		line += buf;

		if (i % 8 == 7) { Log("outfitdump:" + line); line.clear(); }
	}

	if (!line.empty()) { Log("outfitdump:" + line); }

	ApSay("Outfit table written to the log");
}

// v5.20m: random encounters (sn_btin, "unique world events encountered"). Once per second, only in normal play.
static void EncounterTick()
{
	// v6.1 beta 2: turned off. sn_btin counts every world event, not only unique encounters like rescues.
	static const bool kEncounters = false;
	if (!kEncounters) { return; }

	static ULONGLONG s_next = 0;

	ULONGLONG now = GetTickCount64();

	if (now < s_next) { return; }

	s_next = now + 1000;

	if (GAME::GET_GAME_STATE() != 3 || GetTickCount64() < g_syncQuietUntil) { return; }

	float raw = StatGet(408);

	if (raw < 0.0f) { return; }

	long long value = (long long)(raw + 0.5f);
	long long high = ProgressHas("enc_high") ? ProgressGet("enc_high") : -1;

	if (high < 0)
	{
		ProgressSet("enc_base", value);
		ProgressSet("enc_high", value);

		Log("encounters: baseline set to " + std::to_string(value) + " (existing encounters in this save do not count)");

		return;
	}

	if (value > high)
	{
		for (long long n = high + 1; n <= value; n++)
		{
			SendCheck("Random Encounter " + std::to_string(n - ProgressGet("enc_base")));
		}

		ProgressSet("enc_high", value);
	}
}

// "shopdump": logs handle, price, quantity and blocked flag of every shop slot (find the slot of each item).
static void ShopDump()
{
	for (int i = 0; i < SHOP_WATCH_HANDLES; i++)
	{
		Log("shopdump: handle " + std::to_string(i) + " price " + std::to_string(SHOP::SHOP_GET_ITEM_PURCHASE_PRICE(i)) + " qty "
			+ std::to_string(SHOP::SHOP_GET_ITEM_QUANTITY(i)) + " blocked " + std::to_string(SHOP::SHOP_IS_ITEM_BLOCKED(i)));
	}
}

static void GearRemovalTick()
{
	for (size_t k = 0; k < g_gearRemovals.size();)
	{
		if (GetTickCount64() >= g_gearRemovals[k].first)
		{
			ItemDelete(g_gearRemovals[k].second);

			g_gearRemovals.erase(g_gearRemovals.begin() + k);
		}
		else
		{
			k++;
		}
	}
}


// =============================================================================================
// v5.23 shops, every town. Store stock and order from ireddead.com "shop items" (cross-checked: the Armadillo general store
// order there is exactly the slot-handle order our shopdump showed). A shop is recognised by its SHELF (the set of prices on
// sale), not by town name. Every non-consumable item is ONE location "Shop - <item>" that can be bought in any store that sells
// it (once you own a Rabbit's Foot no other store sells one, so per-store checks would be impossible to get).
// Horse deeds are blocked in every general store. Items already sent are blocked everywhere (no double buying).
// =============================================================================================
struct StoreItem { const char* name; int price; bool check; const char* weapon; const char* gear; };
struct StoreDef { const char* store; bool general; std::vector<StoreItem> items; };

#define CK(n, p) { n, p, true, nullptr, nullptr }
#define CW(n, p, w) { n, p, true, w, nullptr }
#define CG(n, p, g) { n, p, true, nullptr, g }
#define XX(n, p) { n, p, false, nullptr, nullptr }

static const std::vector<StoreDef> STORES =
{
	{ "Armadillo General Store", true, { CG("Rabbit's Foot", 25, "RABBITS_FOOT"), CK("Improved Campsite", 175), XX("Apple", 1), XX("Horse Pills", 20),
		CK("Survivalist Map", 25), CK("Cholla Springs Map", 30), CK("Gaptooth Ridge Map", 40) } },
	{ "Thieves' Landing General Store", true, { CG("Rabbit's Foot", 25, "RABBITS_FOOT"), CK("Improved Campsite", 175), XX("Apple", 1), XX("Horse Pills", 20),
		CK("Survivalist Map", 25), XX("Chewing Tobacco", 35), CK("Rio Bravo Map", 35) } },
	{ "Manzanita Post General Store", true, { CG("Rabbit's Foot", 25, "RABBITS_FOOT"), XX("Bait", 15), XX("Apple", 1), XX("Horse Pills", 20), CK("Survivalist Map", 25),
		XX("Medicine", 25), XX("Chewing Tobacco", 35), XX("Throwing Knife", 10), XX("Tomahawk", 10), XX("Fire Bottle", 25), XX("Dynamite", 45),
		XX("Moonshine", 60), CK("Tall Trees Map", 60), XX("Revolver Ammo", 12), XX("Pistol Ammo", 15), XX("Repeater Ammo", 25), XX("Rifle Ammo", 20),
		XX("Shotgun Ammo", 30), XX("Sniper Rifle Ammo", 40) } },
	{ "Blackwater General Store", true, { CG("Rabbit's Foot", 25, "RABBITS_FOOT"), XX("Apple", 1), XX("Horse Pills", 20), CK("Survivalist Map", 25),
		XX("Moonshine", 60), CK("Great Plains Map", 55) } },
	{ "Escalera General Store", true, { CK("Bandana", 20), CK("Survivalist Map", 25), XX("Bait", 15), XX("Apple", 1), XX("Snake Oil", 30), XX("Rebel Scrap", 50) } },
	{ "MacFarlane's Ranch General Store", true, { CK("Survivalist Map", 25), CK("Hennigan's Stead Map", 25), XX("Snake Oil", 30), XX("Revolver Ammo", 12),
		XX("Pistol Ammo", 15), XX("Repeater Ammo", 25), XX("Rifle Ammo", 20), XX("Shotgun Ammo", 30), XX("Sniper Rifle Ammo", 40) } },
	{ "Chuparosa General Store", true, { CK("Survivalist Map", 25), XX("Snake Oil", 30), XX("Chewing Tobacco", 35), XX("Fire Bottle", 25), CK("Perdido Map", 45),
		CK("Diez Coronas Map", 50), XX("Bandito Scrap", 50), XX("Revolver Ammo", 12), XX("Pistol Ammo", 15), XX("Repeater Ammo", 25), XX("Rifle Ammo", 20),
		XX("Shotgun Ammo", 30), XX("Sniper Rifle Ammo", 40) } },
	{ "Thieves' Landing Tailor", false, { CK("Bandana", 20), CK("Bollard Gang Scrap", 100), CK("Walton's Gang Scrap", 150),
		CK("Treasure Hunter Scrap", 250), CK("Elegant Suit", 70) } },
	{ "Blackwater Tailor", false, { CK("Bollard Gang Scrap", 100), CK("Walton's Gang Scrap", 150), XX("US Military Scrap", 200) } },
	{ "Armadillo Gunsmith", false, { CW("Volcanic Pistol", 100, "Volcanic Pistol"), CW("Double Barrel Shotgun", 150, "Double Barrel Shotgun"),
		CW("Winchester Repeater", 350, "Winchester Repeater"), CG("Bandolier", 125, "BANDOLIER"), XX("Bait", 15), XX("Buffalo Rifle", 475),
		XX("Revolver Ammo", 12), XX("Pistol Ammo", 15), XX("Repeater Ammo", 25), XX("Rifle Ammo", 20), XX("Shotgun Ammo", 30), XX("Sniper Rifle Ammo", 40) } },
	{ "Thieves' Landing Gunsmith", false, { CW("Sawed-Off Shotgun", 250, "Sawed-Off Shotgun"), CW("Springfield Rifle", 300, "Springfield Rifle"),
		CG("Bandolier", 125, "BANDOLIER"), XX("Bait", 15), XX("Buffalo Rifle", 475), XX("Revolver Ammo", 12), XX("Pistol Ammo", 15), XX("Repeater Ammo", 25),
		XX("Rifle Ammo", 20), XX("Shotgun Ammo", 30), XX("Sniper Rifle Ammo", 40) } },
	{ "Escalera Gunsmith", false, { CK("Survivalist Map", 25), XX("Bait", 15), CG("Bandolier", 125, "BANDOLIER"), XX("Fire Bottle", 25), XX("Dynamite", 45),
		CW("Semi-Auto Pistol", 300, "Semi-Auto Pistol"), CW("Bolt Action Rifle", 750, "Bolt Action Rifle"), CW("Semi-Auto Shotgun", 1000, "Semi-Auto Shotgun"),
		CW("LeMat Revolver", 1250, "LeMat Revolver"), CK("Punta Orgullo Map", 50), XX("Buffalo Rifle", 475), XX("Revolver Ammo", 12), XX("Pistol Ammo", 15),
		XX("Repeater Ammo", 25), XX("Rifle Ammo", 20), XX("Shotgun Ammo", 30), XX("Sniper Rifle Ammo", 40) } },
	{ "Blackwater Gunsmith", false, { XX("Throwing Knife", 10), CG("Bandolier", 125, "BANDOLIER"), XX("Bait", 15), XX("Fire Bottle", 25), XX("Dynamite", 45),
		CW("Mauser Pistol", 800, "Mauser Pistol"), CW("Evans Repeater", 1000, "Evans Repeater"), CW("Carcano Rifle", 1100, "Carcano Rifle"),
		XX("Explosive Rifle", 10000), XX("Buffalo Rifle", 475), XX("Revolver Ammo", 12), XX("Pistol Ammo", 15), XX("Repeater Ammo", 25), XX("Rifle Ammo", 20),
		XX("Shotgun Ammo", 30), XX("Sniper Rifle Ammo", 40) } },
};

#undef CK
#undef CW
#undef CG
#undef XX

static int g_store = -1;                      // index into STORES, -1 = no shop open
static float g_storeScale = 1.0f;             // Beta: honor changes prices (x0.5 or x1.5), found together with the store

static int Scaled(int _Price, float _Scale) { return (int)std::lround(_Price * _Scale); }
static std::map<int, int> g_storeSlot;        // item index in the store -> shop handle
static std::map<int, int> g_storeMissing;     // item index -> ticks seen as sold
static std::set<int> g_storeSeen;             // item index -> was on offer this visit

static bool IsHorseDeedPrice(int _Price)
{
	for (int base : { 100, 500, 1500, 10000 })
	{
		for (float scale : { 1.0f, 0.5f, 1.5f }) { if (_Price == Scaled(base, scale)) { return true; } }
	}

	return false;
}

// Score every store against the prices on the shelf; the best match (most of its prices present) is the open shop.
static int IdentifyStore(const int* _Price, const int* _Qty, float& _ScaleOut)
{
	std::multiset<int> shelf;

	for (int i = 0; i < SHOP_WATCH_HANDLES; i++) { if (_Price[i] > 0) { shelf.insert(_Price[i]); } }

	if (shelf.empty()) { return -1; }

	int best = -1;
	double bestScore = 0.0;

	for (float scale : { 1.0f, 0.5f, 1.5f })
	for (int s = 0; s < (int)STORES.size(); s++)
	{
		std::multiset<int> left = shelf;
		int hit = 0;

		for (const StoreItem& item : STORES[s].items)
		{
			auto it = left.find(Scaled(item.price, scale));

			if (it != left.end()) { hit++; left.erase(it); }
		}

		// general stores also carry horse deeds, which are not in the lists: do not count them against the store
		int extra = 0;

		for (int price : left) { if (!(STORES[s].general && IsHorseDeedPrice(price))) { extra++; } }

		double score = (double)hit / (double)STORES[s].items.size() - 0.05 * extra;

		if (score > bestScore + 0.001) { bestScore = score; best = s; _ScaleOut = scale; }
	}

	return bestScore >= 0.5 ? best : -1;
}

static void ShopTick2()
{
	int priceAt[SHOP_WATCH_HANDLES];
	int qtyAt[SHOP_WATCH_HANDLES];

	for (int i = 0; i < SHOP_WATCH_HANDLES; i++)
	{
		priceAt[i] = SHOP::SHOP_GET_ITEM_PURCHASE_PRICE(i);
		qtyAt[i] = SHOP::SHOP_GET_ITEM_QUANTITY(i);
	}

	float scale = 1.0f;
	int store = IdentifyStore(priceAt, qtyAt, scale);

	if (store != g_store)
	{
		g_store = store;
		g_storeScale = scale;
		g_storeSlot.clear();
		g_storeMissing.clear();
		g_storeSeen.clear();

		std::string shelf;

		for (int i = 0; i < SHOP_WATCH_HANDLES; i++) { if (priceAt[i] > 0) { shelf += " [" + std::to_string(i) + "]$" + std::to_string(priceAt[i]) + "x" + std::to_string(qtyAt[i]); } }

		const char* district = CORE::GET_DISTRICTS_NAME();

		Log(std::string("SHOP: ") + (store >= 0 ? STORES[store].store : "none") + " x" + std::to_string(scale) + " (district '" + (district ? district : "") + "') shelf:" + shelf);

		// v5.23: the Armadillo shopkeeper IS Herbert Moon: remember his real model for the Herbert Moon trap
		if (store >= 0 && std::string(STORES[store].store) == "Armadillo General Store")
		{
			Actor player = ACTOR::GET_PLAYER_ACTOR(-1);
			Vector3 me = ACTOR::GET_POSITION(player);
			int count = REDHOOK::WORLD_GET_ALL_ACTORS(g_actorBuffer);
			float best = 8.0f;
			int model = -1;

			for (int i = 0; i < count && i < MAX_ACTORS; i++)
			{
				Actor a = g_actorBuffer[i];

				if (a == player || !ENTITY::IS_ACTOR_VALID(a) || HEALTH::IS_ACTOR_DEAD(a) || AI_ANIMAL::ANIMAL_ACTOR_GET_SPECIES(a) != -1) { continue; }

				float d = Distance(ACTOR::GET_POSITION(a), me);

				if (d < best) { best = d; model = (int)OBJECT::GET_ACTOR_ENUM(a); }
			}

			if (model > 0)
			{
				std::ofstream out("rdr1_ap_herbert.txt", std::ios::trunc);

				out << model << "\n";

				Log("herbert: shopkeeper model " + std::to_string(model) + " saved (" + std::to_string(best) + " m away)");
			}
		}

		if (store >= 0)
		{
			// item -> handle: the k-th item of a price in the store list is the k-th slot of that price on the shelf
			const StoreDef& def = STORES[store];
			std::map<int, int> used;

			// v5.25: same-price items are told apart by stock: gear and maps are single items (quantity 1), consumables come in
			// stacks (x5, x10, x30). Thieves' Landing has a $35 map AND $35 chewing tobacco, and its shelf order is not the
			// guide's order (gear and maps come first), so the order alone picked the wrong slot.
			for (int k = 0; k < (int)def.items.size(); k++)
			{
				bool single = def.items[k].check;
				int price = Scaled(def.items[k].price, scale);
				int key = price * 2 + (single ? 1 : 0);
				int want = used[key]++;
				int seen = 0;

				for (int i = 0; i < SHOP_WATCH_HANDLES; i++)
				{
					if (priceAt[i] == price && ((qtyAt[i] <= 1) == single) && seen++ == want) { g_storeSlot[k] = i; break; }
				}
			}
		}
	}

	if (store < 0) { return; }

	const StoreDef& def = STORES[store];

	// horses are not part of this randomizer
	if (def.general)
	{
		std::set<int> listed;

		for (const auto& kv : g_storeSlot) { listed.insert(kv.second); }

		for (int i = 0; i < SHOP_WATCH_HANDLES; i++)
		{
			if (!listed.count(i) && IsHorseDeedPrice(priceAt[i]) && !SHOP::SHOP_IS_ITEM_BLOCKED(i)) { SHOP::SHOP_SET_ITEM_BLOCKED(i, true); }
		}
	}

	for (const auto& kv : g_storeSlot)
	{
		const StoreItem& item = def.items[kv.first];
		int handle = kv.second;

		if (!item.check) { continue; }

		std::string location = std::string("Shop - ") + item.name;
		bool sent = g_sentChecks.count(location) != 0;

		if (sent)
		{
			if (!SHOP::SHOP_IS_ITEM_BLOCKED(handle)) { SHOP::SHOP_SET_ITEM_BLOCKED(handle, true); }   // already bought somewhere

			continue;
		}

		if (SHOP::SHOP_IS_ITEM_BLOCKED(handle)) { SHOP::SHOP_SET_ITEM_BLOCKED(handle, false); }

		if (qtyAt[handle] > 0 && priceAt[handle] == Scaled(item.price, g_storeScale))
		{
			g_storeSeen.insert(kv.first);
			g_storeMissing[kv.first] = 0;
		}
		else if (g_storeSeen.count(kv.first) && item.weapon == nullptr && ++g_storeMissing[kv.first] >= 2)
		{
			// a seen slot that went to quantity 0 while the shop is open: bought (weapons are detected when the gun appears)
			Log("SHOP BOUGHT: " + location + " in " + def.store);

			SendCheck(location);

			if (item.gear != nullptr && !g_apGearGranted.count(item.gear))
			{
				g_gearRemovals.push_back({ GetTickCount64() + 1500, std::string(item.gear) });
			}
		}
	}
}

// called by the weapon watch when a gun shows up
static void ShopWeaponBought2(int _WeaponId)
{
	if (g_store < 0) { return; }

	for (const StoreItem& item : STORES[g_store].items)
	{
		if (item.weapon == nullptr) { continue; }

		int index = FindWeaponIndex(item.weapon);

		if (index < 0 || (int)TEST_WEAPONS[index].id != _WeaponId) { continue; }

		Log(std::string("SHOP BOUGHT: Shop - ") + item.name + " in " + STORES[g_store].store);

		SendCheck(std::string("Shop - ") + item.name);

		if (!g_apGranted.count(_WeaponId) && !g_vanillaWeapons)
		{
			g_shopRemovals.push_back({ GetTickCount64() + 1500, index });
			g_shopRemovals.push_back({ GetTickCount64() + 4000, index });
		}

		return;
	}
}

static void ShopRulesTick()
{
	if (!g_shopDoneLoaded)
	{
		LoadShopDone();
	}

	int priceAt[SHOP_WATCH_HANDLES];
	int qtyAt[SHOP_WATCH_HANDLES];

	bool baitSeen = false;
	bool appleSeen = false;

	for (int i = 0; i < SHOP_WATCH_HANDLES; i++)
	{
		priceAt[i] = SHOP::SHOP_GET_ITEM_PURCHASE_PRICE(i);
		qtyAt[i] = SHOP::SHOP_GET_ITEM_QUANTITY(i);

		if (priceAt[i] == 15 && qtyAt[i] == 10) { baitSeen = true; }
		if (priceAt[i] == 1 && qtyAt[i] == 5) { appleSeen = true; }
	}

	int shop = baitSeen ? 0 : (appleSeen ? 1 : -1);

	{
		// v5.22: a shop that is not recognised (no bait / apple slot) is still logged once per visit, for the other towns
		static bool s_unknownLogged = false;
		bool anySlot = false;

		for (int i = 0; i < SHOP_WATCH_HANDLES; i++) { if (priceAt[i] > 0) { anySlot = true; } }

		if (shop < 0 && anySlot && !s_unknownLogged)
		{
			const char* district = CORE::GET_DISTRICTS_NAME();
			std::string shelf = "SHOP VISIT: district='" + Lower(district ? district : "") + "' type=unknown slots:";

			for (int i = 0; i < SHOP_WATCH_HANDLES; i++)
			{
				if (priceAt[i] > 0) { shelf += " [" + std::to_string(i) + "]$" + std::to_string(priceAt[i]) + "x" + std::to_string(qtyAt[i]); }
			}

			Log(shelf);
		}

		s_unknownLogged = anySlot && shop < 0;
	}

	if (shop != g_shopActive)
	{
		const char* district = CORE::GET_DISTRICTS_NAME();

		g_shopTown = Lower(district ? district : "");

		Log("shoprules: shop " + std::to_string(shop) + " recognised (-1 = none), district '" + g_shopTown + "'");

		// v5.22: log the whole shelf of every shop visited, so the other towns' stores can be added
		if (shop >= 0)
		{
			std::string shelf = "SHOP VISIT: district='" + g_shopTown + "' type=" + (shop == 0 ? "gun" : "general") + " slots:";

			for (int i = 0; i < SHOP_WATCH_HANDLES; i++)
			{
				if (priceAt[i] > 0) { shelf += " [" + std::to_string(i) + "]$" + std::to_string(priceAt[i]) + "x" + std::to_string(qtyAt[i]); }
			}

			Log(shelf);
		}

		g_shopActive = shop;

		for (int r = 0; r < SHOP_RULE_COUNT; r++) { g_shopSeen[r] = false; g_shopMissing[r] = 0; }
	}

	if (shop < 0)
	{
		return;
	}

	bool used[SHOP_WATCH_HANDLES] = {};

	for (int r = 0; r < SHOP_RULE_COUNT; r++)
	{
		const ShopRule& rule = SHOP_RULES[r];

		if (rule.shop != shop || !RuleTownMatches(rule))
		{
			continue;
		}

		int found = -1;

		if (rule.handle >= 0)
		{
			// v5.20m: fixed slot. Present = still on offer (quantity above 0); a bought slot reads quantity 0 and counts as missing.
			if (qtyAt[rule.handle] > 0 && (priceAt[rule.handle] == rule.origPrice || priceAt[rule.handle] == rule.newPrice)) { found = rule.handle; }
		}

		// the second slot of a price collision (Survivalist Map) is found by its new price first.
		for (int i = 0; i < SHOP_WATCH_HANDLES && found < 0 && rule.handle < 0; i++)
		{
			if (!used[i] && priceAt[i] == rule.newPrice && qtyAt[i] > 0) { found = i; }
		}

		// a collided slot that has not been moved yet: take the second handle at the original price.
		if (found < 0 && rule.handle < 0 && rule.newPrice != rule.origPrice && !g_shopDone.count(r))
		{
			int seen = 0;

			for (int i = 0; i < SHOP_WATCH_HANDLES && found < 0; i++)
			{
				if (!used[i] && priceAt[i] == rule.origPrice && qtyAt[i] > 0 && ++seen == 2) { found = i; }
			}

			if (found >= 0)
			{
				SHOP::SHOP_SET_ITEM_PURCHASE_PRICE(found, rule.newPrice);

				Log(std::string("shoprules: moved ") + rule.location + " to price " + std::to_string(rule.newPrice) + " (handle " + std::to_string(found) + ")");
			}
		}

		if (found >= 0)
		{
			used[found] = true;

			g_shopMissing[r] = 0;
			g_shopSeen[r] = true;

			static int s_lastQty[SHOP_RULE_COUNT] = {};
			static int s_lastBlocked[SHOP_RULE_COUNT] = {};

			if (qtyAt[found] != s_lastQty[r] || SHOP::SHOP_IS_ITEM_BLOCKED(found) != s_lastBlocked[r])
			{
				Log(std::string("shoprules: slot '") + rule.location + "' handle " + std::to_string(found) + " qty " + std::to_string(s_lastQty[r]) + " -> " + std::to_string(qtyAt[found])
					+ " blocked " + std::to_string(s_lastBlocked[r]) + " -> " + std::to_string(SHOP::SHOP_IS_ITEM_BLOCKED(found)));

				s_lastQty[r] = qtyAt[found];
				s_lastBlocked[r] = SHOP::SHOP_IS_ITEM_BLOCKED(found);
			}

			if (g_shopDone.count(r))
			{
				// check already sent: do not offer it again
				if (!SHOP::SHOP_IS_ITEM_BLOCKED(found)) { SHOP::SHOP_SET_ITEM_BLOCKED(found, true); }
			}
			else if (SHOP::SHOP_IS_ITEM_BLOCKED(found))
			{
				SHOP::SHOP_SET_ITEM_BLOCKED(found, false);

				Log(std::string("shoprules: unblocked ") + rule.location + " (handle " + std::to_string(found) + ")");
			}
		}
		else if (g_shopSeen[r] && !g_shopDone.count(r))
		{
			// seen before, gone now, while the rest of the shop is still on screen: that is a purchase
			if (++g_shopMissing[r] >= 2)
			{
				g_shopDone.insert(r);

				std::ofstream out(SHOP_DONE_PATH, std::ios::app);

				out << r << "\n";

				SendCheck(rule.location);

				if (rule.weapon != nullptr)
				{
					int index = FindWeaponIndex(rule.weapon);

					if (index >= 0 && !g_vanillaWeapons && !g_apGranted.count((int)TEST_WEAPONS[index].id))
					{
						g_shopRemovals.push_back({ GetTickCount64() + 1500, index });
						g_shopRemovals.push_back({ GetTickCount64() + 4000, index });

						Log(std::string("shoprules: bought ") + rule.weapon + " will be removed again (not received from Archipelago)");
					}
				}
				else
				{
					// v5.20m: gear with a known item name is removed again, unless it was given by Archipelago.
					const char* gearName = nullptr;
					std::string loc = rule.location;

					if (loc.find("Bandolier") != std::string::npos) { gearName = "BANDOLIER"; }
					else if (loc.find("Rabbit's Foot") != std::string::npos) { gearName = "RABBITS_FOOT"; }

					if (gearName != nullptr && !g_apGearGranted.count(gearName))
					{
						g_gearRemovals.push_back({ GetTickCount64() + 1500, std::string(gearName) });
						Log(std::string("shoprules: bought ") + gearName + " will be removed again (not received from Archipelago)");
					}
					else
					{
						Log("shoprules: gear purchase, no known item name to remove (" + loc + ")");
					}
				}
			}
		}
	}

	GearRemovalTick();

	for (size_t k = 0; k < g_shopRemovals.size();)
	{
		if (GetTickCount64() >= g_shopRemovals[k].first)
		{
			int index = g_shopRemovals[k].second;

			if (!g_apGranted.count((int)TEST_WEAPONS[index].id))
			{
				RemoveWeaponByIndex(index);
			}

			g_shopRemovals.erase(g_shopRemovals.begin() + k);
		}
		else
		{
			k++;
		}
	}
}



// ---------------------------------------------------------------------------------------------
// Dead Eye progression (level 0 = Dead Eye disabled)
//   0 disabled   1 basic Dead Eye   2 + auto lock-on (multilock)   3 + manual target painting   4 + faster regeneration
// Level 4's regeneration bonus is an untested placeholder. -1 = not enforced (game decides).
// The plugin re-applies the level every 5 seconds in normal play (never while Dead Eye is active), because the game's scripts may change it.
// ---------------------------------------------------------------------------------------------

static int g_deadeyeLevel = -1;

static void ApplyDeadEyeLevel(bool _Log)
{
	Actor player = ACTOR::GET_PLAYER_ACTOR(-1);

	if (g_deadeyeLevel < 0)
	{
		ACTOR::SET_DISABLE_DEADEYE(player, false);
		ACTOR::SET_DEADEYE_MULTILOCK_ENABLE(player, true);
		ACTOR::SET_DEADEYE_TARGETPAINT_ENABLE(player, true);
		ACTOR::SET_DEADEYE_REGENERATION_RATE_MULTIPLIER(player, 1.0f);
	}
	else
	{
		ACTOR::SET_DISABLE_DEADEYE(player, g_deadeyeLevel == 0);
		ACTOR::SET_DEADEYE_MULTILOCK_ENABLE(player, g_deadeyeLevel >= 2);
		ACTOR::SET_DEADEYE_TARGETPAINT_ENABLE(player, g_deadeyeLevel >= 3);
		ACTOR::SET_DEADEYE_REGENERATION_RATE_MULTIPLIER(player, g_deadeyeLevel >= 4 ? 2.0f : 1.0f);
	}

	if (_Log)
	{
		Log("deadeye level applied: " + std::to_string(g_deadeyeLevel));
	}
}



// ---------------------------------------------------------------------------------------------
// DeathLink: cause detection + cowboy death messages
//   Sending side:   when John dies the log gets "player died: cause=<category> ..." and a matching message
//                   ("death message: ...") is picked. The future client sends that text with the DeathLink.
//   Receiving side: "deathlink <text>" in the input file shows <text> on screen, waits 2 seconds, then kills John.
//   Preview:        "deathmsg <category>" shows a random message (generic, shot, cougar, bear, wolf, snake, animal, lawman,
//                   explosion, environment).
// Faction 8 = lawmen was learned from the kill logs (kills of sheriffs raised the bounty). Faction 4 looked like civilians.
// ---------------------------------------------------------------------------------------------

struct DeathMessages
{
	const char* category;
	std::vector<const char*> lines;
};

static const std::vector<DeathMessages> DEATH_MESSAGES =
{
	{ "generic", { "bit the dust.", "met their maker at high noon.", "went out in a blaze of glory.", "got planted in Boot Hill.", "should have stayed in Blackwater.", "drew second and paid for it." } },
	{ "cougar", { "got chewed up by a cougar.", "mistook a cougar for a housecat." } },
	{ "bear", { "picked a fight with a bear and lost.", "became a bear's supper." } },
	{ "wolf", { "was dinner for the wolves.", "ran out of luck and out of road with a wolf pack." } },
	{ "snake", { "was outdrawn by a rattlesnake.", "stepped on the wrong snake." } },
	{ "animal", { "got mauled by the local wildlife.", "lost a fight with something furry." } },
	{ "shot", { "got shot over a misunderstanding.", "lost a quick-draw contest.", "was outgunned in the dirt.", "caught a bullet meant for somebody else." } },
	{ "lawman", { "met the business end of the law.", "got a bounty they could never pay." } },
	{ "explosion", { "found out dynamite doesn't care who throws it.", "went up in smoke." } },
	{ "environment", { "took a long fall off a short horse.", "met the ground a little too fast." } },
};

static std::string PickDeathMessage(const std::string& _Category)
{
	const DeathMessages* chosen = nullptr;

	for (const DeathMessages& group : DEATH_MESSAGES)
	{
		if (_Category == group.category)
		{
			chosen = &group;
		}
	}

	if (!chosen)
	{
		chosen = &DEATH_MESSAGES[0];
	}

	return chosen->lines[std::rand() % chosen->lines.size()];
}

static std::string DeathCategory(Actor _Player)
{
	Actor attacker = HEALTH::GET_LAST_ATTACKER(_Player);

	WeaponModel weapon = HEALTH::GET_LAST_HIT_WEAPON(_Player);

	if (weapon == WEAPON_THROWN_Dynamite || weapon == WEAPON_THROWN_FireBottle)
	{
		return "explosion";
	}

	if ((int)attacker > 0 && ENTITY::IS_ACTOR_VALID(attacker))
	{
		int species = AI_ANIMAL::ANIMAL_ACTOR_GET_SPECIES(attacker);

		if (species == SPECIES_COUGAR) { return "cougar"; }
		if (species == SPECIES_BEAR) { return "bear"; }
		if (species == SPECIES_WOLF) { return "wolf"; }
		if (species == SPECIES_SNAKE) { return "snake"; }
		if (species != -1 && species != SPECIES_HUMAN) { return "animal"; }

		if (FACTION::GET_ACTOR_FACTION(attacker) == 8)
		{
			return "lawman";
		}

		// v5.14: a non-lawman person who killed John with any gun (the rancher with the revolver case).
		if (IsGun(weapon))
		{
			return "shot";
		}

		return "generic";
	}

	return "environment";
}

static bool g_deathFromLink = false;   // v5.21: a death caused by a received DeathLink is not sent back out (echo suppression)

static void DeathWatch()
{
	static bool wasDead = false;

	Actor player = ACTOR::GET_PLAYER_ACTOR(-1);

	bool dead = HEALTH::IS_ACTOR_DEAD(player);

	if (dead && !wasDead)
	{
		std::string category = DeathCategory(player);

		Log("player died: cause=" + category + " last_attacker=" + std::to_string((int)HEALTH::GET_LAST_ATTACKER(player))
			+ " last_weapon=" + std::to_string((int)HEALTH::GET_LAST_HIT_WEAPON(player)));

		std::string message = PickDeathMessage(category);

		Log("death message: " + message);

		// v5.21: the client sends every line of rdr1_ap_deaths.txt as a DeathLink (when DeathLink is on), then deletes the file.
		if (g_deathFromLink)
		{
			Log("death came from a DeathLink: not reported");
		}
		else
		{
			std::ofstream out("rdr1_ap_deaths.txt", std::ios::app);

			out << message << "\n";
		}

		g_deathFromLink = false;
	}

	wasDead = dead;
}

static void DeathLinkReceive(const std::string& _Text)
{
	Log("deathlink received: " + _Text);

	ApBig("DeathLink", _Text);

	g_deathFromLink = true;

	ULONGLONG until = GetTickCount64() + 2000;

	while (GetTickCount64() < until)
	{
		ScriptWait(0);
	}

	HEALTH::KILL_ACTOR(ACTOR::GET_PLAYER_ACTOR(-1));
}



// ---------------------------------------------------------------------------------------------
// Console probe. The F8 console is probably the game's own debug console, and these natives read what is typed
// into it. Nothing is dereferenced: tokens are only logged as raw numbers, so type a few words
// (for example "ap test 5") into the F8 console, press Enter, close it, and send me the "console command:" lines.
// Also: "consolelog <text>" in the input file prints <text> into the console, and "consolemirror 1" mirrors the whole log there.
// ---------------------------------------------------------------------------------------------

static void ConsoleProbe()
{
	int latest = BUILTIN::GET_LATEST_CONSOLE_COMMAND();
	int tokens = BUILTIN::GET_NUM_CONSOLE_COMMAND_TOKENS();

	// "latest" is a constant (2032490960) while nothing has been typed, so only tokens count.
	if (tokens <= 0)
	{
		return;
	}

	std::string line = "console command: latest=" + std::to_string(latest) + " tokens=" + std::to_string(tokens);

	for (int i = 0; i < tokens && i < 8; i++)
	{
		int value = BUILTIN::GET_CONSOLE_COMMAND_TOKEN(i);

		char hex[16];

		std::snprintf(hex, sizeof(hex), "%08X", (unsigned int)value);

		line += " [" + std::to_string(i) + "]=" + std::to_string(value) + "(0x" + hex + ")";
	}

	Log(line);

	BUILTIN::RESET_LATEST_CONSOLE_COMMAND();
}




// =============================================================================================
// v5.21 ALPHA: Archipelago core
//
// Files (game folder):
//   rdr1_ap_hashmap.txt    "hash script"  objective/label hash -> script that owns the string (generated from the decompiled
//                          scripts; only strings that exist in exactly one script). Tells us which mission/stranger is running.
//   rdr1_ap_scriptmap.txt  "script|kind|location|cluster or part|final|confidence"  script -> location name. Editable.
//   rdr1_ap_items.txt      written by the client, the WHOLE list every time:  "seed <id>", "option <key> <value>",
//                          then "<index>|<item name>|<sender>" for every received item. Read once a second; only items with
//                          an index above the last one applied are applied, so fillers and traps never repeat.
//   rdr1_ap_outbox.txt     one location name per line (SendCheck); "DEATH <text>" lines for DeathLink are written separately.
//
// Mission completion = the game's own "missions completed" stat (sn_miss) goes up (it does on a pass, also on a replay, never on a
// fail: tested). The mission is the one whose objectives were seen last. Stranger parts: sn_rcmc ("stranger missions completed")
// goes up; the arc's check is sent when its LAST part completes. Story missions: the check is sent the moment the mission is done
// (beta 2: no mission items, nothing is held back).
// =============================================================================================
struct ScriptInfo
{
	std::string kind;       // mission | stranger
	std::string location;   // Archipelago location name
	std::string group;      // mission: cluster, stranger: part number
	bool final = false;     // stranger: last part of the arc
};

static std::map<unsigned int, std::string> g_hashScript;
static std::map<std::string, ScriptInfo> g_scriptInfo;
static std::map<std::string, std::string> g_clusterMissions;   // v5.24: cluster -> its missions in story order (for the popup)
static bool g_mapsLoaded = false;

static void LoadApMaps()
{
	g_mapsLoaded = true;

	{
		std::ifstream in("rdr1_ap_hashmap.txt");
		unsigned long long hash;
		std::string script;

		while (in >> hash >> script) { g_hashScript[(unsigned int)hash] = script; }
	}

	{
		std::ifstream in("rdr1_ap_scriptmap.txt");
		std::string line;

		while (std::getline(in, line))
		{
			line = Trim(line);

			if (line.empty() || line[0] == '#') { continue; }

			std::vector<std::string> f;
			size_t pos = 0;

			while (true)
			{
				size_t bar = line.find('|', pos);

				f.push_back(Trim(line.substr(pos, bar == std::string::npos ? std::string::npos : bar - pos)));

				if (bar == std::string::npos) { break; }

				pos = bar + 1;
			}

			if (f.size() >= 3 && f[0] == "@cluster")
			{
				std::string list = f[2];

				for (size_t pos = list.find(';'); pos != std::string::npos; pos = list.find(';', pos)) { list.replace(pos, 1, ", "); }

				g_clusterMissions[f[1]] = list;

				continue;
			}

			if (f.size() < 5) { continue; }

			ScriptInfo info;
			info.kind = f[1];
			info.location = f[2];
			info.group = f[3];
			info.final = f[4] == "1";

			g_scriptInfo[f[0]] = info;
		}
	}

	Log("ap maps: " + std::to_string(g_hashScript.size()) + " hashes, " + std::to_string(g_scriptInfo.size()) + " scripts");

	if (g_hashScript.empty() || g_scriptInfo.empty())
	{
		ApSay(Tag("red", "Mission data missing") + "\nrdr1_ap_hashmap.txt / rdr1_ap_scriptmap.txt not found in the game folder.");
	}
}

// ---------- AP mode, options, clusters ----------

// v6.1 BETA 2: story missions are no longer items. Missions play in the vanilla order and every finished mission sends its check
// at once, so nothing is ever "locked" or held back. (Cluster items from older seeds are still understood but change nothing.)
static bool ClusterReceived(const std::string& _Cluster)
{
	(void)_Cluster;
	return true;
}

static void FlushPendingChecks(const std::string& _Cluster)
{
	for (const auto& kv : g_scriptInfo)
	{
		const ScriptInfo& info = kv.second;

		if (info.kind == "mission" && info.group == _Cluster && ProgressGet("pending:" + info.location) > 0)
		{
			ProgressSet("pending:" + info.location, 0);

			Log("pending check released: " + info.location);

			SendCheck(info.location);
		}
	}
}

// ---------- mission / stranger tracker ----------
static std::string g_lastStrangerScript;
static int g_trackerObjective = 0;
static long long g_prevMiss = -1;
static long long g_prevRcmc = -1;
static ULONGLONG g_trackerStableAt = 0;
static bool g_trackerAfterLoad = false;

static void MissionCompleted()
{
	if (g_lastMissionScript.empty())
	{
		Log("MISSION DONE: no mission objective was seen (unmapped mission). Use 'sendcheck <name>' if a check is missing.");

		ApSay(Tag("red", "Mission finished, but it could not be identified") + "\nTell the developer; the log has the details.");

		return;
	}

	auto it = g_scriptInfo.find(g_lastMissionScript);

	if (it == g_scriptInfo.end() || it->second.kind != "mission")
	{
		Log("MISSION DONE: script " + g_lastMissionScript + " is not in rdr1_ap_scriptmap.txt");

		ApSay(Tag("red", "Unmapped mission finished") + "\nScript " + g_lastMissionScript + " (see log)");

		g_lastMissionScript.clear();

		return;
	}

	const ScriptInfo& info = it->second;

	Log("MISSION DONE: " + info.location + " (script " + g_lastMissionScript + ", cluster " + info.group + ")");

	if (!ClusterReceived(info.group))
	{
		ProgressSet("pending:" + info.location, 1);

		ApSay(Tag("red", "Mission locked") + "\n" + info.location + "\nThe check is saved and sent when you receive " + Tag("blue", info.group) + ".");
	}
	else
	{
		SendCheck(info.location);
	}

	g_lastMissionScript.clear();
}

static void StrangerPartCompleted()
{
	auto it = g_scriptInfo.find(g_lastStrangerScript);

	if (g_lastStrangerScript.empty() || it == g_scriptInfo.end())
	{
		Log("STRANGER DONE: unknown stranger script '" + g_lastStrangerScript + "'");

		return;
	}

	const ScriptInfo& info = it->second;

	Log("STRANGER DONE: " + info.location + " part " + info.group + (info.final ? " (last part)" : "") + " (script " + g_lastStrangerScript + ")");

	if (info.final)
	{
		SendCheck(info.location);

		if (info.location == "Remember My Family") { std::ofstream out(OUTBOX_PATH, std::ios::app); out << "GOAL: Remember My Family\n"; }
	}
	else
	{
		ApSay(Tag("green", "Stranger progress") + "\n" + info.location + " (part " + info.group + ")");
	}

	g_lastStrangerScript.clear();
}

static void MissionTrackerTick(ULONGLONG _Now)
{
	if (!g_mapsLoaded) { LoadApMaps(); }

	if (GAME::GET_GAME_STATE() != 3)
	{
		g_trackerStableAt = _Now + 4000;   // stats are reloaded during a load: compare again once it settles
		g_trackerAfterLoad = true;

		return;
	}

	if (UI::UI_ISACTIVE("MissionFailedMenu") && !g_lastMissionScript.empty())
	{
		Log("tracker: mission failed, back to free roam (" + g_lastMissionScript + ")");

		g_lastMissionScript.clear();
	}

	int objective = JOURNAL::GET_LAST_NOTE_OBJECTIVE();

	if (objective != g_trackerObjective)
	{
		g_trackerObjective = objective;

		auto h = g_hashScript.find((unsigned int)objective);

		if (h != g_hashScript.end())
		{
			auto info = g_scriptInfo.find(h->second);

			if (info != g_scriptInfo.end() && info->second.kind == "stranger")
			{
				if (g_lastStrangerScript != h->second) { Log("tracker: stranger " + info->second.location + " part " + info->second.group + " (" + h->second + ")"); }

				g_lastStrangerScript = h->second;
			}
			else
			{
				if (g_lastMissionScript != h->second)
				{
					Log("tracker: mission " + (info != g_scriptInfo.end() ? info->second.location : std::string("?")) + " (" + h->second + ")");

					if (info != g_scriptInfo.end() && !ClusterReceived(info->second.group))
					{
						ApSay(Tag("red", "This mission is locked") + "\n" + info->second.location + " needs " + Tag("blue", info->second.group)
							+ ".\nYou can play it, but the check waits until you receive it.");
					}
				}

				g_lastMissionScript = h->second;
			}
		}
	}

	long long miss = (long long)std::lround(StatGet(9));
	long long rcmc = (long long)std::lround(StatGet(407));

	if (g_prevMiss < 0)
	{
		if (_Now < g_trackerStableAt) { return; }

		g_prevMiss = miss;
		g_prevRcmc = rcmc;

		return;
	}

	// v6.1 beta 2: do not throw the old counts away across a load. A mission can end INSIDE a load screen (Exodus in America:
	// sn_miss went 0 -> 1 while the game state was 5), and starting over after the load lost that check. After a load the counts
	// are compared once it settles: exactly +1 with a tracked mission is that mission; any other jump is a different save.
	if (_Now < g_trackerStableAt) { return; }

	bool afterLoad = g_trackerAfterLoad;
	g_trackerAfterLoad = false;

	if (miss > g_prevMiss)
	{
		if (!afterLoad || (miss == g_prevMiss + 1 && !g_lastMissionScript.empty())) { MissionCompleted(); }
		else { Log("tracker: missions completed went " + std::to_string(g_prevMiss) + " -> " + std::to_string(miss) + " across a load (another save): not counted"); }
	}

	if (rcmc > g_prevRcmc)
	{
		if (!afterLoad || (rcmc == g_prevRcmc + 1 && !g_lastStrangerScript.empty())) { StrangerPartCompleted(); }
		else { Log("tracker: stranger parts went " + std::to_string(g_prevRcmc) + " -> " + std::to_string(rcmc) + " across a load (another save): not counted"); }
	}

	g_prevMiss = miss;
	g_prevRcmc = rcmc;
}

// ---------- received items ----------
static const char* ITEMS_PATH = "rdr1_ap_items.txt";

// v5.22 Herbert Moon trap: an invisible Herbert Moon (the only model of him in the actor list is the Undead Nightmare one, id 1283)
// stands right behind John and talks: shopkeeper chatter contexts from shopkeeper_brain / rdr2init (SHOP_BROWSE_CHATTER etc.),
// spoken with SAY_SINGLE_LINE_STRING like the scripts do. He follows John for 15 s, then he is removed.
static Actor g_herbertActor = 0;
static ULONGLONG g_herbertNextLine = 0;
static ULONGLONG g_herbertUntil = 0;
static int g_herbertLines = 0;

static void HerbertSpeak()
{
	static const char* LINES[] = { "SHOP_BROWSE_CHATTER", "SHOP_ENTER", "SHOP_TRANSACTION", "SHOP_END_TRANSACTION", "SHOP_BROWSE_CHATTER" };

	const char* line = LINES[std::rand() % 5];

	Invoke<0xBA734A15, void>(g_herbertActor, line, 1, 1, 0, 0, 1, 0);   // AUDIO::SAY_SINGLE_LINE_STRING

	Log(std::string("herbert: says ") + line);
}

static void HerbertCommand()
{
	ActorModel model = (ActorModel)1283;   // ACTOR_UN_MISC_HerbertMoon, unless the real one was learned in his store

	{
		std::ifstream in("rdr1_ap_herbert.txt");
		int learned = 0;

		if (in >> learned && learned > 0) { model = (ActorModel)learned; }
	}

	STREAM::STREAMING_REQUEST_ACTOR(model, true, true);

	ULONGLONG start = GetTickCount64();

	while (!STREAM::STREAMING_IS_ACTOR_LOADED(model, 0) && GetTickCount64() < start + 3000) { ScriptWait(0); }

	Actor player = ACTOR::GET_PLAYER_ACTOR(-1);
	Vector3 position = ACTOR::GET_POSITION(player);

	if (!STREAM::STREAMING_IS_ACTOR_LOADED(model, 0))
	{
		Log("herbert: model did not load");

		ApBig("Herbert Moon", "Herbert Moon speaks sweet nothings into your ear... (he could not make it)");

		return;
	}

	Layout layout = OBJECT::FIND_NAMED_LAYOUT("PlayerLayout");
	std::string name = "ap_herbert_" + std::to_string(g_spawnCounter++);

	g_herbertActor = OBJECT::CREATE_ACTOR_IN_LAYOUT(layout, name.c_str(), model, Vector2(position.x + 1.0f, position.y + 1.0f), position.z + 0.5f, Vector2(), 0.0f);

	if ((int)g_herbertActor <= 0) { Log("herbert: spawn failed"); g_herbertActor = 0; return; }

	ACTORDRAW::SET_DRAW_ACTOR(g_herbertActor, false);
	ACTOR::SET_ACTOR_INVULNERABILITY(g_herbertActor, true);

	g_herbertLines = 0;
	g_herbertNextLine = GetTickCount64() + 300;
	g_herbertUntil = GetTickCount64() + 15000;

	ApBig("Herbert Moon", "Herbert Moon speaks sweet nothings into your ear...");

	Log("herbert: spawned " + std::to_string((int)g_herbertActor));
}

static void HerbertTick(ULONGLONG _Now)
{
	if (g_herbertActor == 0) { return; }

	if (_Now >= g_herbertUntil || !ENTITY::IS_ACTOR_VALID(g_herbertActor))
	{
		if (ENTITY::IS_ACTOR_VALID(g_herbertActor)) { OBJECT::DESTROY_ACTOR(g_herbertActor); }

		g_herbertActor = 0;

		Log("herbert: gone");

		return;
	}

	// keep him at John's shoulder
	Vector3 p = ACTOR::GET_POSITION(ACTOR::GET_PLAYER_ACTOR(-1));
	Vector3 target = p;

	target.x += 0.8f;
	target.y += 0.8f;

	ACTOR::TELEPORT_ACTOR(g_herbertActor, &target, false, false, false);

	if (_Now >= g_herbertNextLine && g_herbertLines < 4)
	{
		HerbertSpeak();

		g_herbertLines++;
		g_herbertNextLine = _Now + 3500;
	}
}

static std::string AnimalKey(const std::string& _Animal)
{
	std::string a = Lower(_Animal);

	if (a == "grizzly bear") { return "grizzly"; }
	if (a == "black bear") { return "bear"; }

	return a;
}

// v5.22: a trap first shows a warning, then fires 2-4 s later so it can be dodged or fought.
struct PendingTrap
{
	std::string command;
	std::string title;     // big warning text
	std::string text;      // help line under it
};

static std::deque<PendingTrap> g_trapQueue2;
static int g_itemsThisTick = 0;   // flood control: a big batch (first connect) gets one summary line instead of 30 popups

static void QueueTrap(const std::string& _Command, const std::string& _Title, const std::string& _Text)
{
	g_trapQueue2.push_back({ _Command, _Title, _Text });
}

static void Announce(const std::string& _Title, const std::string& _Text, bool _Big)
{
	if (g_itemsThisTick > 3) { return; }

	(void)_Big;   // Beta: every received item uses the big pop-up only

	ApBig(_Title, _Text);
}

// v6.1 beta 2: Fame / Honor / Cash fillers survive a checkpoint reload. The game restores the stats of its last checkpoint when a
// mission fails or the player reloads, which silently took back an Honor +20 that arrived after that checkpoint (seen in test:
// honor 0 -> 20 from the item, a failed mission, then 20 -> 0 on the reload). Every grant since the last load is remembered; when
// a load finishes and the stat is below what it was right after the grant, the grants are given again.
struct StatGrant { int id; float amount; float after; };
static std::vector<StatGrant> g_grantsSinceLoad;

static void GrantStat(int _Id, float _Amount)
{
	if (_Id == 0) { CashAddCommand((int)_Amount); }
	else { StatChangeCommand("add", std::string(_Id == 1 ? "honor " : "fame ") + std::to_string((int)_Amount)); }

	g_grantsSinceLoad.push_back({ _Id, _Amount, StatGet(_Id) });
}

static void StatGrantReloadTick()
{
	static int s_prevState = -1;
	static ULONGLONG s_checkAt = 0;

	int state = GAME::GET_GAME_STATE();
	ULONGLONG now = GetTickCount64();

	if (s_prevState == 5 && state == 3) { s_checkAt = now + 4000; }

	s_prevState = state;

	if (s_checkAt == 0 || now < s_checkAt || state != 3) { return; }

	s_checkAt = 0;

	std::vector<StatGrant> grants;
	grants.swap(g_grantsSinceLoad);   // the list starts over at every load

	for (int id : { 0, 1, 3 })
	{
		float lastAfter = 0.0f, total = 0.0f;
		bool any = false;

		for (const StatGrant& g : grants) { if (g.id == id) { lastAfter = g.after; total += g.amount; any = true; } }

		if (!any) { continue; }

		float now_value = StatGet(id);

		if (now_value < lastAfter - 0.5f)
		{
			Log("stat grants: load took back AP " + std::string(id == 0 ? "cash" : (id == 1 ? "honor" : "fame")) + " (now "
				+ std::to_string(now_value) + ", was " + std::to_string(lastAfter) + " after the grant): giving " + std::to_string(total) + " again");

			GrantStat(id, total);
		}
	}
}

static void ApplyApItem(const std::string& _Item, const std::string& _Sender)
{
	std::string from = _Sender.empty() ? std::string() : "from " + _Sender;
	Actor player = ACTOR::GET_PLAYER_ACTOR(-1);

	g_itemsThisTick++;

	int weapon = -1;

	for (int i = 0; i < TEST_WEAPON_COUNT; i++) { if (_Item == TEST_WEAPONS[i].name) { weapon = i; } }

	if (weapon >= 0)
	{
		ReceivedCommand(std::string("add ") + TEST_WEAPONS[weapon].name);

		Announce(_Item, from, true);

		return;
	}

	if (_Item == "Progressive Dead Eye")
	{
		long long level = (std::min)(4LL, ProgressGet("deadeye") + 1);

		ProgressSet("deadeye", level);

		g_deadeyeLevel = (int)level;

		ApplyDeadEyeLevel(true);

		static const char* NAMES[] = { "", "Dead Eye", "Dead Eye: Auto Lock-On", "Dead Eye: Target Painting", "Dead Eye: Fast Recovery" };

		Announce(NAMES[level], from, true);

		return;
	}

	for (const auto& kv : g_scriptInfo)
	{
		if ((kv.second.kind == "mission" && kv.second.group == _Item) || (g_clusterMissions.count(_Item) && kv.first == g_scriptInfo.begin()->first))
		{
			ProgressSet("cluster:" + _Item, 1);

			// v5.24: list the missions in the cluster
			std::string missions = g_clusterMissions.count(_Item) ? g_clusterMissions[_Item] : std::string();

			Announce("Missions Unlocked: " + _Item, missions + " (" + from + ")", true);

			FlushPendingChecks(_Item);

			return;
		}
	}

	if (_Item.rfind("Outfit: ", 0) == 0)
	{
		static const std::map<std::string, std::string> OUTFITS = {
			{ "Gentleman's Attire", "social" }, { "Deadly Assassin Outfit", "gun" }, { "Elegant Suit", "suit" }, { "Walton's Gang Outfit", "dnd" },
			{ "Bollard Twins Outfit", "rustler" }, { "Treasure Hunter Outfit", "treasur" }, { "Bandito Outfit", "bandito" },
			{ "Reyes' Rebels Outfit", "rebel" }, { "U.S. Army Uniform", "military" }, { "U.S. Marshal Uniform", "uslaw" },
			{ "Legend of the West", "legend" }, { "Bureau Uniform", "fbi" }, { "Duster Coat", "duster" }, { "Mexican Poncho", "poncho" },
			{ "Rancher Outfit", "rancher" }, { "Expert Hunter Outfit", "mount" }, { "Savvy Merchant Outfit", "bruise" },
		};

		auto it = OUTFITS.find(_Item.substr(8));

		// outfits are saved by the game itself, and re-applied from the progress file after a load (see ApplySavedOutfits)
		if (it != OUTFITS.end()) { OutfitGive(it->second); ProgressSet("outfit:" + it->second, 1); }

		Announce(_Item.substr(8), from, true);

		return;
	}

	if (_Item == "Max Ammo") { MaxAmmo(); Announce("Max Ammo", from, false); return; }

	if (_Item == "Dead Eye Refill")
	{
		ACTOR::SET_PLAYER_DEADEYE_POINTS(player, 1000.0f, true);

		Announce("Dead Eye Refill", from, false);

		return;
	}

	if (_Item == "Fame +20") { GrantStat(3, 20.0f); ApBig("Fame +20", from); return; }
	if (_Item == "Honor +20") { GrantStat(1, 20.0f); ApBig("Honor +20", from); return; }

	if (_Item.size() > 6 && _Item[0] == '$' && _Item.find(" Cash") != std::string::npos)
	{
		GrantStat(0, (float)std::atoi(_Item.c_str() + 1));   // the game's own cash popup

		return;
	}

	// traps
	if (_Item.size() > 12 && _Item.compare(_Item.size() - 12, 12, " Ambush Trap") == 0)
	{
		std::string animal = _Item.substr(0, _Item.size() - 12);

		QueueTrap(animal == "Random Animal" ? "spawn" : "spawn " + AnimalKey(animal), "Something Is Stalking You", "You hear something in the brush...");

		return;
	}

	if (_Item == "Gang Attack Trap") { QueueTrap("gangattack", "Riders On The Horizon", "Outlaws are coming for you!"); return; }
	if (_Item == "Lawmen Posse Trap") { QueueTrap("posse", "The Law Is Coming", "A posse is riding your way!"); return; }
	if (_Item == "Dead Eye Drain Trap") { QueueTrap("deadeyedrain", "", ""); return; }   // one alert only ("Dead Eye Drained")
	if (_Item == "Drunk Trap") { QueueTrap("drunk 30", "Somebody Spiked Your Drink", "The world starts to spin..."); return; }
	if (_Item == "Slow Motion Trap") { QueueTrap("slowmo 8", "Time Slows Down", "Everything feels heavy..."); return; }
	if (_Item == "Stampede Trap") { QueueTrap("stampede", "The Ground Is Shaking", "Something big is coming!"); return; }
	if (_Item == "Out of Bullets Trap") { QueueTrap("outofbullets", "Your Gunbelt Feels Light", "Check your ammo..."); return; }
	if (_Item == "Herbert Moon Trap") { QueueTrap("herbert", "Herbert Moon", "Herbert Moon speaks sweet nothings into your ear..."); return; }

	Log("ap item: unknown item '" + _Item + "'");

	Announce(_Item, from, false);
}

static void ApModeChanged()
{
	g_syncOn = g_apMode && !g_vanillaWeapons;
	g_shopRules = g_apMode;

	if (g_syncOn)
	{
		// first connection mid-game: take away the guns that were not received (and give the received ones) once it is safe
		g_syncAt = GetTickCount64() + 3000;
		g_syncGiveUp = g_syncAt + 90000;
	}

	if (g_apMode)
	{
		g_deadeyeLevel = (int)ProgressGet("deadeye");

		ApplyDeadEyeLevel(true);
	}

	Log(std::string("ap mode: ") + (g_apMode ? "on" : "off") + " sync=" + std::to_string(g_syncOn) + " shoprules=" + std::to_string(g_shopRules)
		+ " vanilla_weapons=" + std::to_string(g_vanillaWeapons) + " deadeye=" + std::to_string(g_deadeyeLevel));
}

// Switches all per-run files to rdr1_ap_seeds\<seed>\ and reloads them. Returns true when the seed changed.
static bool UseSeed(const std::string& _Seed)
{
	std::string clean;

	for (char c : _Seed) { clean += (std::isalnum((unsigned char)c) || c == '-' || c == '_') ? c : '_'; }

	if (clean.empty()) { return false; }

	std::string dir = std::string("rdr1_ap_seeds\\") + clean;

	if (dir == g_seedDir) { return false; }

	std::error_code ec;

	std::filesystem::create_directories(dir, ec);

	bool fresh = !std::filesystem::exists(dir + "\\rdr1_ap_progress.txt");

	g_seedDir = dir;

	{ std::ofstream last("rdr1_ap_lastseed.txt", std::ios::trunc); last << clean << "\n"; }

	// everything that belongs to a run is reloaded from the new folder
	g_progress.clear();
	g_progressLoaded = false;
	g_gangKillsLoaded = false;
	for (int g = 0; g < GANG_COUNT; g++) { g_gangKills[g] = 0; }
	g_sentChecks.clear();
	g_sentLoaded = false;
	g_received.clear();
	g_receivedLoaded = false;
	g_shopDone.clear();
	g_shopDoneLoaded = false;
	g_prevMiss = -1;
	g_prevRcmc = -1;

	Log("seed folder: " + dir + (fresh ? " (new run)" : " (continuing)") + ", error=" + std::to_string(ec.value()));

	if (fresh) { ApBig("Archipelago", "Connected to a new multiworld"); }
	else { ApSay("Archipelago run resumed"); }

	return true;
}

static void ApItemsTick()
{
	std::ifstream in(ITEMS_PATH);

	if (!in) { return; }

	std::string line;
	std::vector<std::string> lines;

	while (std::getline(in, line)) { line = Trim(line); if (!line.empty()) { lines.push_back(line); } }

	in.close();

	long long applied = ProgressGet("ap_index");
	long long appliedBefore = applied;
	bool modeBefore = g_apMode;

	g_itemsThisTick = 0;
	bool vanillaBefore = g_vanillaWeapons;

	for (const std::string& l : lines)
	{
		if (l.rfind("seed ", 0) == 0)
		{
			std::string seed = Trim(l.substr(5));

			if (UseSeed(seed)) { applied = ProgressGet("ap_index"); }

			g_apMode = true;

			continue;
		}

		if (l.rfind("option ", 0) == 0)
		{
			std::string rest = Trim(l.substr(7));

			if (rest == "weapon_mode vanilla") { g_vanillaWeapons = true; }
			else if (rest.rfind("weapon_mode ", 0) == 0) { g_vanillaWeapons = false; }

			if (ProgressGet("opt_vanilla") != (g_vanillaWeapons ? 1 : 0)) { ProgressSet("opt_vanilla", g_vanillaWeapons ? 1 : 0); }

			continue;
		}

		size_t a = l.find('|');

		if (a == std::string::npos) { continue; }

		long long index = std::atoll(l.substr(0, a).c_str());
		size_t b = l.find('|', a + 1);
		std::string item = Trim(l.substr(a + 1, b == std::string::npos ? std::string::npos : b - a - 1));
		std::string sender = b == std::string::npos ? "" : Trim(l.substr(b + 1));

		if (index <= applied) { continue; }

		Log("ap item #" + std::to_string(index) + ": " + item + (sender.empty() ? "" : " from " + sender));

		ApplyApItem(item, sender);

		applied = index;

		ProgressSet("ap_index", applied);
	}

	if (g_itemsThisTick > 3) { ApBig("Archipelago", std::to_string(applied - appliedBefore) + " items received"); }

	if (g_apMode != modeBefore || g_vanillaWeapons != vanillaBefore) { ApModeChanged(); }
}

static void TrapQueueTick(ULONGLONG _Now)
{
	static ULONGLONG s_next = 0;

	if (g_trapQueue2.empty() || _Now < s_next) { return; }

	if (!SafeToDeliver() || !ACTOR::IS_PLAYER_CONTROLLABLE(-1)) { return; }

	PendingTrap trap = g_trapQueue2.front();

	g_trapQueue2.pop_front();

	ULONGLONG delay = 3000 + (ULONGLONG)(std::rand() % 2001);   // Beta: warning first, the trap 3-5 s later

	s_next = _Now + 10000;   // a burst of traps is spread out

	if (trap.title.empty()) { delay = 0; }   // traps that announce themselves (Dead Eye drain)

	Log("trap warning: " + trap.title + ", '" + trap.command + "' fires in " + std::to_string(delay) + " ms");

	if (!trap.title.empty()) { ApBig(trap.title, trap.text); }

	g_delayed.push_back({ _Now + delay, trap.command });
}


// ---------------------------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------------------------

void Application::Initialize(HMODULE _Module)
{
	ScriptRegister(_Module, []
	{
		Log("script v6.1 beta 2 started");

			LoadMissionTable();

		LoadApMaps();

		{
			// v5.22: resume the last multiworld (offline play still follows its rules and records its checks)
			std::ifstream last("rdr1_ap_lastseed.txt");
			std::string seed;

			if (std::getline(last, seed) && !Trim(seed).empty())
			{
				UseSeed(Trim(seed));

				g_apMode = true;
				g_vanillaWeapons = ProgressGet("opt_vanilla") > 0;

				ApModeChanged();
			}
		}

		ApSay(Tag("red", "Red Dead Redemption") + " randomizer beta 2 loaded");

		ULONGLONG nextBeat = 0;
		ULONGLONG nextPoll = 0;
		ULONGLONG nextWatch = 0;
		ULONGLONG nextActorScan = 0;
			ULONGLONG nextCleanup = 0;
			ULONGLONG nextDeadeye = 0;
			ULONGLONG nextPerfLog = 0;
			ULONGLONG nextWeaponWatch = 0;
			ULONGLONG nextShopWatch = 0;
		bool f7 = false, statKeyA = false, statKeyB = false, f9 = false, f10 = false, f11 = false;

		while (true)
		{
			ULONGLONG now = GetTickCount64();

			if (now >= nextBeat)
			{
				Log("heartbeat");

				nextBeat = now + 30000;
			}

			// Watch game state for anything that looks like mission start/end
			if (now >= nextWatch)
			{
				nextWatch = now + 250;

				WatchValue("game_state", GAME::GET_GAME_STATE());
				WatchValue("hud_showing_objective", HUD::HUD_IS_SHOWING_OBJECTIVE());
				WatchValue("minigame_running", MINIGAME::IS_MINIGAME_RUNNING() ? 1 : 0);
				WatchValue("stat_missions", (int)StatGet(9));   // v5.20m: sn_miss
				WatchValue("stat_failed", (int)StatGet(10));
				WatchValue("stat_money", (int)StatGet(0));
				WatchValue("stat_btin", (int)StatGet(408));
				EncounterTick();
				MissionTrackerTick(now);
				StatChecksTick(now);
				WatchValue("stat_btcm", (int)StatGet(409));
				WatchValue("stat_honor", (int)StatGet(1));
				WatchValue("stat_fame", (int)StatGet(3));
				WatchValue("stat_fame_rank", (int)StatGet(4));
				WatchValue("stat_honor_rank", (int)StatGet(2));
				WatchValue("stat_bounty", (int)StatGet(223));
				WatchValue("stat_kills_law", (int)StatGet(29));
				WatchValue("mission_failed_menu", UI::UI_ISACTIVE("MissionFailedMenu") ? 1 : 0);   // v5.20k: the scripts use this layer for "mission failed"

				// v5.20k: opt-in probe ("mgprobe 1") of the two unnamed MINIGAME natives and the duel score while a minigame runs.
				if (g_mgProbe && MINIGAME::IS_MINIGAME_RUNNING())
				{
					WatchValue("mg_6AAD0420", MINIGAME::_0x6AAD0420(0, 0, 0, 0));
					WatchValue("mg_655D350B", MINIGAME::_0x655D350B(0, 0, 0, 0));
					WatchValue("mg_duel_score", ENTITY::GET_CURRENT_DUEL_SCORE(0));
				}
				WatchValue("targeted_journal_entry", JOURNAL::GET_TARGETED_JOURNAL_ENTRY());
				WatchValue("stat_message_showing", HUD::IS_STAT_MESSAGE_SHOWING());
				WatchValue("cutscene_playing", CUTSCENE::CUTSCENE_MANAGER_IS_CUTSCENE_PLAYING() ? 1 : 0);
				WatchValue("player_controllable_0", ACTOR::IS_PLAYER_CONTROLLABLE(0));
				WatchValue("player_controllable_m1", ACTOR::IS_PLAYER_CONTROLLABLE(-1));
				WatchMissionSignals(now);
				ConsoleProbe();
				WatchValue("last_printed_objective", HUD::GET_LAST_PRINTED_OBJECTIVE());
				WatchValue("last_note_objective", JOURNAL::GET_LAST_NOTE_OBJECTIVE());
				{
					int targeted = JOURNAL::GET_TARGETED_JOURNAL_ENTRY();
					if (targeted != 0)
					{
						WatchValue("targeted_entry_progress", JOURNAL::GET_JOURNAL_ENTRY_PROGRESS(targeted));
						WatchValue("targeted_entry_updated", JOURNAL::IS_JOURNAL_ENTRY_UPDATED(targeted));
					}
				}
			}

			{
				// v5.14: remember when the player last fired (sampled every frame; logged at most once a second).
				static bool wasShooting = false;
				static ULONGLONG lastShotLog = 0;

				bool shooting = ENTITY::IS_ACTOR_SHOOTING(ACTOR::GET_PLAYER_ACTOR(-1));

				if (shooting)
				{
					g_lastShotMs = now;
				}

				if (shooting && !wasShooting && now - lastShotLog >= 1000)
				{
					lastShotLog = now;

					Log("player fired");
				}

				wasShooting = shooting;
			}

			{
				// v5.14: five finger fillet. In the logs every won game showed the objective hash 1616807014 and the lost game
				// 1122126072 (1 loss, 4 wins seen). One win = one check. Prototype: logged only; the client will count them.
				static bool filletInit = false;
				static int lastNote = 0;

				int note = JOURNAL::GET_LAST_NOTE_OBJECTIVE();

				if (!filletInit)
				{
					filletInit = true;
					lastNote = note;
				}
				else if (note != lastNote)
				{
					lastNote = note;

					if (note == 1616807014)
					{
						Log("MINIGAME WIN (five finger fillet, prototype)");

						Say("Fillet won!");
					}
					else if (note == 1122126072)
					{
						Log("MINIGAME LOSS (five finger fillet, prototype)");
					}
				}
			}

			if (g_shoveUntil != 0)
			{
				Actor shovePlayer = ACTOR::GET_PLAYER_ACTOR(-1);

				if (HEALTH::IS_ACTOR_RAGDOLL(shovePlayer))
				{
					g_shoveRagdoll = true;
				}

				if (now >= g_shoveUntil)
				{
					g_shoveUntil = 0;

					if (g_shoveSafe) { ACTOR::SET_ACTOR_INVULNERABILITY(shovePlayer, false); g_shoveSafe = false; }

					Log(std::string("shove result: ragdoll_seen=") + (g_shoveRagdoll ? "1" : "0") + " health " + std::to_string(g_shoveHealth0) + " -> "
						+ std::to_string(HEALTH::GET_ACTOR_HEALTH(shovePlayer)) + " dead=" + std::to_string(HEALTH::IS_ACTOR_DEAD(shovePlayer)));
				}
			}

			if (g_drunkUntil != 0)
			{
				static ULONGLONG nextDrunkTick = 0;

				if (now >= nextDrunkTick)
				{
					nextDrunkTick = now + 1000;

					Actor drunkPlayer = ACTOR::GET_PLAYER_ACTOR(-1);

					if (now >= g_drunkUntil || HEALTH::IS_ACTOR_DEAD(drunkPlayer))
					{
						g_drunkUntil = 0;

						HEALTH::SET_ACTOR_DRUNK(drunkPlayer, false);

						Log("drunk: ended");
					}
					else if (!HEALTH::IS_ACTOR_DRUNK(drunkPlayer))
					{
						HEALTH::SET_ACTOR_DRUNK(drunkPlayer, true);

						Log("drunk: re-applied");
					}
				}
			}

			if (now >= nextActorScan)
			{
				nextActorScan = now + 200;

				LARGE_INTEGER t0, t1, freq;

				QueryPerformanceFrequency(&freq);
				QueryPerformanceCounter(&t0);

				ScanActors();

				QueryPerformanceCounter(&t1);

				double ms = (double)(t1.QuadPart - t0.QuadPart) * 1000.0 / (double)freq.QuadPart;

				if (ms > g_scanMaxMs)
				{
					g_scanMaxMs = ms;
				}

				g_scanTotalMs += ms;
				g_scanCount++;

				if (now >= nextPerfLog)
				{
					nextPerfLog = now + 60000;

					Log("perf: actor scan avg=" + std::to_string(g_scanTotalMs / (double)g_scanCount) + "ms max=" + std::to_string(g_scanMaxMs) + "ms actors=" + std::to_string(g_lastActorCount) + " (last minute)");

					g_scanMaxMs = 0.0;
					g_scanTotalMs = 0.0;
					g_scanCount = 0;
				}
			}

			TrackAnimal(now);

			if (now >= nextCleanup)
			{
				nextCleanup = now + 2000;

				CleanupAnimals(now, false);
			}

			// Once a second: input file + optional named-script watcher
			if (now >= nextPoll)
			{
				nextPoll = now + 1000;

				ApItemsTick();

				StatGrantReloadTick();
				TrapQueueTick(now);
				HerbertTick(now);

				for (size_t i = 0; i < g_delayed.size();)
				{
					if (now >= g_delayed[i].first)
					{
						std::ofstream append(IN_PATH, std::ios::app);

						append << g_delayed[i].second << "\n";

						g_delayed.erase(g_delayed.begin() + i);
					}
					else
					{
						i++;
					}
				}

				std::ifstream in(IN_PATH);

				if (in)
				{
					std::string line;
					std::string all;

					while (std::getline(in, line))
					{
						all += line + "\n";
					}

					in.close();

					std::remove(IN_PATH);

					std::vector<std::string> commandLines;

					{
						size_t pos = 0;

						while (pos <= all.size())
						{
							size_t nl = all.find('\n', pos);

							std::string one = Trim(all.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos));

							if (!one.empty())
							{
								commandLines.push_back(one);
							}

							if (nl == std::string::npos)
							{
								break;
							}

							pos = nl + 1;
						}
					}

					// v5.10: every line of the file is its own command (the future client can queue several at once).
					for (const std::string& command : commandLines)
					{
					Log("received: " + command);

					if (command.rfind("after ", 0) == 0)
					{
						std::string rest = Trim(command.substr(6));
						size_t sp = rest.find(' ');
						std::string secondsText = sp == std::string::npos ? rest : rest.substr(0, sp);

						if (sp == std::string::npos || secondsText.find_first_not_of("0123456789") != std::string::npos)
						{
							Log("after: use  after <seconds> <command>");
						}
						else
						{
							g_delayed.push_back({ GetTickCount64() + (ULONGLONG)std::atoi(secondsText.c_str()) * 1000, Trim(rest.substr(sp + 1)) });

							Log("after: '" + Trim(rest.substr(sp + 1)) + "' scheduled in " + secondsText + " s");
						}

						continue;
					}

					if (command == "weapon" || command.rfind("weapon ", 0) == 0)
					{
						WeaponCommand(command.size() > 6 ? Trim(command.substr(7)) : std::string());
					}
					else if (command == "animalclear")
						{
							CleanupAnimals(GetTickCount64(), true);

							Say("Spawned animals removed");
						}
						else if (command.rfind("weaponlock ", 0) == 0)
						{
							WeaponLockCommand(Trim(command.substr(11)));
						}
						else if (command.rfind("removeweapon ", 0) == 0)
							{
								RemoveWeaponCommand(Trim(command.substr(13)));
							}
							else if (command == "removeall" || command.rfind("removeall ", 0) == 0)
							{
								RemoveAllCommand(command.size() > 9 ? command.substr(10) : std::string());
							}
							else if (command.rfind("equip ", 0) == 0)
						{
							int index = FindWeaponIndex(Trim(command.substr(6)));

							if (index >= 0) { EquipWeapon(index); } else { Log("equip: no weapon matches"); }
						}
						else if (command == "shove" || command.rfind("shove ", 0) == 0)
						{
							std::string name = "Dynamite";
							float distance = 3.0f;
							float size = 2.0f;

							{
								std::string rest = command.size() > 5 ? Trim(command.substr(6)) : std::string();
								std::vector<std::string> parts;
								size_t start = 0;

								while (start < rest.size())
								{
									size_t sp = rest.find(' ', start);
									std::string part = Trim(rest.substr(start, sp == std::string::npos ? std::string::npos : sp - start));

									if (!part.empty()) { parts.push_back(part); }

									if (sp == std::string::npos) { break; }

									start = sp + 1;
								}

								if (parts.size() > 0) { name = parts[0]; }
								if (parts.size() > 1) { distance = (float)std::atof(parts[1].c_str()); }
								if (parts.size() > 2) { size = (float)std::atof(parts[2].c_str()); }
							}

							Actor shovePlayer = ACTOR::GET_PLAYER_ACTOR(-1);

							Vector3 position = ACTOR::GET_POSITION(shovePlayer);

							float angle = (float)(std::rand() % 360) * 3.14159265f / 180.0f;

							Vector3 blast = position;

							blast.x += std::cos(angle) * distance;
							blast.y += std::sin(angle) * distance;

							Vector3 sphere;

							sphere.x = size;
							sphere.y = size;
							sphere.z = size;

							g_shoveHealth0 = HEALTH::GET_ACTOR_HEALTH(shovePlayer);
							g_shoveRagdoll = false;
							g_shoveUntil = GetTickCount64() + 4000;

							// v5.19: the first test (ManureExplosion, 3 m, size 2) killed John from full health. The player is now
							// invulnerable until the shove window ends so the blast only ragdolls (unless "shove <name> <m> <size> unsafe").
							g_shoveSafe = command.find("unsafe") == std::string::npos;

							if (g_shoveSafe) { ACTOR::SET_ACTOR_INVULNERABILITY(shovePlayer, true); }

							Log("shove: explosion '" + name + "' at " + std::to_string(distance) + " m (angle " + std::to_string((int)(angle * 180.0f / 3.14159265f))
								+ " deg), size " + std::to_string(size) + ", health before=" + std::to_string(g_shoveHealth0));

							EXPLOSION::_CREATE_EXPLOSION(&blast, name.c_str(), shovePlayer, &sphere, false);
						}
						else if (command == "drunk" || command.rfind("drunk ", 0) == 0)
						{
							int seconds = command.size() > 5 ? std::atoi(Trim(command.substr(6)).c_str()) : 30;

							Actor drunkPlayer = ACTOR::GET_PLAYER_ACTOR(-1);

							if (seconds > 0)
							{
								g_drunkUntil = GetTickCount64() + (ULONGLONG)seconds * 1000;

								HEALTH::SET_ACTOR_DRUNK(drunkPlayer, true);

								Log("drunk: on for " + std::to_string(seconds) + " s, is_drunk=" + std::to_string(HEALTH::IS_ACTOR_DRUNK(drunkPlayer)));

								ApBig("Drunk!", "Hic! Somebody spiked the whiskey.");
							}
							else
							{
								g_drunkUntil = 0;

								HEALTH::SET_ACTOR_DRUNK(drunkPlayer, false);

								Log("drunk: off, is_drunk=" + std::to_string(HEALTH::IS_ACTOR_DRUNK(drunkPlayer)));
							}
						}
						else if (command == "reclaim")
						{
							ReclaimWeapons("command");
						}
						else if (command.rfind("shopscan", 0) == 0)
						{
							ShopScan(command.size() > 8 ? std::atoi(Trim(command.substr(9)).c_str()) : 16);
						}
						else if (command.rfind("shopblock ", 0) == 0 || command.rfind("shopprice ", 0) == 0)
						{
							ShopSetCommand(command);
						}
						else if (command.rfind("shopwatch", 0) == 0)
						{
							g_shopWatch = command.size() > 9 && Trim(command.substr(9)) == "1";
							g_shopSnapshotValid = false;

							Log(std::string("shopwatch: ") + (g_shopWatch ? "on (open a shop first; overrides are re-applied 4x per second)" : "off"));
						}
						else if (command.rfind("shoprules", 0) == 0)
						{
							std::string arg = Trim(command.substr(9));

							if (arg == "reset")
							{
								g_shopDone.clear();
								std::remove(SHOP_DONE_PATH);
								Log("shoprules: bought-slot record cleared");
							}
							else
							{
								g_shopRules = arg == "1";
								g_shopActive = -1;
								Log(std::string("shoprules: ") + (g_shopRules ? "on" : "off"));
							}
						}
						else if (command == "shopclear")
						{
							g_shopOverrides.clear();

							Log("shopclear: all block/price overrides forgotten");
						}
						else if (command == "weapons")
					{
						DumpWeapons();
					}
					else if (command == "ammo")
					{
						MaxAmmo();
					}
					else if (command == "spawn" || command.rfind("spawn ", 0) == 0)
					{
						SpawnCommand(command.size() > 5 ? Trim(command.substr(6)) : std::string());
					}
					else if (command.rfind("notoriety ", 0) == 0 || command.rfind("wantedfx ", 0) == 0 || command.rfind("paybounty ", 0) == 0)
					{
						size_t sp = command.find(' ');
						BountyCommand(command.substr(0, sp), Trim(command.substr(sp + 1)));
					}
					else if (command == "itemdump" || command.rfind("itemdump ", 0) == 0)
					{
						int n = command.size() > 9 ? std::atoi(Trim(command.substr(9)).c_str()) : 64;

						ItemDump(n > 0 ? n : 64);
					}
					else if (command.rfind("received", 0) == 0)
					{
						ReceivedCommand(command.size() > 9 ? command.substr(9) : std::string("list"));
					}
					else if (command.rfind("sync", 0) == 0)
					{
						std::string arg = command.size() > 5 ? Trim(command.substr(5)) : std::string();

						if (arg == "now") { SyncNow("command"); }
						else
						{
							g_syncOn = arg == "1";

							if (g_syncOn) { SyncNow("sync on"); }

							Log(std::string("sync: ") + (g_syncOn ? "on" : "off"));
						}
					}
					else if (command.rfind("modelprobe ", 0) == 0)
					{
						ModelProbeCommand(command.substr(11));
					}
					else if (false && (command == "brawl" || command.rfind("brawl ", 0) == 0))   // v5.20k: brawl trap dropped
					{
						BrawlCommand(command.size() > 6 ? command.substr(6) : std::string());
					}
					else if (command == "slowmo" || command.rfind("slowmo ", 0) == 0)
					{
						SlowmoCommand(command.size() > 7 ? command.substr(7) : std::string());
					}
					else if (command == "storm" || command.rfind("storm ", 0) == 0)
					{
						StormCommand(command.size() > 6 ? command.substr(6) : std::string());
					}
					else if (command == "stampede" || command.rfind("stampede ", 0) == 0)
					{
						StampedeCommand(command.size() > 9 ? command.substr(9) : std::string());
					}
					else if (command == "outofbullets")
					{
						OutOfBulletsCommand();
					}
					else if (command.rfind("globalread ", 0) == 0)
					{
						GlobalReadCommand(command.substr(11));
					}
					else if (command.rfind("statadd ", 0) == 0) { StatChangeCommand("add", command.substr(8)); }
					else if (command.rfind("statsub ", 0) == 0) { StatChangeCommand("sub", command.substr(8)); }
					else if (command.rfind("statset ", 0) == 0) { StatChangeCommand("set", command.substr(8)); }
					else if (command.rfind("apmode ", 0) == 0)
					{
						g_apMode = Trim(command.substr(7)) == "1";
						ApModeChanged();
					}
					else if (command == "herbert") { HerbertCommand(); }
					else if (command == "outfitdump") { OutfitDump(); }
					else if (command.rfind("outfitgive ", 0) == 0) { OutfitGive(command.substr(11)); }
					else if (command == "posse")
					{
						g_gangDistanceBase = 40.0f;
						GangAttack("lawmen 3");
						g_gangDistanceBase = 22.0f;
					}
					else if (command == "deadeyedrain")
					{
						ACTOR::SET_PLAYER_DEADEYE_POINTS(ACTOR::GET_PLAYER_ACTOR(-1), 0.0f, true);
						ApBig("Dead Eye Drained", "Your focus is gone.");
					}
					else if (command.rfind("sendcheck ", 0) == 0)
					{
						// alpha safety valve: send a location by hand if the plugin missed it
						SendCheck(Trim(command.substr(10)));
					}
					else if (command.rfind("bigstyle ", 0) == 0) { g_bigStyle = std::atoi(command.c_str() + 9); ApBig("Big text style " + std::to_string(g_bigStyle), "sample line"); }
					else if (command.rfind("uitags ", 0) == 0) { g_uiTags = Trim(command.substr(7)) == "1"; }
					else if (command.rfind("statpopups ", 0) == 0) { g_statPopups = Trim(command.substr(11)) == "1"; }
					else if (command == "apstatus")
					{
						ProgressLoad();
						Log(std::string("apstatus: ap_mode=") + (g_apMode ? "1" : "0") + " vanilla_weapons=" + (g_vanillaWeapons ? "1" : "0") + " last_mission_script="
							+ g_lastMissionScript + " last_stranger_script=" + g_lastStrangerScript + " traps_waiting=" + std::to_string(g_trapQueue2.size()));
						for (const auto& kv : g_progress) { Log("apstatus: " + kv.first + " = " + std::to_string(kv.second)); }
					}
					else if (command == "progress" || command == "progress reset")
					{
						if (command == "progress reset")
						{
							g_progress.clear();
							g_progressLoaded = true;
							g_gangKillsLoaded = false;
							for (int g = 0; g < GANG_COUNT; g++) { g_gangKills[g] = 0; }
							ProgressSave();
							Log("progress: reset (encounters will baseline again, gang kills start at 0)");
						}
						else
						{
							ProgressLoad();
							for (const auto& kv : g_progress) { Log("progress: " + kv.first + " = " + std::to_string(kv.second)); }
						}
					}
					else if (command == "crimeprobe" || command.rfind("crimeprobe ", 0) == 0)
					{
						CrimeProbe(command.size() > 10 ? Trim(command.substr(11)) : std::string());
					}
					else if (command == "bountytrap")
					{
						// v5.20m: bounty trap = the bounty stat is the live amount (a later crime adds to it), lawmen come for it.
						StatChangeCommand("set", "bounty 666");
						g_gangDistanceBase = 40.0f;
						GangAttack("lawmen 3");
						g_gangDistanceBase = 22.0f;
						ApBig("Wanted: $666", "A bounty is on your head!");
					}
					else if (command == "hudmoney") { RefreshMoneyHud(); }
					else if (command == "shopdump")
					{
						ShopDump();
					}
					else if (command.rfind("cashadd", 0) == 0)
					{
						CashAddCommand(std::atoi(command.c_str() + 7));
					}
					else if (command == "stats")
					{
						StatsCommand();
					}
					else if (command.rfind("mgprobe", 0) == 0)
					{
						g_mgProbe = command.find('1') != std::string::npos;

						Log(std::string("mgprobe: ") + (g_mgProbe ? "on" : "off"));
					}
					else if (command == "heal")
					{
						Actor healPlayer = ACTOR::GET_PLAYER_ACTOR(-1);

						HEALTH::SET_ACTOR_HEALTH(healPlayer, HEALTH::GET_ACTOR_MAX_HEALTH(healPlayer));

						Log("heal: health now " + std::to_string(HEALTH::GET_ACTOR_HEALTH(healPlayer)));
					}
					else if (command.rfind("kopoints", 0) == 0)   // v5.20k: passout trap dropped (kopoints is log-only)
					{
						// v5.19 non-explosive shove tests: the knockout points / passed-out natives. Both reuse the shove result logger.
						Actor koPlayer = ACTOR::GET_PLAYER_ACTOR(-1);

						g_shoveHealth0 = HEALTH::GET_ACTOR_HEALTH(koPlayer);
						g_shoveRagdoll = false;
						g_shoveSafe = false;
						g_shoveUntil = GetTickCount64() + 5000;

						if (command.rfind("passout ", 0) == 0)
						{
							// v5.20k: "passout <seconds>" passes John out for that long (3 to 10 s, 0 = wake now). The old "passout 1" had no
							// timer and kept him out until woken by hand.
							int seconds = std::atoi(Trim(command.substr(8)).c_str());

							if (seconds > 10) { seconds = 10; }

							int value = seconds > 0 ? 1 : 0;

							if (seconds > 0 && seconds < 3) { seconds = 3; }

							g_passoutUntil = seconds > 0 ? GetTickCount64() + (ULONGLONG)seconds * 1000 : 0;

							HEALTH::SET_ACTOR_PASSED_OUT(koPlayer, value);

							Log("passout: set to " + std::to_string(value) + ", ragdoll=" + std::to_string(HEALTH::IS_ACTOR_RAGDOLL(koPlayer)));
						}
						else
						{
							Log("kopoints: ko=" + std::to_string(HEALTH::GET_ACTOR_KO_POINTS(koPlayer)) + " max=" + std::to_string(HEALTH::GET_ACTOR_MAX_KO_POINTS(koPlayer))
								+ " has_ko=" + std::to_string(HEALTH::_ACTOR_HAS_KO_POINTS(koPlayer)));

							if (command.size() > 9)
							{
								int value = std::atoi(Trim(command.substr(9)).c_str());

								HEALTH::SET_ACTOR_KO_POINTS(koPlayer, value);

								Log("kopoints: set to " + std::to_string(value) + ", now ko=" + std::to_string(HEALTH::GET_ACTOR_KO_POINTS(koPlayer)));
							}
						}
					}
					else if (command == "gangattack" || command.rfind("gangattack ", 0) == 0)
					{
						GangAttack(command.size() > 11 ? Trim(command.substr(11)) : std::string());
					}
					else if (command == "gangkills")
					{
						GangKillsLoad(); for (int g = 0; g < GANG_COUNT; g++) { Log(std::string("gangkills: ") + GANGS[g].name + " " + std::to_string(g_gangKills[g])); }
					}
					else if (command == "journaldump")
					{
						JournalDump();
					}
					else if (command.rfind("itemhas ", 0) == 0)
					{
						ItemHas(command.substr(8));
					}
					else if (command.rfind("itemdel ", 0) == 0)
					{
						ItemDelete(command.substr(8));
					}
					else if (command.rfind("additem ", 0) == 0)
					{
						std::string rest = Trim(command.substr(8));
						int count = 1;

						size_t lastSpace = rest.find_last_of(' ');

						if (lastSpace != std::string::npos)
						{
							std::string tail = Trim(rest.substr(lastSpace + 1));

							if (!tail.empty() && tail.find_first_not_of("0123456789") == std::string::npos)
							{
								count = std::atoi(tail.c_str());

								rest = Trim(rest.substr(0, lastSpace));
							}
						}

						AddItem(rest, count);
					}
					else if (command.rfind("statmax ", 0) == 0)
					{
						int n = std::atoi(command.substr(8).c_str());

						if (n < 16)
						{
							n = 16;
						}

						if (n > 4096)
						{
							n = 4096;
						}

						g_statScanMax = n;
						g_hasSnapshot = false;

						Log("statmax set to " + std::to_string(n) + " (baseline reset, press End to recalibrate)");

						Say("Stat range " + std::to_string(n) + ", press End to recalibrate");
					}
					else if (command == "loadmissions" || command.rfind("missionend", 0) == 0 || command.rfind("mission ", 0) == 0)
						{
							MissionCommand(command);
						}
						else if (command == "quiet 1" || command == "quiet 0")
						{
							g_quiet = command == "quiet 1";

							Log(std::string("quiet mode ") + (g_quiet ? "on" : "off"));
						}
						else if (command.rfind("deadeyelevel ", 0) == 0)
						{
							g_deadeyeLevel = std::atoi(Trim(command.substr(13)).c_str());

							ApplyDeadEyeLevel(true);
						}
						else if (command == "consolemirror 1" || command == "consolemirror 0")
						{
							g_consoleMirror = command == "consolemirror 1";

							Log(std::string("console mirror ") + (g_consoleMirror ? "on" : "off"));
						}
						else if (command.rfind("deathlink ", 0) == 0)
						{
							DeathLinkReceive(Trim(command.substr(10)));
						}
						else if (command.rfind("deathmsg ", 0) == 0)
						{
							std::string text = PickDeathMessage(Trim(command.substr(9)));

							Log("death message preview: " + text);

							Say("John " + text);
						}
						else if (command.rfind("consolelog ", 0) == 0)
						{
							CORE::_LOG_MESSAGE(Trim(command.substr(11)).c_str());
						}
						else if (command == "die")
						{
							HEALTH::KILL_ACTOR(ACTOR::GET_PLAYER_ACTOR(-1));

							Log("die: KILL_ACTOR on player");
						}
						else if (command.rfind("deadeye", 0) == 0)
						{
							DeadEyeCommand(command.size() > 7 ? Trim(command.substr(8)) : std::string("status"));
						}
						else if (command.rfind("statfind ", 0) == 0)
						{
							StatFind(std::atof(Trim(command.substr(9)).c_str()));
						}
						else if (command == "statdump")
						{
							StatDump();
						}
						else if (command == "statreset")
					{
						g_hasSnapshot = false;

						Log("stat baseline reset (press End to recalibrate)");

						Say("Stat baseline reset, press End");
					}
					else
					{
						Say("Note logged:\n" + command);
					}
					}
				}

				std::ifstream scripts(SCRIPTS_PATH);

				if (scripts)
				{
					std::string name;

					while (std::getline(scripts, name))
					{
						name = Trim(name);

						if (!name.empty())
						{
							WatchValue("script " + name, CORE::_IS_ANY_NAMED_SCRIPT_RUNNING(name.c_str()) ? 1 : 0);
						}
					}
				}
			}

			if (KeyEdge(KEY_F7, f7))
			{
				Vector3 position = ACTOR::GET_POSITION(ACTOR::GET_PLAYER_ACTOR(-1));

				Log("player position: " + std::to_string(position.x) + ", " + std::to_string(position.y) + ", " + std::to_string(position.z));

				Say("Position logged");
			}

			// Evaluate both keys every frame so each keeps its own edge state, then combine.
			bool endPressed = KeyEdge(KEY_END, statKeyA);
			bool plusPressed = KeyEdge(KEY_NUMPAD_PLUS, statKeyB);

			if (endPressed || plusPressed)
			{
				StatSnapshot();
			}

			if (now >= nextWeaponWatch)
			{
				nextWeaponWatch = now + 1000;

				SyncTick(now);
				BrawlTick(now);
				MoreTrapsTick(now);
				WeaponWatch();
			}


			DeathWatch();

			if (g_shopRules && now >= nextShopWatch)
			{
				nextShopWatch = now + 250;

				ShopTick2();   // v5.23: every town; the Armadillo-only ShopRulesTick is kept for reference but no longer runs

				// removals queued by the shop code (gear by name, weapons by index)
				GearRemovalTick();

				for (size_t k = 0; k < g_shopRemovals.size();)
				{
					if (GetTickCount64() >= g_shopRemovals[k].first)
					{
						int index = g_shopRemovals[k].second;

						if (!g_apGranted.count((int)TEST_WEAPONS[index].id)) { RemoveWeaponByIndex(index); }

						g_shopRemovals.erase(g_shopRemovals.begin() + k);
					}
					else { k++; }
				}
			}
			else if (g_shopWatch && now >= nextShopWatch)
			{
				nextShopWatch = now + 250;

				ShopWatchTick();
			}

			if (g_deadeyeLevel >= 0 && now >= nextDeadeye)
			{
				nextDeadeye = now + 5000;

				// Not while Dead Eye is active: changing its settings mid-use could cancel it.
				if (GAME::GET_GAME_STATE() == 3 && !ACTOR::IS_PLAYER_DEADEYE(ACTOR::GET_PLAYER_ACTOR(-1)))
				{
					ApplyDeadEyeLevel(false);
				}
			}

			if (KeyEdge(KEY_F9, f9))
			{
				GiveNextWeapon();
			}

			if (KeyEdge(KEY_F10, f10))
			{
				MaxAmmo();
			}

			if (KeyEdge(KEY_F11, f11))
			{
				SpawnCommand("");
			}

			ScriptWait(0);
		}
	});
}



void Application::Shutdown(HMODULE _Module)
{
	ScriptUnregister(_Module);
}
