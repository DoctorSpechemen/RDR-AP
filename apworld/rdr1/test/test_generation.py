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
    options = {"bounty_checks": 20, "lightning_storm_trap_weight": 50, "trap_percentage": 50,
               "found_item_popups": False, "popup_sounds": False}

    def test_bounties_in_seed(self):
        names = {loc.name for loc in self.multiworld.get_locations(self.player)}
        for i in range(1, 21):
            self.assertIn(f"Bounty {i}", names)

    def test_slot_data(self):
        data = self.world.fill_slot_data()
        self.assertNotIn("trap_link", data)
        self.assertFalse(data["found_item_popups"])
        self.assertIn("Lightning Storm Trap", data["trap_weights"])


class TestNoBounties(RDRTestBase):
    options = {"bounty_checks": 0}

    def test_no_bounties(self):
        names = {loc.name for loc in self.multiworld.get_locations(self.player)}
        self.assertFalse(any(n.startswith("Bounty ") and n[7:].isdigit() for n in names))  # not "Bounty Hunters: 5 Kills"



class TestNoHerbert(RDRTestBase):
    options = {"trap_percentage": 100}

    def test_herbert_never_placed(self):
        items = [item.name for item in self.multiworld.itempool if item.player == self.player]
        self.assertTrue(any("Trap" in n for n in items))
        self.assertNotIn("Herbert Moon Trap", items)
        self.assertNotIn("Herbert Moon Trap", self.world.fill_slot_data()["trap_weights"])


class TestRickettsNeedsTargetPainting(RDRTestBase):
    options = {"weapon_mode": "vanilla"}

    def test_gunslinger_needs_dead_eye_3(self):
        self.collect_all_but(["Progressive Dead Eye"])
        dead_eye = self.get_items_by_name("Progressive Dead Eye")
        self.collect(dead_eye[:2])
        self.assertTrue(self.can_reach_location("Civilization, At Any Price"))   # rest of Mexico: Dead Eye 1
        self.assertFalse(self.can_reach_location("The Gunslinger's Tragedy"))
        self.assertFalse(self.can_reach_location("Cowards Die Many Times"))      # comes after Landon's missions
        self.collect(dead_eye[2:3])
        self.assertTrue(self.can_reach_location("The Gunslinger's Tragedy"))
        self.assertTrue(self.can_reach_location("Cowards Die Many Times"))


class TestDuelTrap(RDRTestBase):
    options = {"trap_percentage": 100, "duel_trap_weight": 50}

    def test_duel_trap_in_pool(self):
        from ..Items import item_table
        items = [item.name for item in self.multiworld.itempool if item.player == self.player]
        self.assertIn("Duel Trap", items)
        self.assertEqual(self.world.fill_slot_data()["trap_weights"]["Duel Trap"], 50)
        # ids are append-only: the duel trap comes after the consumables
        self.assertGreater(item_table["Duel Trap"].code, item_table["Chewing Tobacco"].code)


class TestDuelTrapOff(RDRTestBase):
    options = {"trap_percentage": 100, "duel_trap_weight": 0}

    def test_no_duel_trap_at_weight_0(self):
        items = [item.name for item in self.multiworld.itempool if item.player == self.player]
        self.assertNotIn("Duel Trap", items)


class TestRebelKillsAfterReyes(RDRTestBase):
    options = {"weapon_mode": "vanilla", "gang_kill_checks": 5}

    def test_rebel_kills_need_dead_eye_3(self):
        self.collect_all_but(["Progressive Dead Eye"])
        dead_eye = self.get_items_by_name("Progressive Dead Eye")
        self.collect(dead_eye[:2])
        self.assertTrue(self.can_reach_location("Banditos: 5 Kills"))
        self.assertFalse(self.can_reach_location("Reyes' Rebels: 5 Kills"))
        self.collect(dead_eye[2:3])
        self.assertTrue(self.can_reach_location("Reyes' Rebels: 5 Kills"))


class TestRanks(RDRTestBase):
    options = {"fame_ranks": True, "honor_ranks": "bad"}

    def test_rank_locations(self):
        names = {loc.name for loc in self.multiworld.get_locations(self.player)}
        self.assertIn("Fame Rank: Legend", names)
        self.assertIn("Honor Rank: Desperado", names)
        self.assertNotIn("Honor Rank: Hero", names)
        self.assertEqual(self.world.fill_slot_data()["honor_ranks"], "bad")


class TestRanksOff(RDRTestBase):
    options = {"fame_ranks": False, "honor_ranks": "off"}

    def test_no_rank_locations(self):
        names = {loc.name for loc in self.multiworld.get_locations(self.player)}
        self.assertFalse(any(n.startswith(("Fame Rank", "Honor Rank")) for n in names))


class TestDusterEarly(RDRTestBase):
    options = {"include_outfits": True}

    def test_duster_is_early(self):
        self.assertEqual(self.multiworld.early_items[self.player].get("Outfit: Duster Coat"), 1)
        self.assertTrue(any(i.name == "Outfit: Duster Coat" for i in self.multiworld.itempool if i.player == self.player))


class TestMinigameAreas(RDRTestBase):
    def test_liars_dice_and_arm_wrestling_gated(self):
        dice = self.multiworld.get_location("Liar's Dice: Win 1", self.player)
        arm = self.multiworld.get_location("Arm Wrestling: Win 1", self.player)
        poker = self.multiworld.get_location("Poker: Win 1", self.player)
        self.assertEqual(dice.parent_region.name, "Irish and Friends")
        self.assertEqual(arm.parent_region.name, "Nuevo Paraiso")
        self.assertEqual(poker.parent_region.name, "New Austin")
