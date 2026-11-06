"""Static game data for the Red Dead Redemption (RDR1) APWorld.

Everything that describes the game lives here so it can be corrected without touching
generation logic. Ids are append-only: removed items and locations keep their ids reserved
so older seeds still read.
"""

GAME_NAME = "Red Dead Redemption"
BASE_ID = 8_200_000

# ---------------------------------------------------------------------------------------
# Story chapters (NOT items: missions play in the vanilla order; each mission is a check)
# ---------------------------------------------------------------------------------------
# (cluster name, [story missions], [prerequisite clusters], Dead Eye level needed)
#
# A chapter only sets the Dead Eye level logic expects by that point of the story (pacing); the game itself decides
# which mission comes next. Chapters were progression items before beta 1.
#
# The grouping follows the story order from ireddead.com's mission list, checked against a full playthrough
# (Beta 3.0). Landon Ricketts' chapter needs Dead Eye 3: his first mission teaches target painting.
CLUSTERS = [
    ("Armadillo Arrival",
     ["Exodus in America", "New Friends, Old Problems", "Obstacles in Our Path", "This is Armadillo, USA"],
     [], 0),
    ("MacFarlane Ranch",
     ["Women and Cattle", "Wild Horses, Tamed Passions", "A Tempest Looms"],
     ["Armadillo Arrival"], 0),
    ("Armadillo Law",
     ["Political Realities in Armadillo", "Justice in Pike's Basin", "Spare the Rod, Spoil the Bandit"],
     ["Armadillo Arrival"], 0),
    ("West Dickens' Schemes",
     ["Old Swindler Blues"],
     ["Armadillo Arrival"], 0),
    ("The Burning",
     ["The Burning", "Hanging Bonnie MacFarlane", "You Shall Not Give False Testimony, Except for Profit"],
     ["MacFarlane Ranch", "Armadillo Law", "West Dickens' Schemes"], 0),
    ("Seth's Graveyard",
     ["Exhuming and Other Fine Hobbies", "A Gentle Drive with Friends", "Let the Dead Bury Their Dead"],
     ["The Burning"], 0),
    ("Irish and Friends",
     ["Liars, Cheats and Other Proud Americans", "A Frenchman, a Welshman and an Irishman",
      "Man is Born Unto Trouble", "On Shaky's Ground"],
     ["Seth's Graveyard"], 1),
    ("Assault on Fort Mercer",
     ["Can a Swindler Change His Spots?", "The Sport of Kings, and Liars", "The Assault on Fort Mercer"],
     ["Irish and Friends"], 1),
    ("Crossing the Border",
     ["We Shall Be Together in Paradise"],
     ["Assault on Fort Mercer"], 1),
    ("De Santa's Army",
     ["Civilization, At Any Price", "The Demon Drink", "Empty Promises", "Mexican Caesar"],
     ["Crossing the Border"], 1),
    ("Landon Ricketts",
     ["The Gunslinger's Tragedy", "Landon Ricketts Rides Again", "Lucky in Love", "The Mexican Wagon Train"],
     ["Crossing the Border"], 3),   # Beta 3.0: Landon's first mission teaches target painting (Dead Eye 3) and needs it
    ("Fortuna's Rebels",
     ["My Sister's Keeper", "Must a Savior Die?"],
     ["Crossing the Border"], 1),
    ("De Santa's Downfall",
     ["Cowards Die Many Times", "Father Abraham", "Captain De Santa's Downfall"],
     ["De Santa's Army", "Landon Ricketts", "Fortuna's Rebels"], 2),
    ("Reyes' Rebellion",
     ["The Great Mexican Train Robbery", "The Gates of El Presidio", "An Appointed Time"],
     ["De Santa's Downfall"], 2),
    ("Ross's Offer",
     ["Bear One Another's Burdens", "Great Men Are Not Always Wise"],
     ["Reyes' Rebellion"], 2),
    ("Dutch's Hideout",
     ["At Home with Dutch", "For Purely Scientific Purposes", "The Prodigal Son Returns (to Yale)"],
     ["Ross's Offer"], 3),
    ("The Truth",
     ["And You Will Know the Truth", "And the Truth Will Set You Free"],
     ["Dutch's Hideout"], 3),
    ("Homecoming",
     ["The Outlaw's Return", "Pestilence", "Old Friends, New Problems"],
     ["The Truth"], 3),
    ("Marston Ranch",
     ["By Sweat and Toil", "A Continual Feast"],
     ["Homecoming"], 3),
    ("Father and Son",
     ["John Marston and Son", "Wolves, Dogs and Sons", "Spare the Love, Spoil the Child"],
     ["Marston Ranch"], 3),
    ("The Last Enemy",
     ["The Last Enemy That Shall Be Destroyed"],
     ["Father and Son"], 4),
]

