"""Original icon set for the RDR-AP PopTracker pack (drawn from simple shapes; no game art).

Weapons are side-view silhouettes built from parts (barrel, frame, cylinder, grip, stock, lever, scope, ...),
with a small tag in the corner so look-alike guns can be told apart. Outfits are hat-and-coat figures in the
outfit's colours. Everything is drawn at 4x and scaled down for smooth edges.
"""
import math
from PIL import Image, ImageDraw, ImageFont

FONT = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
S = 4            # supersampling
N = 64 * S       # canvas size

STEEL = (52, 52, 58, 255)
DARK = (28, 26, 30, 255)
WOOD = (122, 74, 38, 255)
WOOD_D = (88, 52, 26, 255)
BRASS = (196, 152, 62, 255)
GLASS = (120, 170, 200, 255)

GUN_BG = ((196, 222, 240), (142, 182, 216))
BG = {
    "revolver": GUN_BG, "pistol": GUN_BG, "repeater": GUN_BG, "rifle": GUN_BG, "sniper": GUN_BG, "shotgun": GUN_BG,
    "thrown": ((248, 234, 170), (222, 196, 112)),      # throwables: light yellow
    "outfit": ((244, 186, 176), (212, 134, 124)),      # outfits / gangs: light red
    "deadeye": ((150, 24, 24), (70, 8, 8)),
}
TILE_EDGE = (92, 62, 40, 255)


