"""Red Dead Redemption (PC) Archipelago client. BETA.

Bridges the Archipelago server and the RedHook plugin through plain files in the game folder:

  rdr1_ap_items.txt   written here (whole list every second): "seed <name>", "option weapon_mode <...>",
                      then "<index>|<item name>|<sender>" for every received item, in order.
  rdr1_ap_seeds/<seed>/rdr1_ap_outbox.txt  written by the plugin: one location name per line (and "GOAL: ..." lines).
                      Read here and sent as LocationChecks. Each multiworld has its own folder, so old runs never leak in.
  rdr1_ap_deaths.txt  written by the plugin when John dies; each line is sent as a DeathLink, then the file is deleted.
  rdr1_ap_in.txt      the plugin's command file. A received DeathLink is written here as "deathlink <text>".

Separate saves: /saves ap and /saves vanilla swap the game's save files (see SaveSwap.py). Game must be closed.

Start it from the Archipelago Launcher ("Red Dead Redemption Client") or run it directly. Set the game folder with
/gamedir <path> if the game is not in the default Steam folder.
"""
import asyncio
import os
import time
import typing

import Utils
from CommonClient import CommonContext, ClientCommandProcessor, get_base_parser, gui_enabled, logger, server_loop
from NetUtils import ClientStatus

from .Data import ANIMAL_TRAPS, CLUSTERS, EXTRA_TRAPS, GAME_NAME, GANG_TRAP, DEAD_EYE_DRAIN_TRAP, DRUNK_TRAP, RANDOM_ANIMAL_TRAP
from .Locations import location_name_to_id, location_table

DEFAULT_GAME_DIR = r"C:\Program Files (x86)\Steam\steamapps\common\Red Dead Redemption"
STORY_MISSIONS = [m for _, missions, _, _ in CLUSTERS for m in missions]
WEAPON_MODES = {0: "vanilla", 1: "cattleman_start", 2: "random_start"}
OUR_TRAPS = ANIMAL_TRAPS + [RANDOM_ANIMAL_TRAP, GANG_TRAP, DEAD_EYE_DRAIN_TRAP, DRUNK_TRAP] + EXTRA_TRAPS

# TrapLink: traps from other games are matched to a Red Dead trap by words in their name; anything else becomes a random trap.
TRAP_KEYWORDS = [
    (("lightning", "thunder", "storm", "weather", "bomb", "explo", "rain"), "Lightning Storm Trap"),
    (("slow", "freeze", "ice", "frozen", "stun", "paraly", "sticky", "gravity", "timer", "time"), "Slow Motion Trap"),
    (("confus", "reverse", "invert", "drunk", "dizzy", "spin", "flip", "mirror", "control"), "Drunk Trap"),
    (("ammo", "bullet", "disarm", "weapon", "gun", "magazine"), "Out of Bullets Trap"),
    (("bee", "snake", "spider", "wolf", "bear", "animal", "creature"), "Random Animal Ambush Trap"),
    (("stampede", "cow", "cattle", "herd", "buffalo"), "Stampede Trap"),
    (("police", "law", "cop", "sheriff", "army", "guard", "posse"), "Lawmen Posse Trap"),
    (("enemy", "ambush", "monster", "spawn", "attack", "goomba", "gang"), "Gang Attack Trap"),
    (("text", "literature", "dialog", "phone", "tip", "chat", "message", "talk", "voice", "pong", "tiny"), "Herbert Moon Trap"),
    (("energy", "mana", "magic", "dead eye", "deadeye", "focus", "drain", "sleep", "blind", "dark"), "Dead Eye Drain Trap"),
]


TALLY_LABELS = [("story", "Story"), ("stranger", "Strangers"), ("hideout", "Hideouts"), ("gang_kill", "Gang Kills"),
                ("hat_trick", "Hat Tricks"), ("minigame", "Minigames"), ("bounty", "Bounties"), ("shop", "Shops")]
LOCATION_CATEGORY = {data.code: data.category for data in location_table.values()}


