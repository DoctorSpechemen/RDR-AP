from dataclasses import dataclass

from Options import Choice, DeathLink, DefaultOnToggle, OptionGroup, PerGameCommonOptions, Range

from .Data import MAX_BOUNTY_CHECKS, MAX_GANG_KILLS, MAX_HAT_TRICKS, MAX_MINIGAME_WINS


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
    """Finishing each stranger's whole story is a check (sent when the story ends)."""
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
    """Number of win checks for each minigame (poker, blackjack, liar's dice, five finger fillet, horseshoes, arm wrestling,
    duels). 0 disables them. Each won hand, game or duel sends the next win check."""
    display_name = "Minigame Win Checks"
    range_start = 0
    range_end = MAX_MINIGAME_WINS
    default = 3


class BountyChecks(Range):
    """Number of bounty checks: completing wanted-poster bounties (dead or alive), counted in the order you finish them.
    0 disables them."""
    display_name = "Bounty Checks"
    range_start = 0
    range_end = MAX_BOUNTY_CHECKS
    default = 5


class FameRanks(DefaultOnToggle):
    """Reaching each fame title is a check: Greenhorn, Buckaroo, Mercenary, Gunslinger and Legend."""
    display_name = "Fame Rank Checks"


class HonorRanks(Choice):
    """Reaching honor titles is a check. good: Honest Joe, Peacemaker, Hero. bad: Rustler, Road Agent, Desperado (play an
    outlaw). off: none."""
    display_name = "Honor Rank Checks"
    option_off = 0
    option_good = 1
    option_bad = 2
    default = 1


class IncludeShops(DefaultOnToggle):
    """Buying gear, maps, guns and tailor items in the general stores, gunsmiths and tailors of every town is a check (one check per item, buy it in
    any store that has it). Horse deeds are blocked; consumables are left alone."""
    display_name = "Include Shop Purchases"


class IncludeOutfits(DefaultOnToggle):
    """Outfits are sent to you as items (each one once). Purely cosmetic. Outfits you get any other way (bought in a shop,
    given by the game) are taken away again."""
    display_name = "Include Outfits"


class FoundItemPopups(DefaultOnToggle):
    """Show a short line at the bottom of the screen when one of your checks sends an item to another player
    ("Found Hookshot for Alice")."""
    display_name = "Found Item Popups"


class PopupSounds(DefaultOnToggle):
    """Play a game sound with item pop-ups and trap warnings."""
    display_name = "Pop-up Sounds"


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
    """Relative weight of the out of bullets trap (empties your ammo; half of it comes back after 20 seconds)."""
    display_name = "Out of Bullets Trap Weight"
    range_start = 0
    range_end = 100
    default = 8


class PosseTrapWeight(Range):
    """Relative weight of the lawmen posse trap (a group of lawmen attacks)."""
    display_name = "Lawmen Posse Trap Weight"
    range_start = 0
    range_end = 100
    default = 8



class LightningStormTrapWeight(Range):
    """Relative weight of the lightning trap (three lightning strikes land close to John: keep moving)."""
    display_name = "Lightning Storm Trap Weight"
    range_start = 0
    range_end = 100
    default = 6


class DuelTrapWeight(Range):
    """Relative weight of the duel trap ("A Challenger Approaches..."): a gunman calls you out and the game's duel starts.
    Win and it counts as a duel win; lose and you die (DeathLink sends it on). The trap waits until you are on foot."""
    display_name = "Duel Trap Weight"
    range_start = 0
    range_end = 100
    default = 5


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
    bounty_checks: BountyChecks
    fame_ranks: FameRanks
    honor_ranks: HonorRanks
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
    lightning_storm_trap_weight: LightningStormTrapWeight
    duel_trap_weight: DuelTrapWeight
    found_item_popups: FoundItemPopups
    popup_sounds: PopupSounds
    death_link: DeathLink


# Sections of the template YAML and the WebHost options page. Options not listed here land in "Game Options".
rdr_option_groups = [
    OptionGroup("Goal and Weapons", [Goal, WeaponMode]),
    OptionGroup("Checks", [IncludeStrangerMissions, IncludeGangHideouts, GangKillChecks, HatTricks, MinigameWins,
                           BountyChecks, FameRanks, HonorRanks, IncludeShops]),
    OptionGroup("Items", [IncludeOutfits]),
    OptionGroup("Traps", [TrapPercentage, AnimalTrapWeight, GangTrapWeight, PosseTrapWeight, StampedeTrapWeight,
                          DrunkTrapWeight, SlowMotionTrapWeight, OutOfBulletsTrapWeight,
                          LightningStormTrapWeight, DuelTrapWeight, DeadEyeDrainTrapWeight]),
    OptionGroup("Pop-ups", [FoundItemPopups, PopupSounds]),
    OptionGroup("Multiplayer", [DeathLink]),
]