def _canvas(kind):
    img = Image.new("RGBA", (N, N), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    top, bottom = BG[kind]
    for y in range(N):          # vertical gradient inside a rounded tile
        f = y / N
        c = tuple(int(top[i] * (1 - f) + bottom[i] * f) for i in range(3)) + (255,)
        d.line([(0, y), (N, y)], fill=c)
    mask = Image.new("L", (N, N), 0)
    ImageDraw.Draw(mask).rounded_rectangle((2 * S, 2 * S, N - 2 * S, N - 2 * S), 11 * S, fill=255)
    tile = Image.new("RGBA", (N, N), (0, 0, 0, 0))
    tile.paste(img, (0, 0), mask)
    d = ImageDraw.Draw(tile)
    d.rounded_rectangle((2 * S, 2 * S, N - 2 * S, N - 2 * S), 11 * S, outline=TILE_EDGE if kind != "deadeye" else (232, 210, 168, 255), width=2 * S)
    return tile, d


def _poly(d, pts, fill, outline=DARK):
    d.polygon([(x * S, y * S) for x, y in pts], fill=fill, outline=outline)


def _rect(d, x0, y0, x1, y1, fill, r=0):
    if r:
        d.rounded_rectangle((x0 * S, y0 * S, x1 * S, y1 * S), r * S, fill=fill, outline=DARK, width=S // 2 or 1)
    else:
        d.rectangle((x0 * S, y0 * S, x1 * S, y1 * S), fill=fill, outline=DARK, width=S // 2 or 1)


def _circle(d, cx, cy, r, fill, width=0, outline=DARK):
    d.ellipse(((cx - r) * S, (cy - r) * S, (cx + r) * S, (cy + r) * S), fill=fill, outline=outline, width=width * S if width else 1)


def _tag(d, text):
    if not text:
        return
    font = ImageFont.truetype(FONT, 11 * S)
    w = d.textlength(text, font=font)
    x, y = N - 6 * S - w, N - 17 * S
    d.rounded_rectangle((x - 3 * S, y - 1 * S, x + w + 3 * S, y + 13 * S), 3 * S, fill=(20, 14, 10, 210))
    d.text((x, y), text, font=font, fill=(255, 238, 205, 255))


def _layer():
    layer = Image.new("RGBA", (N, N), (0, 0, 0, 0))
    return layer, ImageDraw.Draw(layer)


def _place(tile, layer, angle=0.0, scale=1.0, dx=0, dy=0):
    """Scale and rotate a drawn weapon around the tile centre, then lay it on the tile."""
    if scale != 1.0:
        big = layer.resize((int(N * scale), int(N * scale)), Image.LANCZOS)
        off = (int(N * scale) - N) // 2
        layer = big.crop((off, off, off + N, off + N))
    if angle:
        layer = layer.rotate(angle, resample=Image.BICUBIC, center=(N / 2, N / 2))
    shadow = Image.new("RGBA", (N, N), (0, 0, 0, 0))
    shadow.paste((0, 0, 0, 110), (0, 0), layer.split()[3])
    tile.alpha_composite(shadow, (dx * S + 2 * S, dy * S + 2 * S))
    tile.alpha_composite(layer, (dx * S, dy * S))


def _finish(img, path):
    img.resize((64, 64), Image.LANCZOS).save(path)


# ---------------------------------------------------------------- handguns
def handgun(path, tag, style="revolver", barrel=22, cyl=True, under=False, lever=False, box_mag=False, slide=False,
            metal=STEEL, grip=WOOD):
    img, td = _canvas("revolver" if cyl else "pistol")
    layer, d = _layer()
    y = 22                                  # barrel line
    bx0 = 24
    _rect(d, bx0, y, bx0 + barrel, y + 5, metal)                           # barrel
    if under:                                                              # LeMat: shotgun barrel under the main one
        _rect(d, bx0, y + 6, bx0 + barrel - 6, y + 10, metal)
    if slide:                                                              # semi-auto slide over the barrel
        _rect(d, bx0 - 8, y - 3, bx0 + barrel, y + 6, metal, r=1)
    _poly(d, [(12, y - 1), (bx0 + 2, y - 1), (bx0 + 2, y + 12), (16, y + 14), (12, y + 8)], metal)   # frame
    if cyl:
        _rect(d, 17, y - 2, bx0 + 1, y + 9, metal, r=2)                   # cylinder
        for k in range(3):
            d.line(((18 + k * 2.4) * S, (y + 1) * S, (18 + k * 2.4) * S, (y + 6) * S), fill=(90, 90, 98, 255), width=S)
    if box_mag:                                                            # Mauser box magazine in front of the trigger
        _rect(d, bx0 - 4, y + 6, bx0 + 4, y + 16, metal)
    d.arc(((14) * S, (y + 8) * S, (24) * S, (y + 18) * S), 0, 180, fill=DARK, width=2 * S)   # trigger guard
    if lever:                                                              # Volcanic lever under the grip
        d.arc(((15) * S, (y + 9) * S, (27) * S, (y + 21) * S), 10, 200, fill=metal, width=2 * S)
    gx = 12
    _poly(d, [(gx, y + 6), (gx + 7, y + 8), (gx + 2, y + 30), (gx - 7, y + 28)], grip)              # grip
    d.line(((gx - 4) * S, (y + 14) * S, (gx + 3) * S, (y + 15) * S), fill=WOOD_D, width=S)
    _rect(d, bx0 + barrel - 3, y - 2, bx0 + barrel - 1, y, metal)         # front sight
    _place(img, layer, angle=8, scale=1.2, dx=6, dy=1)
    _tag(td, tag)
    _finish(img, path)


# ---------------------------------------------------------------- long guns
def long_gun(path, tag, kind="repeater", barrel=34, lever=False, tube=False, bolt=False, scope=False, pump=False,
             double=False, short=False, receiver=STEEL, stock=WOOD, semi=False):
    img, td = _canvas(kind)
    layer, d = _layer()
    y = 30
    bt = 4.5 if kind == "shotgun" else 2.2          # barrel thickness: rifles are slim, shotguns fat
    length = barrel if not short else barrel // 2
    rx0, rx1 = 22, 32                                                       # receiver
    bx1 = min(rx1 + length, 60)
    if double:
        _rect(d, rx1, y - 2, bx1, y + 2, STEEL)
        _rect(d, rx1, y + 2, bx1, y + 6, STEEL)
    else:
        _rect(d, rx1, y, bx1, y + bt, STEEL)                                # barrel
    if tube:
        tb = y + bt + 0.6
        _rect(d, rx1, tb, bx1 - 3, tb + (3 if kind == "shotgun" else 1.8), STEEL)   # magazine tube
    if pump:
        _rect(d, rx1 + 6, y + 4, rx1 + 18, y + 9, WOOD_D, r=1)              # pump grip
    elif not short:
        _rect(d, rx1, y + bt, min(rx1 + 16, bx1 - 4), y + bt + (4 if kind == "shotgun" else 3), stock, r=1)   # forearm
    _rect(d, rx0, y - 2, rx1 + 1, y + 7, receiver, r=1)
    if semi:
        _rect(d, rx0 + 2, y - 4, rx1, y - 2, STEEL)
    stock_len = 18 if not short else 12
    _poly(d, [(rx0, y), (rx0, y + 6), (rx0 - stock_len, y + 13), (rx0 - stock_len - 1, y + 5),
              (rx0 - stock_len + 2, y + 2)], stock)                          # stock
    d.arc(((rx0 + 1) * S, (y + 5) * S, (rx0 + 9) * S, (y + 13) * S), 0, 180, fill=DARK, width=2 * S)  # trigger guard
    if lever:
        d.arc(((rx0 - 2) * S, (y + 5) * S, (rx0 + 10) * S, (y + 17) * S), 0, 200, fill=STEEL, width=2 * S)
    if bolt:
        d.line(((rx1 - 2) * S, (y + 1) * S, (rx1 - 6) * S, (y + 8) * S), fill=STEEL, width=2 * S)
        _circle(d, rx1 - 6, y + 9, 1.6, STEEL)
    if scope:
        _rect(d, rx0 + 1, y - 8, rx1 + 12, y - 4, DARK, r=2)
        _rect(d, rx1 + 10, y - 9, rx1 + 13, y - 3, DARK)
        _rect(d, rx0 - 1, y - 9, rx0 + 2, y - 3, DARK)
        d.line(((rx0 + 3) * S, (y - 7) * S, (rx1 + 9) * S, (y - 7) * S), fill=GLASS, width=S)
    _place(img, layer, angle=24, scale=1.04 if not short else 1.3, dx=1, dy=0)
    _tag(td, tag)
    _finish(img, path)


# ---------------------------------------------------------------- thrown weapons
def thrown(path, what, tag):
    img, d = _canvas("thrown")
    if what == "knives":                                                  # one throwing knife, point up-right
        layer, ld = _layer()
        _poly(ld, [(30, 33), (54, 31), (58, 32), (54, 34.5), (30, 36)], (214, 216, 226, 255))       # blade
        ld.line((32 * S, 34.2 * S, 54 * S, 32.6 * S), fill=(160, 162, 176, 255), width=S)            # edge line
        _rect(ld, 27, 29, 30, 40, BRASS, r=1)                                                         # guard
        _rect(ld, 11, 31.5, 27, 37.5, WOOD_D, r=2)                                                    # handle
        for k in range(4):
            ld.line(((14 + k * 3.5) * S, 31.5 * S, (12.5 + k * 3.5) * S, 37.5 * S), fill=(60, 34, 18, 255), width=S)
        _circle(ld, 10.5, 34.5, 2.2, BRASS)                                                           # pommel
        _place(img, layer, angle=35, scale=1.05, dx=0, dy=0)
    elif what == "bottle":
        _rect(d, 24, 26, 40, 50, (70, 110, 60, 255), r=5)
        _rect(d, 29, 16, 35, 27, (70, 110, 60, 255))
        _rect(d, 28, 12, 36, 17, (230, 220, 200, 255))
        for k, c in enumerate([(255, 210, 60, 255), (255, 140, 30, 255), (220, 60, 20, 255)]):
            r = 6 - k * 1.6
            _circle(d, 32, 9 - k * 1.5, r, c, outline=c)
    elif what == "dynamite":
        for k in range(3):
            _rect(d, 18 + k * 9, 20, 26 + k * 9, 50, (190, 40, 36, 255), r=2)
        _rect(d, 17, 30, 45, 34, (60, 40, 30, 255))
        d.line((31 * S, 20 * S, 36 * S, 12 * S, 40 * S, 10 * S), fill=(40, 30, 20, 255), width=S + 2)
        _circle(d, 41, 9, 3, (255, 200, 60, 255), outline=(255, 120, 30, 255))
    elif what == "tomahawk":                                              # hatchet head, wrapped haft, two hanging feathers
        layer, ld = _layer()
        _rect(ld, 30, 14, 34.5, 60, WOOD, r=2)                                               # haft
        for k in range(5):                                                                    # leather wrap under the head
            ld.line((30 * S, (23 + k * 2) * S, 34.5 * S, (24.4 + k * 2) * S), fill=(70, 40, 22, 255), width=S)
        _rect(ld, 30, 50, 34.5, 60, (96, 58, 30, 255), r=2)                                  # grip wrap
        arc = [(46 + 8.5 * math.cos(math.radians(g)), 17 + 10.5 * math.sin(math.radians(g))) for g in range(-80, 81, 10)]
        _poly(ld, [(34, 12.5), (42, 14.5)] + arc + [(42, 20.5), (34, 21.5)], (196, 198, 208, 255))   # curved, flared blade
        edge = [(46 + 7.6 * math.cos(math.radians(g)), 17 + 9.6 * math.sin(math.radians(g))) for g in range(-70, 71, 10)]
        ld.line([(x * S, y * S) for x, y in edge], fill=(250, 250, 255, 255), width=S)                  # bright cutting edge
        _poly(ld, [(30, 13), (22, 15), (21, 17.5), (30, 20)], (150, 152, 164, 255))          # poll / back spike
        _circle(ld, 32.2, 17, 1.2, (60, 60, 70, 255))                                         # rivet
        ld.line((30 * S, 22 * S, 24 * S, 28 * S), fill=(70, 40, 22, 255), width=S)            # cord to the feathers
        _circle(ld, 24, 28.5, 1.4, (40, 120, 160, 255))                                       # bead
        _circle(ld, 23.4, 31.5, 1.3, (190, 50, 40, 255))
        for fx, top in ((21, 31), (26.5, 33)):                                                # two feathers
            tip = top + 20
            _poly(ld, [(fx, top), (fx + 3, top + 5), (fx + 3.3, top + 13), (fx, tip), (fx - 3.3, top + 13), (fx - 3, top + 5)],
                  (246, 240, 228, 255))
            _poly(ld, [(fx - 3.3, top + 13), (fx, tip), (fx + 3.3, top + 13), (fx, top + 11)], (66, 40, 26, 255))   # dark tip
            ld.line((fx * S, top * S, fx * S, (tip - 1) * S), fill=(130, 104, 76, 255), width=S)                  # quill
        _place(img, layer, angle=-24, scale=1.08, dx=-1, dy=0)
    _tag(d, tag)
    _finish(img, path)


# ---------------------------------------------------------------- outfits and Dead Eye
def outfit(path, hat, coat, accent, tag, style="cowboy", skin=(222, 184, 150, 255), badge=False, bandolier=False,
           bandana=None, serape=None, buttons=None, mustache=False, tie=None):
    img, d = _canvas("outfit")
    # body / coat
    _poly(d, [(16, 60), (20, 38), (32, 34), (44, 38), (48, 60)], coat)
    if serape:                                                             # striped poncho / serape over the shoulders
        _poly(d, [(14, 52), (20, 37), (32, 33), (44, 37), (50, 52), (32, 47)], serape[0])
        for k, c in enumerate(serape[1:]):
            yy = 40 + k * 3.5
            d.line((19 * S, (yy + 4) * S, 32 * S, (yy - 0.5) * S, 45 * S, (yy + 4) * S), fill=c, width=int(1.6 * S))
    _poly(d, [(29, 35), (32, 44), (35, 35)], accent)                       # shirt / collar
    if tie:
        _poly(d, [(31, 36), (33, 36), (33.6, 43), (32, 45), (30.4, 43)], tie)
    if buttons:
        for k in range(3):
            _circle(d, 32, 46 + k * 4, 1.1, buttons, outline=buttons)
    elif not serape:
        d.line((32 * S, 44 * S, 32 * S, 60 * S), fill=DARK, width=S)
    if bandolier:                                                          # crossed ammo belts
        for (x0, x1) in ((20, 44), (44, 20)):
            d.line((x0 * S, 38 * S, x1 * S, 58 * S), fill=(110, 80, 40, 255), width=3 * S)
            for k in range(6):
                f = (k + 0.5) / 6
                _circle(d, x0 + (x1 - x0) * f, 38 + 20 * f, 0.9, BRASS, outline=BRASS)
    if badge:                                                              # tin star
        cx, cy = 24, 44
        pts = []
        for k in range(10):
            r = 3.6 if k % 2 == 0 else 1.6
            a_ = math.radians(-90 + k * 36)
            pts.append((cx + math.cos(a_) * r, cy + math.sin(a_) * r))
        _poly(d, pts, (236, 212, 96, 255), outline=(120, 90, 30, 255))
    # head
    _circle(d, 32, 27, 7, skin)
    if mustache:
        _poly(d, [(27.5, 30), (32, 28.6), (36.5, 30), (35, 31.4), (32, 30.4), (29, 31.4)], (52, 34, 24, 255))
    if bandana:                                                            # bandit mask over the lower face
        _poly(d, [(25, 27.5), (39, 27.5), (38, 32), (32, 36), (26, 32)], bandana)
    # hats
    if style == "sombrero":
        d.ellipse((10 * S, 17 * S, 54 * S, 25 * S), fill=hat, outline=DARK)
        d.pieslice((24 * S, 6 * S, 40 * S, 26 * S), 180, 360, fill=hat, outline=DARK)
        d.line((25 * S, 18.5 * S, 39 * S, 18.5 * S), fill=accent, width=2 * S)
    elif style == "tophat":
        d.ellipse((20 * S, 18 * S, 44 * S, 23 * S), fill=hat, outline=DARK)
        _rect(d, 25, 5, 39, 20, hat, r=1)
        d.line((25 * S, 17 * S, 39 * S, 17 * S), fill=accent, width=2 * S)
    elif style == "bowler":
        d.ellipse((21 * S, 18 * S, 43 * S, 23 * S), fill=hat, outline=DARK)
        d.pieslice((24 * S, 10 * S, 40 * S, 26 * S), 180, 360, fill=hat, outline=DARK)
        d.line((25 * S, 17.5 * S, 39 * S, 17.5 * S), fill=accent, width=S + 1)
    elif style == "kepi":                                                  # army cap with a short visor
        _poly(d, [(25, 21), (26, 11), (39, 13), (39, 21)], hat)
        _poly(d, [(38, 19.5), (45, 21), (39, 22)], DARK)
        _circle(d, 32, 16, 1.4, BRASS, outline=BRASS)
    elif style == "furcap":
        d.pieslice((23 * S, 11 * S, 41 * S, 29 * S), 180, 360, fill=hat, outline=DARK)
        for k in range(6):
            d.line(((24 + k * 3) * S, 20 * S, (24.5 + k * 3) * S, 17 * S), fill=(70, 50, 30, 255), width=S)
        d.line((41 * S, 19 * S, 46 * S, 30 * S), fill=hat, width=2 * S)    # tail
    elif style == "miner":                                                 # floppy brim, rolled sleeves look
        d.ellipse((17 * S, 17 * S, 47 * S, 23 * S), fill=hat, outline=DARK)
        d.pieslice((24 * S, 9 * S, 40 * S, 27 * S), 180, 360, fill=hat, outline=DARK)
    else:                                                                  # cowboy hat (wide or regular)
        brim = 15 if style == "wide" else 12
        d.ellipse(((32 - brim) * S, 17 * S, (32 + brim) * S, 22.5 * S), fill=hat, outline=DARK)
        _poly(d, [(25, 20), (26, 11), (29, 13), (32, 11), (35, 13), (38, 11), (39, 20)], hat)
        d.line((25.5 * S, 17.5 * S, 38.5 * S, 17.5 * S), fill=accent, width=2 * S)
    _tag(d, tag)
    _finish(img, path)


def dead_eye(path, level=0):
    """Dead Eye eye; level 1-4 adds that level's mark: 1 plain, 2 lock-on reticle, 3 painted X marks, 4 recovery arrows."""
    img, d = _canvas("deadeye")
    d.ellipse((8 * S, 20 * S, 56 * S, 44 * S), fill=(245, 235, 220, 255), outline=DARK, width=2 * S)
    _circle(d, 32, 32, 10, (150, 20, 20, 255), width=1)
    _circle(d, 32, 32, 4.5, (20, 10, 10, 255))
    for a in range(0, 360, 45):
        r = math.radians(a)
        d.line(((32 + math.cos(r) * 15) * S, (32 + math.sin(r) * 15) * S, (32 + math.cos(r) * 20) * S,
                (32 + math.sin(r) * 20) * S), fill=(255, 220, 200, 255), width=S)
    gold = (250, 210, 90, 255)
    if level == 2:                                                         # auto lock-on: reticle corners
        for sx, sy in ((-1, -1), (1, -1), (-1, 1), (1, 1)):
            x, y = 32 + sx * 14, 32 + sy * 14
            d.line((x * S, y * S, (x - sx * 6) * S, y * S), fill=gold, width=2 * S)
            d.line((x * S, y * S, x * S, (y - sy * 6) * S), fill=gold, width=2 * S)
    elif level == 3:                                                       # target painting: red X marks
        for x, y in ((14, 14), (50, 14), (14, 50)):
            d.line(((x - 4) * S, (y - 4) * S, (x + 4) * S, (y + 4) * S), fill=(255, 60, 40, 255), width=2 * S)
            d.line(((x - 4) * S, (y + 4) * S, (x + 4) * S, (y - 4) * S), fill=(255, 60, 40, 255), width=2 * S)
    elif level == 4:                                                       # fast recovery: circular arrow
        d.arc((16 * S, 16 * S, 48 * S, 48 * S), 200, 520, fill=gold, width=2 * S)
        _poly(d, [(14, 25), (20, 22), (19, 29)], gold, outline=gold)
    if level:
        _tag(d, str(level))
    _finish(img, path)


# ---------------------------------------------------------------- the set
def weapon_icon(name, path):
    W = {
        "Cattleman Revolver": lambda p: handgun(p, "CAT", barrel=20),
        "Schofield Revolver": lambda p: handgun(p, "SCH", barrel=25, metal=(70, 76, 90, 255)),
        "Double Action Revolver": lambda p: handgun(p, "DA", barrel=17, grip=(60, 40, 30, 255)),
        "LeMat Revolver": lambda p: handgun(p, "LM", barrel=22, under=True),
        "Volcanic Pistol": lambda p: handgun(p, "VOL", barrel=24, cyl=False, lever=True, metal=BRASS),
        "Semi-Auto Pistol": lambda p: handgun(p, "SA", barrel=20, cyl=False, slide=True),
        "High Power Pistol": lambda p: handgun(p, "HP", barrel=24, cyl=False, slide=True, grip=(50, 40, 34, 255)),
        "Mauser Pistol": lambda p: handgun(p, "MAU", barrel=24, cyl=False, box_mag=True),
        "Carbine Repeater": lambda p: long_gun(p, "CAR", barrel=24, lever=True, tube=True),
        "Winchester Repeater": lambda p: long_gun(p, "WIN", barrel=30, lever=True, tube=True),
        "Henry Repeater": lambda p: long_gun(p, "HEN", barrel=30, lever=True, tube=True, receiver=BRASS),
        "Evans Repeater": lambda p: long_gun(p, "EV", barrel=34, lever=True, tube=True, stock=WOOD_D),
        "Springfield Rifle": lambda p: long_gun(p, "SPR", kind="rifle", barrel=34),
        "Bolt Action Rifle": lambda p: long_gun(p, "BA", kind="rifle", barrel=34, bolt=True),
        "Buffalo Rifle": lambda p: long_gun(p, "BUF", kind="rifle", barrel=38, stock=WOOD_D),
        "Rolling Block Rifle": lambda p: long_gun(p, "RB", kind="sniper", barrel=36, scope=True),
        "Carcano Rifle": lambda p: long_gun(p, "CRC", kind="sniper", barrel=36, scope=True, bolt=True),
        "Sawed-Off Shotgun": lambda p: long_gun(p, "SAW", kind="shotgun", barrel=30, double=True, short=True),
        "Double Barrel Shotgun": lambda p: long_gun(p, "DBL", kind="shotgun", barrel=30, double=True),
        "Pump Action Shotgun": lambda p: long_gun(p, "PMP", kind="shotgun", barrel=30, pump=True, tube=True),
        "Semi-Auto Shotgun": lambda p: long_gun(p, "SAS", kind="shotgun", barrel=30, tube=True, semi=True),
        "Throwing Knives": lambda p: thrown(p, "knives", ""),
        "Fire Bottles": lambda p: thrown(p, "bottle", ""),
        "Dynamite": lambda p: thrown(p, "dynamite", ""),
        "Tomahawk": lambda p: thrown(p, "tomahawk", ""),
    }
    W[name](path)


TAN = (190, 140, 100, 255)        # skin tones
LIGHT = (226, 188, 154, 255)
BLACK = (24, 22, 26, 255)
WHITE = (238, 234, 224, 255)

OUTFIT_LOOK = {
    # gentlemen and suits
    "Gentleman's Attire": dict(hat=BLACK, coat=(46, 46, 56, 255), accent=WHITE, tag="GEN", style="tophat", tie=(150, 30, 40, 255), mustache=True),
    "Elegant Suit": dict(hat=(64, 54, 46, 255), coat=(112, 98, 84, 255), accent=WHITE, tag="SUIT", style="bowler", tie=(40, 60, 110, 255)),
    "Savvy Merchant Outfit": dict(hat=(70, 44, 44, 255), coat=(104, 52, 62, 255), accent=(230, 210, 140, 255), tag="MER", style="bowler", tie=(200, 170, 60, 255), mustache=True),
    "Bureau Uniform": dict(hat=BLACK, coat=(54, 56, 66, 255), accent=WHITE, tag="BUR", style="bowler", tie=BLACK, badge=True),
    "Deadly Assassin Outfit": dict(hat=BLACK, coat=(26, 26, 30, 255), accent=(176, 24, 24, 255), tag="DA", style="wide", tie=(176, 24, 24, 255)),
    # gangs
    "Walton's Gang Outfit": dict(hat=BLACK, coat=(96, 70, 50, 255), accent=(150, 110, 60, 255), tag="WAL", style="tophat", bandana=(130, 40, 32, 255)),
    "Bollard Twins Outfit": dict(hat=(116, 92, 60, 255), coat=(92, 108, 78, 255), accent=(70, 50, 30, 255), tag="BOL", style="cowboy", bandana=(70, 90, 140, 255)),
    "Treasure Hunter Outfit": dict(hat=(132, 112, 76, 255), coat=(146, 126, 90, 255), accent=(196, 152, 62, 255), tag="TH", style="miner", mustache=True),
    "Bandito Outfit": dict(hat=(150, 110, 60, 255), coat=(120, 50, 40, 255), accent=(240, 200, 80, 255), tag="BAN", style="sombrero", skin=TAN, mustache=True, bandolier=True),
    "Reyes' Rebels Outfit": dict(hat=(176, 146, 92, 255), coat=WHITE, accent=(190, 30, 30, 255), tag="REB", style="sombrero", skin=TAN, mustache=True, bandolier=True),
    "Mexican Poncho": dict(hat=(160, 124, 74, 255), coat=(90, 60, 40, 255), accent=(70, 40, 24, 255), tag="PON", style="sombrero", skin=TAN,
                           serape=[(176, 60, 40, 255), (240, 200, 90, 255), (40, 110, 90, 255), (240, 230, 210, 255)]),
    # law and army
    "U.S. Army Uniform": dict(hat=(44, 54, 92, 255), coat=(52, 66, 116, 255), accent=(220, 190, 80, 255), tag="ARMY", style="kepi", buttons=(230, 196, 80, 255)),
    "U.S. Marshal Uniform": dict(hat=WHITE, coat=(84, 72, 60, 255), accent=(220, 200, 80, 255), tag="USM", style="cowboy", badge=True, mustache=True),
    # frontier
    "Legend of the West": dict(hat=WHITE, coat=(156, 124, 84, 255), accent=(196, 152, 62, 255), tag="LEG", style="wide", badge=True),
    "Duster Coat": dict(hat=(84, 62, 42, 255), coat=(160, 128, 86, 255), accent=(112, 80, 50, 255), tag="DUS", style="wide"),
    "Rancher Outfit": dict(hat=(190, 160, 100, 255), coat=(92, 102, 134, 255), accent=(200, 180, 140, 255), tag="RAN", style="cowboy"),
    "Expert Hunter Outfit": dict(hat=(120, 90, 56, 255), coat=(112, 82, 52, 255), accent=(200, 180, 140, 255), tag="HUN", style="furcap", mustache=True),
}


def outfit_icon(name, path):
    look = OUTFIT_LOOK.get(name.replace("Outfit: ", ""))
    if look is None:
        look = dict(hat=(80, 60, 40, 255), coat=(100, 90, 80, 255), accent=(200, 180, 140, 255), tag="")
    outfit(path, **look)


# ---------------------------------------------------------------- map
def draw_map(path, boxes, W=1200, H=800, seed=7, labels=None):
    """Hand-drawn style frontier map: parchment, three territories with rough borders, a river border, mountains,
    cacti and pines, a compass rose and a title ribbon. Original artwork."""
    import random
    rnd = random.Random(seed)
    img = Image.new("RGBA", (W, H), (0, 0, 0, 255))
    d = ImageDraw.Draw(img)
    for y in range(H):                                                     # parchment gradient
        f = y / H
        d.line([(0, y), (W, y)], fill=(int(226 - 30 * f), int(200 - 34 * f), int(150 - 40 * f), 255))
    for _ in range(2600):                                                  # paper grain
        x, y = rnd.randrange(W), rnd.randrange(H)
        c = rnd.randrange(150, 215)
        d.point((x, y), fill=(c, int(c * 0.86), int(c * 0.62), 255))
    for k in range(18):                                                    # burnt edge
        d.rectangle((k, k, W - 1 - k, H - 1 - k), outline=(110 - k * 3, 70 - k * 2, 40 - k, 255))

    tint = {"West Elizabeth": (126, 156, 104), "New Austin": (204, 164, 104), "Nuevo Paraiso": (206, 132, 88)}

    def rough(box):
        x0, y0, x1, y1 = box
        pts = []
        steps = 14
        for i in range(steps):
            pts.append((x0 + (x1 - x0) * i / steps, y0 + rnd.uniform(-9, 9)))
        for i in range(steps):
            pts.append((x1 + rnd.uniform(-9, 9), y0 + (y1 - y0) * i / steps))
        for i in range(steps):
            pts.append((x1 - (x1 - x0) * i / steps, y1 + rnd.uniform(-9, 9)))
        for i in range(steps):
            pts.append((x0 + rnd.uniform(-9, 9), y1 - (y1 - y0) * i / steps))
        return pts

    labels = labels or {}
    anywhere = boxes.get("Anywhere")
    boxes = {k: v for k, v in boxes.items() if k != "Anywhere"}
    over = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    od = ImageDraw.Draw(over)
    for region, box in boxes.items():
        pts = rough(box)
        od.polygon(pts, fill=tint[region] + (150,))
        od.line(pts + [pts[0]], fill=(84, 52, 30, 255), width=4)
    img.alpha_composite(over)
    d = ImageDraw.Draw(img)

    # river between New Austin and Nuevo Paraiso, and along the top of New Austin
    na, np_, we = boxes["New Austin"], boxes["Nuevo Paraiso"], boxes["West Elizabeth"]
    rx = (na[2] + np_[0]) / 2
    river = [(rx + math.sin(t / 30) * 10, t) for t in range(int(na[1]) - 10, H - 10, 6)]
    d.line(river, fill=(70, 120, 170, 255), width=9, joint="curve")
    ry = (we[3] + na[1]) / 2
    river2 = [(t, ry + math.sin(t / 40) * 8) for t in range(30, int(rx) + 4, 6)]
    d.line(river2, fill=(70, 120, 170, 255), width=8, joint="curve")

    def mountain(x, y, s):
        d.polygon([(x - s, y), (x, y - s * 1.2), (x + s, y)], fill=(130, 96, 64, 255), outline=(70, 44, 26, 255))
        d.polygon([(x - s * 0.28, y - s * 0.86), (x, y - s * 1.2), (x + s * 0.28, y - s * 0.86)], fill=(240, 236, 228, 255))

    def cactus(x, y, s):
        g = (70, 110, 60, 255)
        d.rounded_rectangle((x - s * 0.18, y - s, x + s * 0.18, y), s * 0.18, fill=g)
        d.rounded_rectangle((x - s * 0.6, y - s * 0.7, x - s * 0.35, y - s * 0.3), s * 0.12, fill=g)
        d.rectangle((x - s * 0.6, y - s * 0.42, x, y - s * 0.3), fill=g)
        d.rounded_rectangle((x + s * 0.35, y - s * 0.85, x + s * 0.6, y - s * 0.45), s * 0.12, fill=g)
        d.rectangle((x, y - s * 0.57, x + s * 0.6, y - s * 0.45), fill=g)

    def pine(x, y, s):
        for k in range(3):
            w = s * (0.7 - k * 0.18)
            d.polygon([(x - w, y - k * s * 0.32), (x, y - s * 0.55 - k * s * 0.32), (x + w, y - k * s * 0.32)],
                      fill=(46, 86, 54, 255))
        d.rectangle((x - s * 0.08, y, x + s * 0.08, y + s * 0.18), fill=(90, 60, 30, 255))

    def scatter(box, fn, count, s):
        x0, y0, x1, y1 = box
        for _ in range(count):
            fn(rnd.uniform(x0 + 30, x1 - 30), rnd.uniform(y0 + 70, y1 - 20), s * rnd.uniform(0.8, 1.2))

    def adobe(x, y, s):
        wall, dark = (200, 140, 92, 255), (120, 70, 40, 255)
        d.rectangle((x - s * 0.7, y - s * 0.7, x + s * 0.7, y), fill=wall, outline=dark, width=2)
        d.rectangle((x - s * 0.75, y - s * 0.8, x + s * 0.75, y - s * 0.66), fill=(176, 118, 74, 255), outline=dark)
        for k in range(4):                                                  # roof beams poking out
            bx = x - s * 0.55 + k * s * 0.37
            d.rectangle((bx, y - s * 0.75, bx + s * 0.1, y - s * 0.62), fill=(90, 54, 30, 255))
        d.rectangle((x - s * 0.15, y - s * 0.38, x + s * 0.15, y), fill=(84, 52, 30, 255))      # door
        d.rectangle((x + s * 0.3, y - s * 0.5, x + s * 0.5, y - s * 0.32), fill=(84, 52, 30, 255))  # window

    scatter(we, pine, 26, 22)
    scatter(we, mountain, 4, 30)
    scatter(na, cactus, 20, 22)
    scatter(np_, cactus, 8, 22)
    scatter(np_, adobe, 4, 30)

    if anywhere:                                                            # checks that can happen anywhere
        x0, y0, x1, y1 = anywhere
        d.rounded_rectangle(anywhere, 14, fill=(244, 228, 192, 120), outline=(84, 52, 30, 255), width=3)
        f = ImageFont.truetype(FONT, 26)
        d.text((x0 + 16, y0 + 10), "Anywhere", fill=(70, 36, 20, 255), font=f)

    big = ImageFont.truetype(FONT, 34)
    for region, box in boxes.items():                                       # region names on a paper label
        region = labels.get(region, region)
        w = d.textlength(region, font=big)
        d.rounded_rectangle((box[0] + 14, box[1] + 8, box[0] + 34 + w, box[1] + 54), 8, fill=(244, 228, 192, 235),
                            outline=(84, 52, 30, 255), width=3)
        d.text((box[0] + 24, box[1] + 12), region, fill=(70, 36, 20, 255), font=big)

    # compass rose
    cx, cy, r = 1090, 310, 52
    d.ellipse((cx - r, cy - r, cx + r, cy + r), outline=(84, 52, 30, 255), width=3)
    for k, a in enumerate(range(0, 360, 45)):
        rr = r if k % 2 == 0 else r * 0.6
        ang = math.radians(a - 90)
        side = math.radians(a)
        tip = (cx + math.cos(ang) * rr, cy + math.sin(ang) * rr)
        d.polygon([tip, (cx + math.cos(side) * 9, cy + math.sin(side) * 9), (cx - math.cos(side) * 9, cy - math.sin(side) * 9)],
                  fill=(150, 30, 26, 255) if k == 0 else (84, 52, 30, 255))
    d.text((cx - 9, cy - r - 36), "N", fill=(84, 52, 30, 255), font=ImageFont.truetype(FONT, 26))

    # title ribbon
    title = ImageFont.truetype(FONT, 40)
    sub = ImageFont.truetype(FONT, 24)
    tx0, ty0, tx1, ty1 = 610, 70, 1160, 190
    d.polygon([(tx0 - 30, ty0 + 20), (tx0, ty0 + 20), (tx0, ty1 - 20), (tx0 - 30, ty1 - 20), (tx0 - 12, (ty0 + ty1) / 2)],
              fill=(120, 26, 22, 255))
    d.polygon([(tx1 + 30, ty0 + 20), (tx1, ty0 + 20), (tx1, ty1 - 20), (tx1 + 30, ty1 - 20), (tx1 + 12, (ty0 + ty1) / 2)],
              fill=(120, 26, 22, 255))
    d.rounded_rectangle((tx0, ty0, tx1, ty1), 10, fill=(170, 36, 30, 255), outline=(84, 20, 16, 255), width=4)
    for text, font, yy in (("RED DEAD REDEMPTION", title, ty0 + 18), ("~ Archipelago ~", sub, ty0 + 72)):
        w = d.textlength(text, font=font)
        d.text(((tx0 + tx1 - w) / 2, yy), text, fill=(250, 238, 210, 255), font=font)
    img.convert("RGB").save(path)


def logo(path):
    """Small square pack logo: a red tile with a revolver crossed over a rope ring and 'AP'."""
    img, d = _canvas("deadeye")
    d.ellipse((10 * S, 10 * S, 54 * S, 54 * S), outline=(232, 200, 140, 255), width=3 * S)
    font = ImageFont.truetype(FONT, 22 * S)
    w = d.textlength("AP", font=font)
    d.text(((N - w) / 2, 19 * S), "AP", font=font, fill=(250, 238, 210, 255))
    _finish(img, path)
