"""Builds the RDR-AP PopTracker pack from the APWorld's own tables (ids always match the world).

Run from an Archipelago checkout that has worlds/rdr1:  python make_poptracker.py <output folder>
"""
import json
import os
import sys
import zipfile

from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rdr_icons

import ModuleUpdate
ModuleUpdate.update = lambda *a, **k: None

from worlds.rdr1.Data import AREA_GATES, CLUSTERS, OUTFITS, STORY_LOCKED_OUTFITS, WEAPONS, DEAD_EYE_ITEM, FILLER_ITEMS
from worlds.rdr1.Items import item_table
from worlds.rdr1.Locations import location_table

OUT = sys.argv[1]
VERSION = "0.5.0"
FONT = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"

from worlds.rdr1.Data import CHAPTER_DEAD_EYE
CHAPTER_DE = dict(CHAPTER_DEAD_EYE)
AREA_DE = {area: CHAPTER_DE[chapter] for area, chapter in AREA_GATES.items()}


def dead_eye_for(area: str) -> int:
    return CHAPTER_DE.get(area, AREA_DE.get(area, 0))


def region_of(area: str) -> str:
    if area in AREA_DE:
        return area
    de = CHAPTER_DE[area]
    chapters = [c[0] for c in CLUSTERS]
    i = chapters.index(area)
    if i <= chapters.index("Crossing the Border"):    # the raft to Mexico still starts in New Austin
        return "New Austin"
    if i < chapters.index("Ross's Offer"):
        return "Nuevo Paraiso"
    return "West Elizabeth"


def code(name: str) -> str:
    return "".join(c.lower() if c.isalnum() else "_" for c in name).strip("_")


def icon(path: str, text: str, color) -> None:
    img = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.rounded_rectangle((2, 2, 61, 61), 10, fill=color, outline=(20, 12, 6, 255), width=3)
    words = text.replace("'", "").replace(".", "").split()
    label = "".join(w[0] for w in words[:3]).upper() if len(text) > 4 else text
    font = ImageFont.truetype(FONT, 24 if len(label) <= 2 else 18)
    w = d.textlength(label, font=font)
    d.text(((64 - w) / 2, 18), label, fill=(255, 245, 225, 255), font=font)
    img.save(path)


os.makedirs(OUT, exist_ok=True)
for sub in ("images/items", "images/maps", "items", "locations", "layouts", "maps", "scripts/autotracking"):
    os.makedirs(os.path.join(OUT, sub), exist_ok=True)

# ---------------------------------------------------------------- items
items = []
item_mapping = {}
grid_weapons, grid_outfits = [], []

items.append({"name": "Progressive Dead Eye", "type": "consumable", "img": "images/items/deadeye.png",
              "codes": "deadeye", "max_quantity": 4})
rdr_icons.dead_eye(os.path.join(OUT, "images/items/deadeye.png"))
# one icon per Dead Eye level (they light up in order as Progressive Dead Eye arrives)
DEAD_EYE_LEVEL_NAMES = ["Dead Eye", "Dead Eye: Auto Lock-On", "Dead Eye: Target Painting", "Dead Eye: Fast Recovery"]
for lv, nm in enumerate(DEAD_EYE_LEVEL_NAMES, start=1):
    items.append({"name": nm, "type": "toggle", "img": f"images/items/deadeye_{lv}.png", "codes": f"deadeye_{lv}"})
    rdr_icons.dead_eye(os.path.join(OUT, f"images/items/deadeye_{lv}.png"), lv)
item_mapping[item_table[DEAD_EYE_ITEM].code] = ["deadeye", "consumable"]

for w in WEAPONS:
    c = code(w)
    items.append({"name": w, "type": "toggle", "img": f"images/items/{c}.png", "codes": c})
    rdr_icons.weapon_icon(w, os.path.join(OUT, f"images/items/{c}.png"))
    item_mapping[item_table[w].code] = [c, "toggle"]
    grid_weapons.append(c)

for o in OUTFITS:
    if o in STORY_LOCKED_OUTFITS:
        continue
    c = code(o)
    items.append({"name": o, "type": "toggle", "img": f"images/items/{c}.png", "codes": c})
    rdr_icons.outfit_icon(o, os.path.join(OUT, f"images/items/{c}.png"))
    item_mapping[item_table[o].code] = [c, "toggle"]
    grid_outfits.append(c)