def match_trap(name: str) -> str:
    if name in OUR_TRAPS:
        return name
    import re
    low = name.lower()
    tokens = re.findall(r"[a-z]+", low)
    for words, trap in TRAP_KEYWORDS:
        for w in words:
            if (" " in w and w in low) or any(tok.startswith(w) for tok in tokens):
                return trap
    return ""


class RDRCommandProcessor(ClientCommandProcessor):
    def _cmd_gamedir(self, path: str = "") -> bool:
        """Show or set the Red Dead Redemption game folder (where RDR-AP.red is)."""
        if path:
            self.ctx.game_dir = path.strip('"')
            Utils.persistent_store("client", "rdr1_game_dir", self.ctx.game_dir)
        logger.info(f"Game folder: {self.ctx.game_dir} ({'found' if os.path.isdir(self.ctx.game_dir) else 'NOT FOUND'})")
        return True

    def _cmd_saves(self, *args: str) -> bool:
        """Separate saves (automatic by default): /saves ap, /saves vanilla, /saves auto on|off, /saves (status).
        Only while the game is closed. Every swap keeps a backup in rdr1_ap_saves/_backups."""
        from .SaveSwap import SaveSwap
        which = " ".join(args).strip().lower()
        swap = SaveSwap(self.ctx.game_dir)
        if which == "ap":
            if not self.ctx.room_seed:
                logger.info("Connect to the multiworld first.")
                return True
            seed = "".join(c if (c.isalnum() or c in "-_") else "_" for c in str(self.ctx.room_seed))
            logger.info(swap.to_ap(seed))
        elif which == "vanilla":
            logger.info(swap.to_vanilla())
        elif which in ("auto on", "auto off", "auto"):
            on = which != "auto off"
            Utils.persistent_store("client", "rdr1_auto_saves", on)
            logger.info(f"Automatic save swapping {'on' if on else 'off'}.")
        else:
            state = swap.state()
            logger.info("Saves in place: " + ("multiworld " + state[3:] if state.startswith("ap ") else "your normal saves"))
        return True

    def _cmd_resync(self) -> bool:
        """Re-send every location the plugin has found (safe to use any time)."""
        self.ctx.sent_lines = 0
        logger.info("All locations from the outbox will be sent again.")
        return True


