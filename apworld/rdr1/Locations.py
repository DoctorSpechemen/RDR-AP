from typing import Dict, NamedTuple

from BaseClasses import Location

from .Data import (BASE_ID, BOUNTIES, CHALLENGES, CLUSTERS, GAME_NAME, GANG_FACTIONS, HIDEOUTS, MAX_CHALLENGE_RANKS,
                   MAX_GANG_KILLS, MAX_HAT_TRICKS, MAX_MINIGAME_WINS, MAX_RANDOM_ENCOUNTERS, MAX_STAT_MILESTONES, OLD_MAX_HAT_TRICKS, MAX_BOUNTY_CHECKS, MINIGAMES, MINIGAME_AREA, SHOP_LOCATIONS, LATE_SHOP_LOCATIONS, STRANGERS,
                   FAME_RANKS, GOOD_HONOR_RANKS, BAD_HONOR_RANKS)


class RDRLocation(Location):
    game = GAME_NAME


class LocData(NamedTuple):
    code: int
    category: str  # story, stranger, hideout, gang_kill, hat_trick, minigame, shop
    area: str      # story: cluster name; everything else: area name
    index: int = 0  # 1-based position inside its group (used by "up to N" options)
    group: str = ""


def _build() -> Dict[str, LocData]:
    table: Dict[str, LocData] = {}
    code = BASE_ID

    def add(name: str, category: str, area: str, index: int = 0, group: str = "") -> None:
        nonlocal code
        table[name] = LocData(code, category, area, index, group)
        code += 1

    # Order here defines location ids: only ever append new groups at the end.
    for cluster, missions, _, _ in CLUSTERS:
        for mission in missions:
            add(mission, "story", cluster)
    for area, names in STRANGERS.items():
        for name in names:
            add(name, "stranger", area)
    for area, hideouts in HIDEOUTS.items():
        for hideout in hideouts:
            add(f"{hideout} Hideout: Cleared", "hideout", area, 0, hideout)
    for gang, (area, _faction) in GANG_FACTIONS.items():
        for i in range(1, MAX_GANG_KILLS + 1):
            add(f"{gang}: {i * 5} Kills", "gang_kill", area, i, gang)  # i-th check = 5*i kills (5..50)
    for i in range(1, OLD_MAX_HAT_TRICKS + 1):
        add(f"Hat Trick {i}", "hat_trick", "New Austin", i)
    for game in MINIGAMES:
        for i in range(1, MAX_MINIGAME_WINS + 1):
            add(f"{game}: Win {i}", "minigame", MINIGAME_AREA.get(game, "New Austin"), i, game)
    # Removed in beta 2 (could not be detected reliably): bounties, challenge ranks, random encounters, fame/honor milestones.
    # Their ids stay reserved so the ids after them never move.
    code += sum(BOUNTIES.values()) + len(CHALLENGES) * MAX_CHALLENGE_RANKS + MAX_RANDOM_ENCOUNTERS
    for name, area in SHOP_LOCATIONS.items():
        add(name, "shop", area)
    code += 2 * MAX_STAT_MILESTONES
    # Beta 2: hat tricks 6..15 (appended so earlier ids do not move)
    for i in range(OLD_MAX_HAT_TRICKS + 1, MAX_HAT_TRICKS + 1):
        add(f"Hat Trick {i}", "hat_trick", "New Austin", i)
    # Beta 2.1: bounties in the order they are completed
    for i in range(1, MAX_BOUNTY_CHECKS + 1):
        add(f"Bounty {i}", "bounty", "New Austin" if i <= 8 else ("Nuevo Paraiso" if i <= 16 else "West Elizabeth"), i)
    # Beta 3.0: Bandito and Rebel scraps
    for name, area in LATE_SHOP_LOCATIONS.items():
        add(name, "shop", area)
    # Beta 3.0: fame and honor ranks (group = fame / honor_good / honor_bad)
    for name, (_cut, area) in FAME_RANKS.items():
        add(f"Fame Rank: {name}", "rank", area, 0, "fame")
    for name, (_cut, area) in GOOD_HONOR_RANKS.items():
        add(f"Honor Rank: {name}", "rank", area, 0, "honor_good")
    for name, (_cut, area) in BAD_HONOR_RANKS.items():
        add(f"Honor Rank: {name}", "rank", area, 0, "honor_bad")
    return table


location_table: Dict[str, LocData] = _build()
location_name_to_id: Dict[str, int] = {name: data.code for name, data in location_table.items()}

location_name_groups = {}
for _name, _data in location_table.items():
    location_name_groups.setdefault(_data.category.replace("_", " ").title(), set()).add(_name)