# The story opens the territories: Mexico after "We Shall Be Together in Paradise", West Elizabeth after "An Appointed Time"
# (tester 2026-11-05: Mexico showed as available before the raft mission). Two toggles, set by auto-tracking when that
# mission is checked; without auto-tracking, click them yourself.
AREA_OPEN = {"Nuevo Paraiso": ("mexico_open", "Mexico Open", "We Shall Be Together in Paradise", "MX", (196, 120, 60, 255)),
             "West Elizabeth": ("we_open", "West Elizabeth Open", "An Appointed Time", "WE", (70, 120, 70, 255))}
# Beta 3.0: story points that open a minigame (Liar's Dice after the first Irish mission; Arm Wrestling uses Mexico Open).
STORY_OPEN = {"dice_open": ("Liar's Dice Open", "Liars, Cheats and Other Proud Americans", "LD", (150, 60, 60, 255))}
MINIGAME_GATE = {"Liar's Dice": "dice_open", "Arm Wrestling": "mexico_open"}
STORY_TOGGLES = [(c, nm, m, l, col) for c, nm, m, l, col in AREA_OPEN.values()] + [(c,) + v for c, v in STORY_OPEN.items()]
for c, nm, mission, label, color in STORY_TOGGLES:
    items.append({"name": nm, "type": "toggle", "img": f"images/items/{c}.png", "codes": c})
    icon(os.path.join(OUT, f"images/items/{c}.png"), label, color)

# hidden option counters, set from slot_data (they hide checks that are not in the seed)
SETTINGS = {"opt_strangers": 1, "opt_hideouts": 1, "opt_shops": 1, "opt_gangkills": 10, "opt_hattricks": 15,
            "opt_minigames": 5, "opt_bounties": 20, "opt_fame": 1, "opt_honor_good": 1, "opt_honor_bad": 1}
for c, mx in SETTINGS.items():
    items.append({"name": c, "type": "consumable", "img": "images/items/deadeye.png", "codes": c, "max_quantity": mx})

with open(os.path.join(OUT, "items/items.json"), "w") as f:
    json.dump(items, f, indent=1)

# ---------------------------------------------------------------- map
W, H = 1200, 800
REGION_BOX = {"West Elizabeth": (40, 40, 560, 380), "New Austin": (40, 420, 700, 760), "Nuevo Paraiso": (740, 420, 1160, 760),
              "Anywhere": (600, 215, 1000, 385)}
REGION_LABEL = {"Nuevo Paraiso": "Mexico"}        # shown on the map and in pin names
ANYWHERE = {"hat_trick", "minigame", "bounty", "rank"}    # checks that are not tied to one territory (bounties count in the order
                                                  # you finish them; Bounty 9+ still need Mexico's Dead Eye in logic)
REGION_COLOR = {"West Elizabeth": (60, 90, 50, 255), "New Austin": (150, 105, 55, 255), "Nuevo Paraiso": (170, 80, 40, 255)}
with open(os.path.join(OUT, "maps/maps.json"), "w") as f:
    json.dump([{"name": "Map", "location_size": 36, "location_border_thickness": 3, "img": "images/maps/map.png"}], f, indent=1)

# ---------------------------------------------------------------- locations
CATEGORY_LABEL = {"story": "Story", "stranger": "Strangers", "hideout": "Hideouts", "gang_kill": "Gang Kills",
                  "hat_trick": "Hat Tricks", "minigame": "Minigames", "bounty": "Bounties", "shop": "Shops", "rank": "Ranks"}
VIS = {"stranger": lambda d: ["opt_strangers"], "hideout": lambda d: ["opt_hideouts"], "shop": lambda d: ["opt_shops"],
       "gang_kill": lambda d: [f"opt_gangkills:{d.index}"], "hat_trick": lambda d: [f"opt_hattricks:{d.index}"],
       "minigame": lambda d: [f"opt_minigames:{d.index}"], "bounty": lambda d: [f"opt_bounties:{d.index}"],
       "rank": lambda d: [{"fame": "opt_fame", "honor_good": "opt_honor_good", "honor_bad": "opt_honor_bad"}[d.group]]}

