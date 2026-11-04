from dataclasses import dataclass

from Options import Choice, DeathLink, DefaultOnToggle, OptionGroup, PerGameCommonOptions, Range, Toggle

from .Data import MAX_GANG_KILLS, MAX_HAT_TRICKS, MAX_MINIGAME_WINS


class Goal(Choice):
    """What completes your game.

    John Story: finish all 57 John Marston story missions.
    Full Story: also finish the epilogue where Jack hunts down Edgar Ross ("Remember My Family")."""
    display_name = "Goal"
    option_john_story = 0
    option_full_story = 1
    default = 0


class WeaponMode(Choice):
    """How weapons are handled.

    Vanilla: pick weapons up normally in the world; weapons are not in the item pool.
    Cattleman Start: start with only the Cattleman Revolver; every other weapon is sent to you.
    Random Start: start with one random weapon; every other weapon is sent to you."""
    display_name = "Weapon Mode"
    option_vanilla = 0
    option_cattleman_start = 1
    option_random_start = 2
    default = 1


class IncludeStrangerMissions(DefaultOnToggle):
    """Completing each stranger mission is a check."""
    display_name = "Include Stranger Missions"


class IncludeGangHideouts(DefaultOnToggle):
    """Clearing each gang hideout is a check (Pike's Basin, Twin Rocks, Fort Mercer, Tumbleweed, Gaptooth Breach, Nosalida,
    Tesoro Azul)."""
    display_name = "Include Gang Hideouts"


class GangKillChecks(Range):
    """Number of kill checks for each gang (Walton's Gang, Banditos, etc.; 0 disables them). Each check is a milestone of 5 kills (5, 10, ... up to 50). Kills are counted by the gang the enemy belongs to; scripted mission enemies do not count."""
    display_name = "Gang Member Kill Checks"
    range_start = 0
    range_end = MAX_GANG_KILLS
    default = 5


class HatTricks(Range):
    """Number of hat trick checks (shoot a hat off someone without killing them). 0 disables them."""
    display_name = "Hat Trick Checks"
    range_start = 0
    range_end = MAX_HAT_TRICKS
    default = 5


class MinigameWins(Range):
    """Number of wins that count as checks for each minigame (poker, blackjack, duels, etc). 0 disables them. A won hand / game / duel counts once."""
    display_name = "Minigame Win Checks"
    range_start = 0
    range_end = MAX_MINIGAME_WINS
    default = 3


class IncludeShops(DefaultOnToggle):
    """Buying gear, maps, guns and tailor items in the general stores, gunsmiths and tailors of every town is a check (one check per item, buy it in
    any store that has it). Horse deeds are blocked; consumables are left alone."""
    display_name = "Include Shop Purchases"


class IncludeOutfits(DefaultOnToggle):
    """Outfits are sent to you as items (each one once). Purely cosmetic."""
    display_name = "Include Outfits"


class TrapPercentage(Range):
    """Percentage of filler items that are replaced by traps."""
    display_name = "Trap Percentage"
    range_start = 0
    range_end = 100
    default = 10


class AnimalTrapWeight(Range):
    """Relative weight of the animal ambush trap (a random predator attacks)."""
    display_name = "Animal Ambush Trap Weight"
    range_start = 0
    range_end = 100
    default = 10


class GangTrapWeight(Range):
    """Relative weight of the gang attack trap."""
    display_name = "Gang Attack Trap Weight"
    range_start = 0
    range_end = 100
    default = 20


class DrunkTrapWeight(Range):
    """Relative weight of the drunk trap (John gets drunk for a while)."""
    display_name = "Drunk Trap Weight"
    range_start = 0
    range_end = 100
    default = 10

class SlowMotionTrapWeight(Range):
    """Relative weight of the slow motion trap (time slows for a few seconds)."""
    display_name = "Slow Motion Trap Weight"
    range_start = 0
    range_end = 100
    default = 8


class StampedeTrapWeight(Range):
    """Relative weight of the stampede trap (a herd of harmless animals swarms John)."""
    display_name = "Stampede Trap Weight"
    range_start = 0
    range_end = 100
    default = 8


class OutOfBulletsTrapWeight(Range):
    """Relative weight of the out of bullets trap (empties your ammo)."""
    display_name = "Out of Bullets Trap Weight"
    range_start = 0
    range_end = 100
    default = 8


class HerbertMoonTrapWeight(Range):
    """Relative weight of the Herbert Moon trap ("Herbert Moon speaks sweet nothings into your ear...")."""
    display_name = "Herbert Moon Trap Weight"
    range_start = 0
    range_end = 100
    default = 6


class PosseTrapWeight(Range):
    """Relative weight of the lawmen posse trap (a group of lawmen attacks)."""
    display_name = "Lawmen Posse Trap Weight"
    range_start = 0
    range_end = 100
    default = 8



class DeadEyeDrainTrapWeight(Range):
    """Relative weight of the Dead Eye drain trap (empties your Dead Eye meter). Keep this low to make it rare."""
    display_name = "Dead Eye Drain Trap Weight"
    range_start = 0
    range_end = 100
    default = 3


@dataclass
class RDROptions(PerGameCommonOptions):
    goal: Goal
    weapon_mode: WeaponMode
    include_stranger_missions: IncludeStrangerMissions
    include_gang_hideouts: IncludeGangHideouts
    gang_kill_checks: GangKillChecks
    hat_tricks: HatTricks
    minigame_wins: MinigameWins
    include_shops: IncludeShops
    include_outfits: IncludeOutfits
    trap_percentage: TrapPercentage
    animal_trap_weight: AnimalTrapWeight
    gang_trap_weight: GangTrapWeight
    dead_eye_drain_trap_weight: DeadEyeDrainTrapWeight
    drunk_trap_weight: DrunkTrapWeight
    slowmo_trap_weight: SlowMotionTrapWeight
    stampede_trap_weight: StampedeTrapWeight
    out_of_bullets_trap_weight: OutOfBulletsTrapWeight
    posse_trap_weight: PosseTrapWeight
    herbert_moon_trap_weight: HerbertMoonTrapWeight
    death_link: DeathLink


# Sections of the template YAML and the WebHost options page. Options not listed here land in "Game Options".
rdr_option_groups = [
    OptionGroup("Goal and Weapons", [Goal, WeaponMode]),
    OptionGroup("Checks", [IncludeStrangerMissions, IncludeGangHideouts, GangKillChecks, HatTricks, MinigameWins,
                           IncludeShops]),
    OptionGroup("Items", [IncludeOutfits]),
    OptionGroup("Traps", [TrapPercentage, AnimalTrapWeight, GangTrapWeight, PosseTrapWeight, StampedeTrapWeight,
                          DrunkTrapWeight, SlowMotionTrapWeight, OutOfBulletsTrapWeight, HerbertMoonTrapWeight,
                          DeadEyeDrainTrapWeight]),
    OptionGroup("Multiplayer", [DeathLink]),
]
