from . import RDRTestBase


class TestDefaults(RDRTestBase):
    """Default options: John story goal, Cattleman start."""


class TestFullStoryEverything(RDRTestBase):
    options = {
        "goal": "full_story",
        "gang_kill_checks": 10,
        "hat_tricks": 15,
    }


class TestVanillaWeapons(RDRTestBase):
    options = {"weapon_mode": "vanilla"}


class TestRandomStartWeapon(RDRTestBase):
    options = {"weapon_mode": "random_start", "trap_percentage": 50}


class TestMinimal(RDRTestBase):
    options = {
        "include_stranger_missions": False,
        "include_gang_hideouts": False,
        "hat_tricks": 0,
        "minigame_wins": 0,
        "weapon_mode": "vanilla",
    }

    def test_final_mission_needs_only_dead_eye(self):
        mission = "The Last Enemy That Shall Be Destroyed"
        self.collect_all_but(["Progressive Dead Eye"])
        self.assertFalse(self.can_reach_location(mission))
        for _ in range(4):
            self.collect_by_name("Progressive Dead Eye")
        self.assertTrue(self.can_reach_location(mission))

    def test_no_mission_items(self):
        names = {item.name for item in self.multiworld.itempool} | {item.name for item in self.multiworld.precollected_items[self.player]}
        self.assertNotIn("Armadillo Arrival", names)
        self.assertNotIn("The Burning", names)
        self.assertTrue(self.can_reach_location("The Burning"))  # chapter 1 needs nothing


class TestManyTraps(RDRTestBase):
    options = {"trap_percentage": 60}


class TestBeta21Features(RDRTestBase):
    options = {"bounty_checks": 20, "trap_link": True, "lightning_storm_trap_weight": 50, "trap_percentage": 50,
               "found_item_popups": False, "popup_sounds": False}

    def test_bounties_in_seed(self):
        names = {loc.name for loc in self.multiworld.get_locations(self.player)}
        for i in range(1, 21):
            self.assertIn(f"Bounty {i}", names)

    def test_slot_data(self):
        data = self.world.fill_slot_data()
        self.assertTrue(data["trap_link"])
        self.assertFalse(data["found_item_popups"])
        self.assertIn("Lightning Storm Trap", data["trap_weights"])


class TestNoBounties(RDRTestBase):
    options = {"bounty_checks": 0}

    def test_no_bounties(self):
        names = {loc.name for loc in self.multiworld.get_locations(self.player)}
        self.assertFalse(any(n.startswith("Bounty ") and n[7:].isdigit() for n in names))  # not "Bounty Hunters: 5 Kills"

