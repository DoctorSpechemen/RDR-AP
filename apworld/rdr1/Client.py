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
import typing

import Utils
from CommonClient import CommonContext, ClientCommandProcessor, get_base_parser, gui_enabled, logger, server_loop
from NetUtils import ClientStatus

from .Data import CLUSTERS, GAME_NAME
from .Locations import location_name_to_id

DEFAULT_GAME_DIR = r"C:\Program Files (x86)\Steam\steamapps\common\Red Dead Redemption"
STORY_MISSIONS = [m for _, missions, _, _ in CLUSTERS for m in missions]
WEAPON_MODES = {0: "vanilla", 1: "cattleman_start", 2: "random_start"}


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
            asyncio.create_task(self.update_death_link(bool(self.slot_data.get("death_link", False))))
            logger.info(f"Connected. Game folder: {self.game_dir}")
            self.auto_saves_in()
            if not os.path.isdir(self.game_dir):
                logger.warning("Game folder not found. Use /gamedir <path to the Red Dead Redemption folder>.")

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
