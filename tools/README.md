# tools

`make_poptracker.py` builds the PopTracker pack from the APWorld's own item and location tables, so ids always match.
Run it from an Archipelago source checkout with `worlds/rdr1` in place:

    python tools/make_poptracker.py out/RDR-AP_PopTracker

It writes the pack folder and `out/RDR-AP_PopTracker.zip`. Needs Pillow (`pip install pillow`).