CLUSTER_NAMES = [c[0] for c in CLUSTERS]

# Dead Eye a chapter needs in logic: its own level, and never less than any chapter it follows (the story is played in order,
# so a chapter locked behind Landon Ricketts' Dead Eye 3 locks everything after it too). CLUSTERS is in story order.
CHAPTER_DEAD_EYE = {}
for _name, _missions, _after, _dead_eye in CLUSTERS:
    CHAPTER_DEAD_EYE[_name] = max([_dead_eye] + [CHAPTER_DEAD_EYE[a] for a in _after])
FINAL_CLUSTER = "The Last Enemy"

# Progressive Dead Eye: 1 Dead Eye, 2 auto lock-on, 3 target painting, 4 fast recovery (the plugin sets the game's flags).
DEAD_EYE_ITEM = "Progressive Dead Eye"
DEAD_EYE_LEVELS = 4

# ---------------------------------------------------------------------------------------
# Areas. Logic expects an area once John has the Dead Eye level of the listed chapter.
# ---------------------------------------------------------------------------------------
AREA_GATES = {
    "New Austin": "Armadillo Arrival",
    "Nuevo Paraiso": "Crossing the Border",
    "West Elizabeth": "Ross's Offer",
}

# ---------------------------------------------------------------------------------------
# Stranger missions (base game). "Remember My Family" is the Edgar Ross epilogue and is
# used as the goal in "full_story" mode instead of being a normal check.
# ---------------------------------------------------------------------------------------
STRANGERS = {
    "New Austin": [
        "American Appetites", "California", "Flowers for a Lady", "Funny Man", "I Know You",
        "Jenny's Faith", "Let No Man Put Asunder", "Lights, Camera, Action", "Water and Honesty",
        "Who Are You to Judge?",
    ],
    "Nuevo Paraiso": ["Aztec Gold", "Deadalus and Son", "Eva in Peril", "Love is the Opiate", "Poppycock"],
    "West Elizabeth": ["American Lobbyist", "The Prohibitionist", "The Wronged Woman"],
}

# ---------------------------------------------------------------------------------------
# Gang hideouts: one "cleared" check each (the hideout's best-time stat). Gang kills are counted per gang, below.
# ---------------------------------------------------------------------------------------
HIDEOUTS = {
    "New Austin": ["Pike's Basin", "Twin Rocks", "Fort Mercer", "Tumbleweed", "Gaptooth Breach"],
    "Nuevo Paraiso": ["Nosalida", "Tesoro Azul"],
}
MAX_GANG_KILLS = 10   # kill checks per gang (one every 5 kills, up to 50)

# Gang kill checks are counted per gang (the plugin reads the actor's faction id), not per hideout.
# name -> (area, faction id from the community eFaction enum). Scripted mission enemies (faction 19) never count.
GANG_FACTIONS = {
    "Walton's Gang": ("New Austin", 25),
    "Banditos": ("Nuevo Paraiso", 12),
    # Beta 3.0: rebels are John's allies until Reyes wins; after "An Appointed Time" they are Mexico's law (a bounty brings
    # them). So their kill checks come after that point in logic: the chapter that follows it (tester).
    "Reyes' Rebels": ("Ross's Offer", 27),
    "Mexican Soldiers": ("Nuevo Paraiso", 16),
    "Treasure Hunters": ("West Elizabeth", 24),
    "Cattle Rustlers": ("West Elizabeth", 14),
    "Indian Raiders": ("West Elizabeth", 15),
    "Smugglers": ("West Elizabeth", 26),
    "Bounty Hunters": ("West Elizabeth", 11),
}