class RDRContext(CommonContext):
    game = GAME_NAME
    items_handling = 0b111  # every item, including our own and the starting items
    command_processor = RDRCommandProcessor

    def __init__(self, server_address: typing.Optional[str], password: typing.Optional[str]):
        super().__init__(server_address, password)
        self.game_dir: str = Utils.persistent_load().get("client", {}).get("rdr1_game_dir", DEFAULT_GAME_DIR)
        self.slot_data: typing.Dict[str, typing.Any] = {}
        self.sent_lines = 0
        self.goal_sent = False
        self.last_items_text = ""
        self.wrote_once = False
        self.trap_index: typing.Optional[int] = None   # TrapLink: items before this index were already there at connect

    def path(self, name: str) -> str:
        return os.path.join(self.game_dir, name)

    def seed_path(self, name: str) -> str:
        # the plugin keeps every multiworld in its own folder: rdr1_ap_seeds/<seed>/ (same character rule as the plugin)
        clean = "".join(c if (c.isalnum() or c in "-_") else "_" for c in str(self.room_seed))
        return os.path.join(self.game_dir, "rdr1_ap_seeds", clean, name)

    async def server_auth(self, password_requested: bool = False):
        if password_requested and not self.password:
            await super().server_auth(password_requested)
        await self.get_username()
        await self.send_connect()

    def on_package(self, cmd: str, args: dict):
        if cmd == "Connected":
            self.slot_data = args.get("slot_data", {}) or {}
            self.sent_lines = 0
            self.goal_sent = False
            self.last_items_text = ""
            self.trap_index = None
            asyncio.create_task(self.update_death_link(bool(self.slot_data.get("death_link", False))))
            asyncio.create_task(self.update_trap_link(bool(self.slot_data.get("trap_link", False))))
            logger.info(f"Connected. Game folder: {self.game_dir}")
            self.auto_saves_in()
            if not os.path.isdir(self.game_dir):
                logger.warning("Game folder not found. Use /gamedir <path to the Red Dead Redemption folder>.")

        elif cmd == "Bounced":
            tags = args.get("tags", [])
            data = args.get("data", {}) or {}
            if "TrapLink" in tags and "TrapLink" in self.tags and data.get("source") != self.player_names.get(self.slot):
                self.receive_trap_link(str(data.get("trap_name", "")), str(data.get("source", "Someone")))

    # ------------------------------------------------------------------ TrapLink and "found" pop-ups

    async def update_trap_link(self, on: bool) -> None:
        old = self.tags.copy()
        if on:
            self.tags.add("TrapLink")
        else:
            self.tags -= {"TrapLink"}
        if old != self.tags and self.server and not self.server.socket.closed:
            await self.send_msgs([{"cmd": "ConnectUpdate", "tags": self.tags}])

    def game_command(self, line: str) -> None:
        line = line.replace("\n", " ").replace("\r", " ")
        try:
            with open(self.path("rdr1_ap_in.txt"), "a", encoding="utf-8") as f:
                f.write(line + "\n")
        except OSError as e:
            logger.error(f"Could not pass a message to the game: {e}")

    def receive_trap_link(self, trap_name: str, source: str) -> None:
        trap = match_trap(trap_name)
        weights = self.slot_data.get("trap_weights", {})
        if not trap or weights.get(trap, 1) == 0:
            enabled = [t for t, w in weights.items() if w > 0] or [RANDOM_ANIMAL_TRAP]
            import random
            trap = random.choice(enabled)
        logger.info(f"TrapLink: {trap_name} from {source} -> {trap}")
        self.game_command(f"traplink {trap}|{source}")

    async def send_trap_links(self) -> None:
        """Every trap that arrives from the server (after connecting) is passed on to the other TrapLink players."""
        count = len(self.items_received)
        if self.trap_index is None:
            self.trap_index = count
            return
        if "TrapLink" not in self.tags or count <= self.trap_index:
            self.trap_index = count
            return
        for item in self.items_received[self.trap_index:count]:
            name = self.item_names.lookup_in_game(item.item)
            if name in OUR_TRAPS:
                await self.send_msgs([{"cmd": "Bounce", "tags": ["TrapLink"],
                                       "data": {"time": time.time(), "source": self.player_names.get(self.slot, ""),
                                                "trap_name": name}}])
                logger.info(f"TrapLink: sent {name}")
        self.trap_index = count

    def on_print_json(self, args: dict) -> None:
        super().on_print_json(args)
        if args.get("type") != "ItemSend" or not self.slot_data.get("found_item_popups", True) or self.slot is None:
            return
        item = args.get("item")
        receiving = args.get("receiving")
        if item is None or receiving is None or getattr(item, "player", None) != self.slot or receiving == self.slot:
            return
        try:
            name = self.item_names.lookup_in_slot(item.item, receiving)
        except Exception:
            name = "an item"
        who = self.player_names.get(receiving, "someone")
        self.game_command(f"found Found {name} for {who}")

    # ------------------------------------------------------------------ automatic save swap

    def auto_saves(self) -> bool:
        return bool(Utils.persistent_load().get("client", {}).get("rdr1_auto_saves", True))

    def auto_saves_in(self) -> None:
        """On connect: put this multiworld's saves in place, if the game is closed (it cannot be done safely while it runs)."""
        if not self.auto_saves() or not self.room_seed:
            return
        from .SaveSwap import SaveSwap, game_running
        swap = SaveSwap(self.game_dir)
        seed = "".join(c if (c.isalnum() or c in "-_") else "_" for c in str(self.room_seed))
        if swap.state() == f"ap {seed}":
            logger.info("Archipelago saves for this multiworld are in place.")
            return
        if game_running():
            logger.warning("The game is already running, so the Archipelago saves were NOT swapped in. "
                           "Next time start this client first, then the game (or close the game and type /saves ap).")
            return
        if swap.state().startswith("ap "):
            logger.info(swap.to_vanilla())
        logger.info(swap.to_ap(seed))

    def auto_saves_out(self) -> None:
        """On exit: give the normal saves back, if the game is closed."""
        if not self.auto_saves():
            return
        from .SaveSwap import SaveSwap, game_running
        swap = SaveSwap(self.game_dir)
        if swap.state().startswith("ap "):
            if game_running():
                logger.warning("Game still running: your normal saves come back next time the client starts with the game closed.")
            else:
                logger.info(swap.to_vanilla())

    def on_deathlink(self, data: typing.Dict[str, typing.Any]) -> None:
        super().on_deathlink(data)
        cause = data.get("cause") or f"{data.get('source', 'Someone')} died."
        try:
            with open(self.path("rdr1_ap_in.txt"), "a", encoding="utf-8") as f:
                f.write(f"deathlink {cause}\n")
        except OSError as e:
            logger.error(f"Could not pass the DeathLink to the game: {e}")

    # ------------------------------------------------------------------ file bridge

    @property
    def room_seed(self) -> typing.Optional[str]:
        # newer Archipelago keeps the server's seed in server_seed_name (seed_name stays None for clients without a patch)
        return getattr(self, "server_seed_name", None) or self.seed_name

    def write_items(self) -> None:
        if not self.wrote_once:
            logger.info(f"Bridge running: items -> {self.path('rdr1_ap_items.txt')}")
            self.wrote_once = True
        lines = [f"seed {self.room_seed}"]
        mode = WEAPON_MODES.get(self.slot_data.get("weapon_mode", 1), "cattleman_start")
        lines.append(f"option weapon_mode {mode}")
        lines.append(f"option sounds {'on' if self.slot_data.get('popup_sounds', True) else 'off'}")
        lines.append(f"option found_popups {'on' if self.slot_data.get('found_item_popups', True) else 'off'}")
        lines.append(f"option gang_kill_checks {self.slot_data.get('gang_kill_checks', 10)}")
        lines.append(f"option hat_tricks {self.slot_data.get('hat_tricks', 15)}")
        lines += self.tally_lines()
        for index, item in enumerate(self.items_received, start=1):
            name = self.item_names.lookup_in_game(item.item)
            sender = self.player_names.get(item.player, "") if item.player != self.slot else ""
            lines.append(f"{index}|{name}|{sender}")
        text = "\n".join(lines) + "\n"
        if text == self.last_items_text:
            return
        tmp = self.path("rdr1_ap_items.tmp")
        with open(tmp, "w", encoding="utf-8") as f:
            f.write(text)
        os.replace(tmp, self.path("rdr1_ap_items.txt"))  # atomic: the plugin never sees half a file
        self.last_items_text = text

    def tally_lines(self) -> typing.List[str]:
        """F9 in game: checks done / total for each category of this seed, all checks, and items received."""
        done = set(self.checked_locations)
        every = done | set(self.missing_locations)
        lines = []
        for category, label in TALLY_LABELS:
            ids = [i for i in every if LOCATION_CATEGORY.get(i) == category]
            if ids:
                lines.append(f"tally {label}|{sum(1 for i in ids if i in done)}|{len(ids)}")
        if every:
            lines.append(f"tally All checks|{len(done & every)}|{len(every)}")
        lines.append(f"tally Items received|{len(self.items_received)}|")
        names = [self.item_names.lookup_in_game(item.item) for item in self.items_received]
        cash = sum(int(n[1:].split(" ")[0]) for n in names if n.startswith("$") and n.endswith(" Cash"))
        if cash:
            lines.append(f"tally Cash received|${cash}|")
        traps = [n for n in names if n in OUR_TRAPS]
        if traps:
            # "Traps survived 9 (Herbert Moon 4, Stampede 2, ...)", most frequent first
            counts: typing.Dict[str, int] = {}
            for n in traps:
                short = "Animal Ambush" if n.endswith("Ambush Trap") else n[:-5] if n.endswith(" Trap") else n
                counts[short] = counts.get(short, 0) + 1
            detail = ", ".join(f"{k} {v}" for k, v in sorted(counts.items(), key=lambda kv: (-kv[1], kv[0])))
            lines.append(f"tally Traps survived|{len(traps)} ({detail})|")
        return lines

    async def read_outbox(self) -> None:
        try:
            with open(self.seed_path("rdr1_ap_outbox.txt"), encoding="utf-8") as f:
                lines = [line.strip() for line in f if line.strip()]
        except FileNotFoundError:
            return
        if len(lines) < self.sent_lines:
            self.sent_lines = 0  # the file was replaced: send everything again (the server ignores repeats)
        new = lines[self.sent_lines:]
        self.sent_lines = len(lines)
        ids = []
        goal_line = False
        for line in new:
            if line.startswith("GOAL:"):
                goal_line = True
                continue
            loc = location_name_to_id.get(line)
            if loc is None:
                logger.info(f"Game reported '{line}', which is not a location in this seed (ignored).")
            elif loc in self.missing_locations:
                ids.append(loc)
        if ids:
            await self.send_msgs([{"cmd": "LocationChecks", "locations": ids}])
        await self.check_goal(goal_line)

    async def check_goal(self, epilogue_done: bool) -> None:
        if self.goal_sent or not self.slot_data:
            return
        full_story = self.slot_data.get("goal", 0) == 1
        story_ids = [location_name_to_id[m] for m in STORY_MISSIONS]
        story_done = all(i in self.checked_locations for i in story_ids)
        if story_done and (not full_story or epilogue_done or self.epilogue_seen()):
            self.goal_sent = True
            await self.send_msgs([{"cmd": "StatusUpdate", "status": ClientStatus.CLIENT_GOAL}])
            logger.info("Goal complete!")

    def epilogue_seen(self) -> bool:
        try:
            with open(self.seed_path("rdr1_ap_outbox.txt"), encoding="utf-8") as f:
                return any(line.startswith("GOAL: Remember My Family") for line in f)
        except FileNotFoundError:
            return False

    async def send_deaths(self) -> None:
        path = self.path("rdr1_ap_deaths.txt")
        if not os.path.exists(path):
            return
        try:
            with open(path, encoding="utf-8") as f:
                messages = [line.strip() for line in f if line.strip()]
            os.remove(path)
        except OSError:
            return
        if "DeathLink" in self.tags:
            for message in messages:
                await self.send_death(f"{self.player_names.get(self.slot, 'John')} {message}")


