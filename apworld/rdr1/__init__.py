from typing import Any, Dict, List, Set

from BaseClasses import ItemClassification, Region, Tutorial
from Options import OptionError
from worlds.AutoWorld import WebWorld, World

from .Data import (ALL_FILLERS, FILLER_WEIGHTS, FILLER_DEFAULT_WEIGHT, OUTFITS, ANIMAL_TRAPS, AREA_GATES, CLUSTER_NAMES, CLUSTERS, DEAD_EYE_DRAIN_TRAP, DEAD_EYE_ITEM,
                   DEAD_EYE_LEVELS, DEFAULT_START_WEAPON, DRUNK_TRAP, EXTRA_TRAPS, FILLER_ITEMS, FINAL_CLUSTER, STORY_LOCKED_OUTFITS, GAME_NAME, GANG_TRAP,
                   GUNS,
                   RANDOM_ANIMAL_TRAP, WEAPONS)
from .Items import RDRItem, item_name_groups, item_name_to_id, item_table
from .Locations import RDRLocation, location_name_groups, location_name_to_id, location_table
from .Options import RDROptions, rdr_option_groups

from worlds.LauncherComponents import Component, Type, components, launch_subprocess


def launch_client(*args: str) -> None:
    from .Client import launch
    launch_subprocess(launch, name="RDR1Client", args=args)


components.append(Component("Red Dead Redemption Client", func=launch_client, component_type=Type.CLIENT,
                            game_name="Red Dead Redemption", supports_uri=True))


class RDRWeb(WebWorld):
    theme = "dirt"
    setup_en = Tutorial(
        "Multiworld Setup Guide",
        "A guide to setting up Red Dead Redemption (PC) for Archipelago.",
        "English",
        "setup_en.md",
        "setup/en",
        ["jhastert1"],
    )
    tutorials = [setup_en]
    option_groups = rdr_option_groups