OLD_MAX_HAT_TRICKS = 5   # alpha/beta 1 count (their ids come first)
MAX_HAT_TRICKS = 15

# Shop purchases. One location per item ("Shop - <item>"), buyable in any store that sells it (owning one stops the other
# stores from selling it, so per-store checks could become impossible). Stock from ireddead.com's shop list; the plugin
# recognises the store by its shelf. Value = area or the cluster that must be received first. Consumables, horse deeds and the
# Buffalo Rifle (needs Master Hunter 5) are not checks.
SHOP_LOCATIONS = {
    "Shop - Rabbit's Foot": "New Austin", "Shop - Improved Campsite": "New Austin", "Shop - Bandolier": "New Austin",
    "Shop - Survivalist Map": "New Austin", "Shop - Cholla Springs Map": "New Austin", "Shop - Gaptooth Ridge Map": "New Austin",
    "Shop - Rio Bravo Map": "New Austin", "Shop - Hennigan's Stead Map": "New Austin",
    "Shop - Volcanic Pistol": "New Austin", "Shop - Double Barrel Shotgun": "New Austin", "Shop - Winchester Repeater": "New Austin",
    "Shop - Sawed-Off Shotgun": "New Austin", "Shop - Springfield Rifle": "New Austin",
    "Shop - Perdido Map": "Nuevo Paraiso", "Shop - Diez Coronas Map": "Nuevo Paraiso", "Shop - Punta Orgullo Map": "Nuevo Paraiso",
    "Shop - Semi-Auto Pistol": "Nuevo Paraiso", "Shop - Bolt Action Rifle": "Nuevo Paraiso", "Shop - Semi-Auto Shotgun": "Nuevo Paraiso",
    "Shop - LeMat Revolver": "Reyes' Rebellion",
    "Shop - Tall Trees Map": "West Elizabeth", "Shop - Great Plains Map": "West Elizabeth", "Shop - Mauser Pistol": "West Elizabeth",
    "Shop - Evans Repeater": "West Elizabeth", "Shop - Carcano Rifle": "West Elizabeth",
    # tailors (Thieves' Landing, Blackwater) and the Escalera general store's bandana
    "Shop - Bandana": "Nuevo Paraiso", "Shop - Elegant Suit": "New Austin", "Shop - Bollard Gang Scrap": "New Austin",
    "Shop - Walton's Gang Scrap": "New Austin", "Shop - Treasure Hunter Scrap": "New Austin",
}
# Beta 3.0: the Mexican outfit scraps (Chuparosa and Escalera general stores) are checks too (tester). Their ids come after the
# bounties (append only). The US Military scrap stays out (the army uniform is not an item).
LATE_SHOP_LOCATIONS = {"Shop - Bandito Scrap": "Nuevo Paraiso", "Shop - Rebel Scrap": "Nuevo Paraiso"}

# Beta 3.0: fame and honor RANK checks: reaching each title sends a check. Titles from the game's own lists (gamepressure /
# ireddead). Cut-offs: Greenhorn 1000, Buckaroo 2000, Mercenary 4000, Honest Joe 1500 and Peacemaker 2500 were seen in play;
# the rest follow the same pattern (fame doubles and is capped at 10000 by the game, honor steps 1000 from 1500, capped at 5000
# either way). title -> (cut-off, logic area or chapter).
FAME_RANKS = {"Greenhorn": (1000, "New Austin"), "Buckaroo": (2000, "New Austin"), "Mercenary": (4000, "Nuevo Paraiso"),
              "Gunslinger": (8000, "West Elizabeth"), "Legend": (10000, "Homecoming")}
GOOD_HONOR_RANKS = {"Honest Joe": (1500, "New Austin"), "Peacemaker": (2500, "Nuevo Paraiso"), "Hero": (3500, "West Elizabeth")}
BAD_HONOR_RANKS = {"Rustler": (-1500, "New Austin"), "Road Agent": (-2500, "Nuevo Paraiso"), "Desperado": (-3500, "West Elizabeth")}