async def game_loop(ctx: RDRContext) -> None:
    while not ctx.exit_event.is_set():
        try:
            if ctx.server and ctx.slot is not None and ctx.room_seed and os.path.isdir(ctx.game_dir):
                ctx.write_items()
                await ctx.read_outbox()
                await ctx.send_deaths()
                await ctx.send_trap_links()
        except Exception as e:  # keep the bridge alive whatever happens
            logger.exception(f"Bridge error: {e}")
        await asyncio.sleep(1.0)


def launch(*args: str) -> None:
    async def main(args) -> None:
        ctx = RDRContext(args.connect, args.password)
        ctx.server_task = asyncio.create_task(server_loop(ctx), name="server loop")
        loop = asyncio.create_task(game_loop(ctx), name="RDR1 game loop")
        if gui_enabled:
            ctx.run_gui()
        ctx.run_cli()
        await ctx.exit_event.wait()
        loop.cancel()
        try:
            ctx.auto_saves_out()
        except Exception as e:
            logger.error(f"Could not restore your normal saves automatically: {e}. Use /saves vanilla.")
        await ctx.shutdown()

    import colorama

    parser = get_base_parser(description="Red Dead Redemption Archipelago client (beta).")
    parsed = parser.parse_args(args)
    colorama.just_fix_windows_console()
    asyncio.run(main(parsed))
    colorama.deinit()
