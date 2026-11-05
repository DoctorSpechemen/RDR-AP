"""Static game data for the Red Dead Redemption (RDR1) APWorld.

Everything that describes the game lives here so it can be corrected without touching
generation logic. Items marked DRAFT were written from memory / secondary sources and
should be verified in-game before the first public release.
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
# DRAFT: the grouping follows the story order from ireddead.com's mission list. The exact
# unlock/branch order in the game still has to be confirmed (see README notes).
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
     ["Crossing the Border"], 1),
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
START_CLUSTER = "Armadillo Arrival"
FINAL_CLUSTER = "The Last Enemy"

# Progressive Dead Eye upgrades. DRAFT: level count and gating are a design choice.
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
# Gang hideouts. Each hideout gets one "cleared" check plus up to MAX_GANG_KILLS kill checks.
# (Gang names intentionally not used: one gang per hideout.)
# ---------------------------------------------------------------------------------------
HIDEOUTS = {
    "New Austin": ["Pike's Basin", "Twin Rocks", "Fort Mercer", "Tumbleweed", "Gaptooth Breach"],
    "Nuevo Paraiso": ["Nosalida", "Tesoro Azul"],
}
MAX_GANG_KILLS = 10

# Gang kill checks are counted per gang (the plugin reads the actor's faction id), not per hideout.
# name -> (area, faction id from the community eFaction enum). Scripted mission enemies (faction 19) never count.
GANG_FACTIONS = {
    "Walton's Gang": ("New Austin", 25),
    "Banditos": ("Nuevo Paraiso", 12),
    "Reyes' Rebels": ("Nuevo Paraiso", 27),
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

# Minigame wins are read from each game's "money won" stat (a win adds the winnings). Hideout clears from each hideout's
# best-time stat. Both found in the decompiled scripts.
# Random encounters: the game's "unique world events encountered" counter (sn_btin). Each new encounter is one check; a save
# reload cannot count the same ones twice (the plugin keeps a high-water mark).
MAX_RANDOM_ENCOUNTERS = 20

# Minigames: one check per win, up to MAX_MINIGAME_WINS.
MINIGAMES = ["Poker", "Blackjack", "Liar's Dice", "Five Finger Fillet", "Horseshoes", "Arm Wrestling", "Duels"]
MAX_MINIGAME_WINS = 5

# Bounties per area.
BOUNTIES = {"New Austin": 8, "Nuevo Paraiso": 8, "West Elizabeth": 4}

# Ambient challenges (Treasure Hunter is covered by the treasure map option instead).
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

# Filler. Cash is written straight into the game's money stat and redrawn on the HUD (tested). "Dead Eye Refill" fills the meter.
# Outfits: cosmetic items, each sent once. Name -> internal outfit name in the game's outfit table (plugin "outfitgive").
# "Gentleman's Attire" = social is confirmed in game; the other display names are best guesses from the internal names.
# Internal names from the game's outfit table. Matched to the wiki's outfit list: social (Gentleman's Attire) is confirmed in
# game, gun = Deadly Assassin was seen by the tester; mount (Expert Hunter) and bruise (Savvy Merchant) are by elimination.
OUTFITS = {
    "Outfit: Gentleman's Attire": "social", "Outfit: Deadly Assassin Outfit": "gun", "Outfit: Elegant Suit": "suit",
    "Outfit: Walton's Gang Outfit": "dnd", "Outfit: Bollard Twins Outfit": "rustler", "Outfit: Treasure Hunter Outfit": "treasur",
    "Outfit: Bandito Outfit": "bandito", "Outfit: Reyes' Rebels Outfit": "rebel", "Outfit: U.S. Army Uniform": "military",
    "Outfit: U.S. Marshal Uniform": "uslaw", "Outfit: Legend of the West": "legend", "Outfit: Bureau Uniform": "fbi",
    "Outfit: Duster Coat": "duster", "Outfit: Mexican Poncho": "poncho", "Outfit: Rancher Outfit": "rancher",
    "Outfit: Expert Hunter Outfit": "mount", "Outfit: Savvy Merchant Outfit": "bruise",
}

# The pause menu takes these three away again until late in the story (pause.c: unless story mission 57 is passed, the
# wearable bit of uslaw, military and fbi is cleared every time the menu opens), so they are not sent as items.
# Gentleman's Attire is unlocked from the start on PC (the Social Club / Rockstar Games Launcher bonus), so it is not an item either.
STORY_LOCKED_OUTFITS = {"Outfit: U.S. Marshal Uniform", "Outfit: U.S. Army Uniform", "Outfit: Bureau Uniform",
                        "Outfit: Gentleman's Attire"}

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

# Fame / honor milestones: one check every 500 points (honor counts either way). Later milestones need later areas.
MAX_STAT_MILESTONES = 10
MILESTONE_STEP = 500

# Animal ambush traps. The plugin also spawns a harmless rabbit with every ambush, for fun.
TRAP_ANIMALS = ["Cougar", "Grizzly Bear", "Black Bear", "Wolf", "Coyote", "Bobcat", "Boar", "Snake"]
ANIMAL_TRAPS = [f"{a} Ambush Trap" for a in TRAP_ANIMALS]
RANDOM_ANIMAL_TRAP = "Random Animal Ambush Trap"
GANG_TRAP = "Gang Attack Trap"
# Drains the Dead Eye meter. (The $666 bounty trap was removed again, see below.)
DEAD_EYE_DRAIN_TRAP = "Dead Eye Drain Trap"
# Makes John drunk for a while (the plugin command is "drunk <seconds>"). DRAFT: the plugin side is untested in game.
DRUNK_TRAP = "Drunk Trap"
# Added later (plugin commands slowmo / stampede / outofbullets / gangattack lawmen). Storm and brawl traps were dropped after testing.
SLOWMO_TRAP = "Slow Motion Trap"
STAMPEDE_TRAP = "Stampede Trap"
OUT_OF_BULLETS_TRAP = "Out of Bullets Trap"
POSSE_TRAP = "Lawmen Posse Trap"
# Bounty trap: removed for now (the plugin has "bountytrap" but the game shows no wanted state; waiting for the real crime event).
# An invisible Herbert Moon follows John for 15 s and talks his ear off (plugin command "herbert").
HERBERT_TRAP = "Herbert Moon Trap"
# Beta 2.1: a storm rolls in and three lightning strikes land 5-12 m from John (plugin command "lightning").
LIGHTNING_TRAP = "Lightning Storm Trap"
EXTRA_TRAPS = [SLOWMO_TRAP, STAMPEDE_TRAP, OUT_OF_BULLETS_TRAP, POSSE_TRAP, HERBERT_TRAP, LIGHTNING_TRAP]   # append only (ids)

# Beta 2.1 bounties: completed wanted-poster bounties, counted in the order you finish them ("Bounty 1", "Bounty 2", ...).
# The game has 20; logic expects the first 8 in New Austin, the next 8 in Nuevo Paraiso and the last 4 in West Elizabeth.
MAX_BOUNTY_CHECKS = 20