# Minigame wins are read from each game's own stats (blackjack per hand, five finger fillet from its win objective); a win
# sends the first "Win N" not sent yet. Found in the decompiled scripts.
# Random encounters were removed in beta 2 (not reliable); their location ids stay reserved.
MAX_RANDOM_ENCOUNTERS = 20

# Minigames: one check per win, up to MAX_MINIGAME_WINS.
MINIGAMES = ["Poker", "Blackjack", "Liar's Dice", "Five Finger Fillet", "Horseshoes", "Arm Wrestling", "Duels"]
MAX_MINIGAME_WINS = 5

# Bounties per area.
BOUNTIES = {"New Austin": 8, "Nuevo Paraiso": 8, "West Elizabeth": 4}

# Ambient challenges: removed in beta 2 (ids reserved).
CHALLENGES = ["Survivalist", "Sharpshooter", "Master Hunter"]
MAX_CHALLENGE_RANKS = 10

# Treasure map finds are not tracked (dropped by the user). Buying the maps in the General Store is still a shop check.

# ---------------------------------------------------------------------------------------
# Items
# ---------------------------------------------------------------------------------------
# Names match the weapon list in the RedHook plugin (Application.cpp, TEST_WEAPONS). The Knife and the Lasso are not
# items: John always has them.
GUNS = [
    "Cattleman Revolver", "Schofield Revolver", "Double Action Revolver", "LeMat Revolver",
    "Volcanic Pistol", "Semi-Auto Pistol", "High Power Pistol", "Mauser Pistol",
    "Carbine Repeater", "Winchester Repeater", "Henry Repeater", "Evans Repeater",
    "Springfield Rifle", "Bolt Action Rifle", "Buffalo Rifle",
    "Sawed-Off Shotgun", "Double Barrel Shotgun", "Pump Action Shotgun", "Semi-Auto Shotgun",
    "Rolling Block Rifle", "Carcano Rifle",
]
THROWN = ["Throwing Knives", "Fire Bottles", "Dynamite", "Tomahawk"]
WEAPONS = GUNS + THROWN
DEFAULT_START_WEAPON = "Cattleman Revolver"

# Outfits: cosmetic items, each sent once. Name -> internal name in the game's outfit table (plugin "outfitgive"). Matched to
# the wiki's outfit list: social, gun, suit, treasur, rustler and bruise are confirmed in game, the rest by elimination.
# The plugin takes away any pool outfit the player owns without having received it.
OUTFITS = {
    "Outfit: Gentleman's Attire": "social", "Outfit: Deadly Assassin Outfit": "gun", "Outfit: Elegant Suit": "suit",
    "Outfit: Walton's Gang Outfit": "dnd", "Outfit: Bollard Twins Outfit": "rustler", "Outfit: Treasure Hunter Outfit": "treasur",
    "Outfit: Bandito Outfit": "bandito", "Outfit: Reyes' Rebels Outfit": "rebel", "Outfit: U.S. Army Uniform": "military",
    "Outfit: U.S. Marshal Uniform": "uslaw", "Outfit: Legend of the West": "legend", "Outfit: Bureau Uniform": "fbi",
    "Outfit: Duster Coat": "duster", "Outfit: Mexican Poncho": "poncho", "Outfit: Rancher Outfit": "rancher",
    "Outfit: Expert Hunter Outfit": "mount", "Outfit: Savvy Merchant Outfit": "bruise",
}

# Gentleman's Attire is unlocked from the start on PC (the Social Club / Rockstar Games Launcher bonus), so it is not an item.
# Beta 3.0: the three law uniforms ARE items again. The pause menu still won't offer them before late in the story (pause.c:
# unless story mission 57 is passed, it clears their "wearable" bit whenever it opens), so the plugin puts a received uniform
# on with F10 instead, through the game's own outfit change.
STORY_LOCKED_OUTFITS = {"Outfit: Gentleman's Attire"}

