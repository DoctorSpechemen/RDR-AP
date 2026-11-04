"""Separate Archipelago saves for Red Dead Redemption (PC). ALPHA.

The game always reads Documents/Rockstar Games/Red Dead Redemption/Profiles/<id>/RDR2SAVE*.SAV. Instead of hooking the game,
the client SWAPS those files while the game is closed:

  /saves ap       your normal saves are backed up, then this multiworld's own saves are put in place (empty = new game)
  /saves vanilla  this multiworld's saves are stored in its folder, then your normal saves come back
  /saves          show which set is in place

Nothing is ever deleted without a copy first: every swap also leaves a timestamped backup in
<game folder>/rdr1_ap_saves/_backups/. Turn Rockstar cloud saves off for RDR1 while using this, or the launcher may sync the
other set back.
"""
import os
import shutil
import subprocess
import time
import typing

SAVE_PATTERNS = ("RDR2SAVE", "RDR2EXTRAS")


def profile_dir() -> typing.Optional[str]:
    root = os.path.join(os.path.expanduser("~"), "Documents", "Rockstar Games", "Red Dead Redemption", "Profiles")
    if not os.path.isdir(root):
        return None
    profiles = [os.path.join(root, d) for d in os.listdir(root) if os.path.isdir(os.path.join(root, d))]
    if not profiles:
        return None
    return max(profiles, key=os.path.getmtime)  # the most recently used profile


def game_running() -> bool:
    try:
        out = subprocess.run(["tasklist", "/FI", "IMAGENAME eq RDR.exe"], capture_output=True, text=True, timeout=10).stdout
        return "RDR.exe" in out
    except Exception:
        return False  # not on Windows / tasklist missing: assume closed


def save_files(folder: str) -> typing.List[str]:
    if not os.path.isdir(folder):
        return []
    return [f for f in os.listdir(folder) if f.upper().startswith(SAVE_PATTERNS) and f.upper().endswith(".SAV")]


class SaveSwap:
    def __init__(self, game_dir: str):
        self.root = os.path.join(game_dir, "rdr1_ap_saves")
        self.state_file = os.path.join(self.root, "state.txt")

    def state(self) -> str:
        try:
            with open(self.state_file, encoding="utf-8") as f:
                return f.read().strip() or "vanilla"
        except FileNotFoundError:
            return "vanilla"

    def _write_state(self, state: str) -> None:
        os.makedirs(self.root, exist_ok=True)
        with open(self.state_file, "w", encoding="utf-8") as f:
            f.write(state)

    def _move_set(self, src: str, dst: str, label: str) -> int:
        """Copy every save file from src to dst (and a timestamped backup), then remove them from src."""
        files = save_files(src)
        os.makedirs(dst, exist_ok=True)
        backup = os.path.join(self.root, "_backups", time.strftime("%Y%m%d-%H%M%S") + "-" + label)
        os.makedirs(backup, exist_ok=True)
        for f in files:
            shutil.copy2(os.path.join(src, f), os.path.join(backup, f))
            shutil.copy2(os.path.join(src, f), os.path.join(dst, f))
        for f in files:  # only after every copy succeeded
            os.remove(os.path.join(src, f))
        return len(files)

    def _copy_in(self, src: str, dst: str) -> int:
        files = save_files(src)
        for f in files:
            shutil.copy2(os.path.join(src, f), os.path.join(dst, f))
        return len(files)

    def to_ap(self, seed: str) -> str:
        profile = profile_dir()
        if not profile:
            return "No Red Dead Redemption profile folder found."
        if game_running():
            return "Close the game first: saves can only be swapped while it is not running."
        state = self.state()
        if state.startswith("ap "):
            if state == f"ap {seed}":
                return "This multiworld's saves are already in place."
            return f"Another multiworld's saves are in place ({state[3:]}). Use /saves vanilla first."
        n = self._move_set(profile, os.path.join(self.root, "_vanilla"), "vanilla")
        m = self._copy_in(os.path.join(self.root, seed), profile)
        self._write_state(f"ap {seed}")
        return (f"Normal saves put aside ({n} files). " +
                (f"Loaded this multiworld's saves ({m} files)." if m else "No saves for this multiworld yet: start a NEW GAME."))

    def to_vanilla(self) -> str:
        profile = profile_dir()
        if not profile:
            return "No Red Dead Redemption profile folder found."
        if game_running():
            return "Close the game first: saves can only be swapped while it is not running."
        state = self.state()
        if not state.startswith("ap "):
            return "Your normal saves are already in place."
        seed = state[3:]
        n = self._move_set(profile, os.path.join(self.root, seed), "ap-" + seed)
        m = self._copy_in(os.path.join(self.root, "_vanilla"), profile)
        self._write_state("vanilla")
        return f"Multiworld saves stored ({n} files), normal saves restored ({m} files)."