pins = {}
location_mapping = {}
for name, data in location_table.items():
    if data.category not in CATEGORY_LABEL:
        continue
    region = "Anywhere" if data.category in ANYWHERE else region_of(data.area)
    if data.category == "gang_kill" and data.group == "Reyes' Rebels":   # in logic after An Appointed Time, but in Mexico
        region = "Nuevo Paraiso"
    if data.category == "hideout":                     # every hideout is its own pin, named after the hideout
        pin = f"{REGION_LABEL.get(region, region)} - {data.group}"
    else:
        pin = f"{REGION_LABEL.get(region, region)} - {CATEGORY_LABEL[data.category]}"
    de = dead_eye_for(data.area)
    section = {"name": name, "item_count": 1}
    rule = ([f"deadeye:{de}"] if de else []) + ([AREA_OPEN[region][0]] if region in AREA_OPEN else [])
    if data.category == "minigame" and data.group in MINIGAME_GATE:
        rule.append(MINIGAME_GATE[data.group])
    if data.category == "gang_kill" and data.group == "Reyes' Rebels":
        rule.append("we_open")   # the story mission that sets "West Elizabeth Open" is An Appointed Time
    if rule:
        section["access_rules"] = [",".join(rule)]
    if data.category in VIS:
        section["visibility_rules"] = VIS[data.category](data)
    pins.setdefault(pin, (region, data.category, []))[2].append(section)
    location_mapping[data.code] = [f"@{pin}/{name}"]