class RDRWorld(World):
    """
    Red Dead Redemption: former outlaw John Marston hunts down his old gang across the American
    frontier and Mexico to win back his family. Story missions play in their normal order and each one is
    a check, Dead Eye upgrades are earned from other worlds, and checks are spread across missions, strangers, gang
    hideouts, hat tricks, minigames, shops and more.
    """
    game = GAME_NAME
    web = RDRWeb()
    options_dataclass = RDROptions
    options: RDROptions
    topology_present = False

    item_name_to_id = item_name_to_id
    location_name_to_id = location_name_to_id
    item_name_groups = item_name_groups
    location_name_groups = location_name_groups

    required_client_version = (0, 5, 0)

    # ------------------------------------------------------------------ helpers

    def _dead_eye_needed(self, chapter: str) -> int:
        """Story chapters are not items: missions play in the vanilla order and every mission is just a check. Logic only
        expects the Dead Eye levels John would have reached by that point of the story (pacing)."""
        return next(c[3] for c in CLUSTERS if c[0] == chapter)

    def _rule(self, chapter: str):
        dead_eye = self._dead_eye_needed(chapter)
        player = self.player
        if dead_eye == 0:
            return None
        return lambda state: state.has(DEAD_EYE_ITEM, player, dead_eye)

    def _location_enabled(self, data) -> bool:
        o = self.options
        cat = data.category
        if cat == "story":
            return True
        if cat == "stranger":
            return bool(o.include_stranger_missions)
        if cat == "hideout":
            return bool(o.include_gang_hideouts)
        if cat == "gang_kill":
            return data.index <= o.gang_kill_checks.value
        if cat == "hat_trick":
            return data.index <= o.hat_tricks.value
        if cat == "minigame":
            return data.index <= o.minigame_wins.value
        if cat == "bounty":
            return data.index <= o.bounty_checks.value
        if cat == "shop":
            return bool(o.include_shops)
        return False

    # ------------------------------------------------------------------ generation

    def generate_early(self) -> None:
        mode = self.options.weapon_mode
        self.starting_weapon = None
        if mode == "cattleman_start":
            self.starting_weapon = DEFAULT_START_WEAPON
        elif mode == "random_start":
            self.starting_weapon = self.random.choice(GUNS)

    def create_regions(self) -> None:
        multiworld, player = self.multiworld, self.player

        menu = Region("Menu", player, multiworld)
        multiworld.regions.append(menu)

        cluster_regions: Dict[str, Region] = {}
        for name in CLUSTER_NAMES:
            region = Region(name, player, multiworld)
            multiworld.regions.append(region)
            cluster_regions[name] = region
            menu.connect(region, f"Menu -> {name}", self._rule(name))

        area_regions: Dict[str, Region] = {}
        for area, gate in AREA_GATES.items():
            region = Region(area, player, multiworld)
            multiworld.regions.append(region)
            area_regions[area] = region
            menu.connect(region, f"Menu -> {area}", self._rule(gate))

        for name, data in location_table.items():
            if not self._location_enabled(data):
                continue
            parent = cluster_regions[data.area] if data.area in cluster_regions else area_regions[data.area]
            parent.locations.append(RDRLocation(player, name, data.code, parent))

        # Goal events
        if self.options.goal == "full_story":
            epilogue = Region("Edgar Ross Epilogue", player, multiworld)
            multiworld.regions.append(epilogue)
            menu.connect(epilogue, "Menu -> Edgar Ross Epilogue", self._rule(FINAL_CLUSTER))
            goal_location = RDRLocation(player, "Edgar Ross Epilogue Complete", None, epilogue)
            epilogue.locations.append(goal_location)
        else:
            region = cluster_regions[FINAL_CLUSTER]
            goal_location = RDRLocation(player, "John Marston's Story Complete", None, region)
            region.locations.append(goal_location)
        goal_location.place_locked_item(RDRItem("Victory", ItemClassification.progression, None, player))
        multiworld.completion_condition[player] = lambda state: state.has("Victory", player)

    def create_item(self, name: str) -> RDRItem:
        data = item_table[name]
        return RDRItem(name, data.classification, data.code, self.player)

    def get_filler_item_name(self) -> str:
        weights = [FILLER_WEIGHTS.get(name, FILLER_DEFAULT_WEIGHT) for name in ALL_FILLERS]
        return self.random.choices(ALL_FILLERS, weights)[0]

    def _random_trap(self) -> str:
        o = self.options
        animals = [RANDOM_ANIMAL_TRAP]   # one trap; the plugin picks the animal (the style option was removed)
        names: List[str] = animals + [GANG_TRAP, DEAD_EYE_DRAIN_TRAP, DRUNK_TRAP] + EXTRA_TRAPS
        weights = [o.animal_trap_weight.value] * len(animals) + [o.gang_trap_weight.value,
                                                                o.dead_eye_drain_trap_weight.value, o.drunk_trap_weight.value,
                                                                o.slowmo_trap_weight.value, o.stampede_trap_weight.value,
                                                                o.out_of_bullets_trap_weight.value, o.posse_trap_weight.value,
                                                                o.herbert_moon_trap_weight.value,
                                                                o.lightning_storm_trap_weight.value]
        if sum(weights) == 0:
            return self.get_filler_item_name()
        return self.random.choices(names, weights)[0]

    def create_items(self) -> None:
        multiworld, player = self.multiworld, self.player

        if self.starting_weapon:
            multiworld.push_precollected(self.create_item(self.starting_weapon))

        pool: List[RDRItem] = []
        pool += [self.create_item(DEAD_EYE_ITEM) for _ in range(DEAD_EYE_LEVELS)]
        if self.options.weapon_mode != "vanilla":
            pool += [self.create_item(w) for w in WEAPONS if w != self.starting_weapon]
        if self.options.include_outfits:
            pool += [self.create_item(o) for o in OUTFITS if o not in STORY_LOCKED_OUTFITS]

        unfilled = len(multiworld.get_unfilled_locations(player))
        remaining = unfilled - len(pool)
        if remaining < 0:
            raise OptionError(f"{self.player_name}: {len(pool)} items but only {unfilled} locations. "
                              f"Enable more check categories.")

        trap_count = round(remaining * self.options.trap_percentage.value / 100)
        for _ in range(trap_count):
            pool.append(self.create_item(self._random_trap()))
        for _ in range(remaining - trap_count):
            pool.append(self.create_item(self.get_filler_item_name()))

        multiworld.itempool += pool

    def fill_slot_data(self) -> Dict[str, Any]:
        o = self.options
        return {
            "goal": o.goal.value,
            "weapon_mode": o.weapon_mode.value,
            "starting_weapon": self.starting_weapon,
            "gang_kill_checks": o.gang_kill_checks.value,
            "hat_tricks": o.hat_tricks.value,
            "minigame_wins": o.minigame_wins.value,
            "include_stranger_missions": bool(o.include_stranger_missions),
            "include_gang_hideouts": bool(o.include_gang_hideouts),
            "include_shops": bool(o.include_shops),
            "death_link": bool(o.death_link),
            "trap_link": bool(o.trap_link),
            "trap_weights": {RANDOM_ANIMAL_TRAP: o.animal_trap_weight.value, GANG_TRAP: o.gang_trap_weight.value,
                             DEAD_EYE_DRAIN_TRAP: o.dead_eye_drain_trap_weight.value, DRUNK_TRAP: o.drunk_trap_weight.value,
                             "Slow Motion Trap": o.slowmo_trap_weight.value, "Stampede Trap": o.stampede_trap_weight.value,
                             "Out of Bullets Trap": o.out_of_bullets_trap_weight.value,
                             "Lawmen Posse Trap": o.posse_trap_weight.value,
                             "Herbert Moon Trap": o.herbert_moon_trap_weight.value,
                             "Lightning Storm Trap": o.lightning_storm_trap_weight.value},
            "found_item_popups": bool(o.found_item_popups),
            "popup_sounds": bool(o.popup_sounds),
            "bounty_checks": o.bounty_checks.value,
        }