# Fame / Honor +50 are back (beta 2.1): they use the two ids the old +20 items had, and the plugin gives them again if a
# checkpoint reload takes them back (the reason they were removed in beta 2).
FILLER_ITEMS = ["Max Ammo", "Dead Eye Refill", "$15 Cash", "$35 Cash", "$55 Cash", "Fame +50", "Honor +50"]
REMOVED_FILLERS = 0
# Consumables put straight into the satchel (display name -> the game's internal item name, tested with "additem").
# They stay in the satchel, so a reload cannot take them back. Their ids come after the traps (appended).
CONSUMABLES = {"Snake Oil": "SNAKE_OIL", "Tonic": "TONIC", "Medicine": "MEDS_LVL1", "Moonshine": "MOONSHINE",
               "Chewing Tobacco": "CHEWING_TOBACCO"}
ALL_FILLERS = FILLER_ITEMS + list(CONSUMABLES)
# Relative weights for random filler (Dead Eye Refill came up far too often, so it has half the weight of the others).
FILLER_WEIGHTS = {"Dead Eye Refill": 1}
FILLER_DEFAULT_WEIGHT = 2

# Fame / honor milestones: removed in beta 2 (ids reserved). Rank checks are planned for beta 3 (fame ranks every 1000,
# honor ranks at 1500, 2500, ...).
MAX_STAT_MILESTONES = 10

# Animal ambush traps. The plugin also spawns a harmless rabbit with every ambush, for fun.
TRAP_ANIMALS = ["Cougar", "Grizzly Bear", "Black Bear", "Wolf", "Coyote", "Bobcat", "Boar", "Snake"]
ANIMAL_TRAPS = [f"{a} Ambush Trap" for a in TRAP_ANIMALS]
RANDOM_ANIMAL_TRAP = "Random Animal Ambush Trap"
GANG_TRAP = "Gang Attack Trap"
# Drains the Dead Eye meter. (The $666 bounty trap was removed again, see below.)
DEAD_EYE_DRAIN_TRAP = "Dead Eye Drain Trap"
# Makes John drunk for 30 seconds (plugin command "drunk <seconds>").
DRUNK_TRAP = "Drunk Trap"
# Plugin commands slowmo / stampede / outofbullets / gangattack lawmen.
SLOWMO_TRAP = "Slow Motion Trap"
STAMPEDE_TRAP = "Stampede Trap"
OUT_OF_BULLETS_TRAP = "Out of Bullets Trap"
POSSE_TRAP = "Lawmen Posse Trap"
# Bounty trap: removed for now (the plugin has "bountytrap" but the game shows no wanted state; waiting for the real crime event).
# Herbert Moon: removed in Beta 3.0 (see REMOVED_TRAPS).
HERBERT_TRAP = "Herbert Moon Trap"
# Three lightning strikes land 5-12 m from John (plugin command "lightning"; the storm was dropped in Beta 3.0).
LIGHTNING_TRAP = "Lightning Storm Trap"
EXTRA_TRAPS = [SLOWMO_TRAP, STAMPEDE_TRAP, OUT_OF_BULLETS_TRAP, POSSE_TRAP, HERBERT_TRAP, LIGHTNING_TRAP]   # append only (ids)
# Beta 3.0: the Herbert Moon trap is out of the pool (his voice lines never played in game). The item keeps its id so older seeds
# still read, but no new seed places it.
REMOVED_TRAPS = [HERBERT_TRAP]

# Beta 3.0: a gunman calls John out and the game's own duel starts (plugin command "duel"). Win it and it counts as a duel win;
# lose it and John dies (DeathLink goes out). Its id comes after the consumables (ids are append-only).
DUEL_TRAP = "Duel Trap"
LATE_TRAPS = [DUEL_TRAP]
ACTIVE_EXTRA_TRAPS = [t for t in EXTRA_TRAPS if t not in REMOVED_TRAPS] + LATE_TRAPS

# Beta 2.1 bounties: completed wanted-poster bounties, counted in the order you finish them ("Bounty 1", "Bounty 2", ...).
# The game has 20; logic expects the first 8 in New Austin, the next 8 in Nuevo Paraiso and the last 4 in West Elizabeth.
MAX_BOUNTY_CHECKS = 20