children = []
per_region = {}
for pin, (region, cat, sections) in sorted(pins.items(), key=lambda kv: kv[1][1] == "hideout"):
    n = per_region.get(region, 0)
    per_region[region] = n + 1
    box = REGION_BOX[region]
    if region == "Anywhere":
        x = box[0] + 62 + n * 92
        y = box[1] + 95
    else:
        cols = 4 if box[2] - box[0] > 500 else 3
        x = box[0] + 80 + (n % cols) * ((box[2] - box[0] - 160) // (cols - 1))
        y = box[1] + 95 + (n // cols) * 98
    children.append({"name": pin, "map_locations": [{"map": "Map", "x": int(x), "y": int(y)}], "sections": sections})

# the map picture is drawn once the pins are placed, so trees, cacti and huts stay clear of the pins and their labels
rdr_icons.draw_map(os.path.join(OUT, "images/maps/map.png"), REGION_BOX, W, H, labels=REGION_LABEL,
                   keepout=[(c["map_locations"][0]["x"], c["map_locations"][0]["y"]) for c in children],
                   anywhere_pins=[(c["name"].split(" - ", 1)[1], c["map_locations"][0]["x"], c["map_locations"][0]["y"])
                                  for c in children if c["name"].startswith("Anywhere - ")])

# label every pin on the map picture
img = Image.open(os.path.join(OUT, "images/maps/map.png"))
d = ImageDraw.Draw(img)
small = ImageFont.truetype(FONT, 15)
img = img.convert("RGBA")
d = ImageDraw.Draw(img)
for child in children:
    loc = child["map_locations"][0]
    label = child["name"].split(" - ", 1)[1]
    w = d.textlength(label, font=small)
    d.rounded_rectangle((loc["x"] - w / 2 - 5, loc["y"] + 22, loc["x"] + w / 2 + 5, loc["y"] + 42), 5,
                        fill=(244, 228, 192, 230), outline=(84, 52, 30, 255), width=2)
    d.text((loc["x"] - w / 2, loc["y"] + 24), label, fill=(70, 36, 20, 255), font=small)
img.convert("RGB").save(os.path.join(OUT, "images/maps/map.png"))

with open(os.path.join(OUT, "locations/locations.json"), "w") as f:
    json.dump([{"name": "Red Dead Redemption", "children": children}], f, indent=1)

# ---------------------------------------------------------------- layouts
def rows(codes, per_row=6):
    return [codes[i:i + per_row] for i in range(0, len(codes), per_row)]

guns = [c for c in grid_weapons if c not in ("throwing_knives", "fire_bottles", "dynamite", "tomahawk")]
throw = [c for c in grid_weapons if c in ("throwing_knives", "fire_bottles", "dynamite", "tomahawk")]


def grid_of(codes, per_row=6):
    return {"type": "itemgrid", "h_alignment": "center", "item_margin": "3,3", "item_size": 44, "rows": rows(codes, per_row)}


grid = {"type": "array", "orientation": "vertical", "margin": "0,0", "content": [
    {"type": "group", "header": "Dead Eye", "content": grid_of([f"deadeye_{lv}" for lv in range(1, 5)], 4)},
    {"type": "group", "header": "Story Progress", "content": grid_of([t[0] for t in STORY_TOGGLES], 4)},
    {"type": "group", "header": "Guns", "content": grid_of(guns)},
    {"type": "group", "header": "Throwables", "content": grid_of(throw, 4)},
    {"type": "group", "header": "Outfits", "content": grid_of(grid_outfits)},
]}
layouts = {
    "shared_item_grid": grid,
    "tracker_default": {"type": "container", "background": "#00000000", "content": {"type": "dock", "dropshadow": True, "content": [
        {"type": "dock", "dock": "left", "v_alignment": "stretch", "content": [
            {"type": "group", "header": "Items", "dock": "top", "content": {"type": "layout", "key": "shared_item_grid"}}]},
        {"type": "tabbed", "tabs": [{"title": "Map", "content": {"type": "map", "maps": ["Map"]}}]}]}},
    "tracker_broadcast": {"type": "layout", "background": "#00000000", "key": "shared_item_grid"},
}
with open(os.path.join(OUT, "layouts/tracker.json"), "w") as f:
    json.dump(layouts, f, indent=1)

# ---------------------------------------------------------------- scripts
def lua_table(mapping):
    lines = ["{"]
    for k, v in sorted(mapping.items()):
        lines.append(f'  [{k}] = {{' + ", ".join(json.dumps(x) for x in v) + "},")
    lines.append("}")
    return "\n".join(lines)

with open(os.path.join(OUT, "scripts/autotracking/item_mapping.lua"), "w") as f:
    f.write("ITEM_MAPPING = " + lua_table(item_mapping) + "\n")
with open(os.path.join(OUT, "scripts/autotracking/location_mapping.lua"), "w") as f:
    f.write("LOCATION_MAPPING = " + lua_table(location_mapping) + "\n")
    f.write("AREA_OPEN_BY_LOCATION = {\n" + "".join(f'  [{location_table[m].code}] = "{c}",\n' for c, nm, m, l, col in STORY_TOGGLES) + "}\n")

with open(os.path.join(OUT, "scripts/init.lua"), "w") as f:
    f.write('''Tracker:AddItems("items/items.json")
Tracker:AddMaps("maps/maps.json")
Tracker:AddLocations("locations/locations.json")
Tracker:AddLayouts("layouts/tracker.json")
if PopVersion and PopVersion >= "0.18.0" then
  ScriptHost:LoadScript("scripts/autotracking.lua")
end
''')

with open(os.path.join(OUT, "scripts/autotracking.lua"), "w") as f:
    f.write('''ScriptHost:LoadScript("scripts/autotracking/item_mapping.lua")
ScriptHost:LoadScript("scripts/autotracking/location_mapping.lua")

local function set_count(code, n)
  local obj = Tracker:FindObjectForCode(code)
  if obj then obj.AcquiredCount = n end
end

function onClear(slot_data)
  for _, v in pairs(LOCATION_MAPPING) do
    local obj = Tracker:FindObjectForCode(v[1])
    if obj then obj.AvailableChestCount = obj.ChestCount end
  end
  for _, v in pairs(ITEM_MAPPING) do
    local obj = Tracker:FindObjectForCode(v[1])
    if obj then
      if v[2] == "toggle" then obj.Active = false else obj.AcquiredCount = 0 end
    end
  end
  for lv = 1, 4 do
    local obj = Tracker:FindObjectForCode("deadeye_" .. lv)
    if obj then obj.Active = false end
  end
  for _, c in pairs(AREA_OPEN_BY_LOCATION) do
    local obj = Tracker:FindObjectForCode(c)
    if obj then obj.Active = false end
  end
  slot_data = slot_data or {}
  local function flag(key, default) local v = slot_data[key]; if v == nil then return default end; return v and v ~= 0 end
  set_count("opt_strangers", flag("include_stranger_missions", true) and 1 or 0)
  set_count("opt_hideouts", flag("include_gang_hideouts", true) and 1 or 0)
  set_count("opt_shops", flag("include_shops", true) and 1 or 0)
  set_count("opt_gangkills", slot_data["gang_kill_checks"] or 5)
  set_count("opt_hattricks", slot_data["hat_tricks"] or 5)
  set_count("opt_minigames", slot_data["minigame_wins"] or 3)
  set_count("opt_bounties", slot_data["bounty_checks"] or 0)
  set_count("opt_fame", flag("fame_ranks", false) and 1 or 0)
  set_count("opt_honor_good", slot_data["honor_ranks"] == "good" and 1 or 0)
  set_count("opt_honor_bad", slot_data["honor_ranks"] == "bad" and 1 or 0)
end

function onItem(index, item_id, item_name, player_number)
  local v = ITEM_MAPPING[item_id]
  if not v then return end
  local obj = Tracker:FindObjectForCode(v[1])
  if not obj then return end
  if v[2] == "toggle" then obj.Active = true
  else obj.AcquiredCount = obj.AcquiredCount + obj.Increment end
  if v[1] == "deadeye" then
    local lv = Tracker:FindObjectForCode("deadeye_" .. obj.AcquiredCount)
    if lv then lv.Active = true end
  end
end

function onLocation(location_id, location_name)
  local open = AREA_OPEN_BY_LOCATION[location_id]   -- the story mission that opens Mexico / West Elizabeth
  if open then
    local t = Tracker:FindObjectForCode(open)
    if t then t.Active = true end
  end
  local v = LOCATION_MAPPING[location_id]
  if not v then return end
  local obj = Tracker:FindObjectForCode(v[1])
  if obj then obj.AvailableChestCount = obj.AvailableChestCount - 1 end
end

-- Hinted checks: a check of ours that someone hinted gets a highlight (its pin changes colour): "priority" when the hinted
-- item is progression, "no priority" otherwise. Uses Archipelago's hint list (data storage key _read_hints_<team>_<slot>).
local function hints_key()
  return "_read_hints_" .. tostring(Archipelago.TeamNumber) .. "_" .. tostring(Archipelago.PlayerNumber)
end

local function apply_hints(value)
  if not value or Highlight == nil then return end
  for _, hint in ipairs(value) do
    if hint.finding_player == Archipelago.PlayerNumber and not hint.found then
      local v = LOCATION_MAPPING[hint.location]
      local obj = v and Tracker:FindObjectForCode(v[1])
      if obj then
        if (hint.item_flags or 0) & 1 == 1 then obj.Highlight = Highlight.Priority else obj.Highlight = Highlight.NoPriority end
      end
    end
  end
end

local function on_hints(key, value)
  if key == hints_key() then apply_hints(value) end
end

local base_clear = onClear
function onClear(slot_data)
  base_clear(slot_data)
  if Highlight ~= nil then
    for _, v in pairs(LOCATION_MAPPING) do
      local obj = Tracker:FindObjectForCode(v[1])
      if obj then obj.Highlight = Highlight.None end
    end
  end
  if Archipelago.SetNotify then
    Archipelago:SetNotify({hints_key()})
    Archipelago:Get({hints_key()})
  end
end

local base_location = onLocation
function onLocation(location_id, location_name)
  base_location(location_id, location_name)
  local v = LOCATION_MAPPING[location_id]
  local obj = v and Tracker:FindObjectForCode(v[1])
  if obj and Highlight ~= nil then obj.Highlight = Highlight.None end
end

if Archipelago.AddSetReplyHandler then
  Archipelago:AddSetReplyHandler("hints handler", function(key, value, old) on_hints(key, value) end)
  Archipelago:AddRetrievedHandler("hints handler", function(key, value) on_hints(key, value) end)
end

Archipelago:AddClearHandler("clear handler", onClear)
Archipelago:AddItemHandler("item handler", onItem)
Archipelago:AddLocationHandler("location handler", onLocation)
''')

with open(os.path.join(OUT, "manifest.json"), "w") as f:
    json.dump({"name": "Red Dead Redemption Archipelago", "game_name": "Red Dead Redemption", "package_uid": "rdr1_ap",
               "package_version": VERSION, "platform": "pc", "author": "RDR-AP", "min_poptracker_version": "0.18.0",
               # the "ap" flag is what makes PopTracker show the AP (connect to Archipelago) button
               "variants": {"map": {"display_name": "Map Tracker", "flags": ["ap"]}}}, f, indent=1)

zpath = OUT.rstrip("/") + ".zip"
with zipfile.ZipFile(zpath, "w", zipfile.ZIP_DEFLATED) as z:
    for root, _, files in os.walk(OUT):
        for fn in files:
            full = os.path.join(root, fn)
            z.write(full, os.path.relpath(full, OUT))
print("pins", len(children), "locations", len(location_mapping), "items", len(item_mapping), "->", zpath)
