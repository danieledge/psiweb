#!/usr/bin/env python3
"""PsiWeb's toolbar pictures (original pixel art, in the spirit of the
Psion's own programs) -> BMP files for bmconv, and app/pwicons.h with
their ids. As mail/tools/mkicons.py.

    web/tools/mkicons.py OUTDIR HEADER

The toolband's pictures (BAND_ICONS) are 16x16 and in colour (/c8).

Each toolbar picture is 24x20 ASCII art (narrower if its word needs the room):
k black, d dark grey, l light grey, w white, . transparent. Every picture gets a 1-bit mask right after it in
the MBM (black = drawn), so ids go picture, mask, picture, mask...
"""
import sys, os, math
from PIL import Image

GREY = {'k': 0, 'd': 85, 'l': 170, 'w': 255, '.': 255}

ICONS = [
# Open address: a globe
('ToolOpen', """
........kkkkkkkk........
......kkwwwwkwwwkk......
.....kwwwwwwkwwwwwk.....
....kwwwwwwwkwwwwwwk....
...kwwwwwwwwkwwwwwwwk...
...kwwwkwwwwkwwwwkwwk...
..kwwwwwkkkkkkkkkwwwwk..
..kwwwwwwwwwkwwwwwwwwk..
..kwwwwwwwwwkwwwwwwwwk..
..kkkkkkkkkkkkkkkkkkkk..
..kwwwwwwwwwkwwwwwwwwk..
..kwwwwwwwwwkwwwwwwwwk..
..kwwwwwkkkkkkkkkwwwwk..
...kwwwkwwwwkwwwwkwwk...
...kwwwwwwwwkwwwwwwwk...
....kwwwwwwwkwwwwwwk....
.....kwwwwwwkwwwwwk.....
......kkwwwwkwwwkk......
........kkkkkkkk........
........................
"""),
# Back: an arrow to the left
('ToolBack', """
........................
........................
.......k................
......kk................
.....kwk................
....kwwkkkkkkkkkkkkkk...
...kwwwwwwwwwwwwwwwwk...
..kwwwwwwwwwwwwwwwwwk...
.kwwwwwwwwwwwwwwwwwwk...
..kwwwwwwwwwwwwwwwwwk...
...kwwwwwwwwwwwwwwwwk...
....kwwkkkkkkkkkkkkkk...
.....kwk................
......kk................
.......k................
........................
........................
........................
........................
........................
"""),
# Start: the start page and its favourites, a star (0.99: a house until
# 0.98, which is the Home page's idea, not the start page's)
('ToolHome', None),
# Pictures: a framed picture of hills under the sun (20 wide, so that
# the button's word fits beside it)
('ToolPictures', """
....................
kkkkkkkkkkkkkkkkkkkk
kwwwwwwwwwwwwwwwwwwk
kwwwwwwwwwwwwkkkwwwk
kwwwwwwwwwwwkdddkwwk
kwwwwwwwwwwwkdddkwwk
kwwwwwwwwwwwkdddkwwk
kwwwwwkwwwwwwkkkwwwk
kwwwwkdkwwwwwwwwwwwk
kwwwkdddkwwwwwwwwwwk
kwwkdddddkwwwwkwwwwk
kwkdddddddkwwkdkwwwk
kkdddddddddkkdddkwwk
kddddddddddddddddkwk
kdddddddddddddddddkk
kddddddddddddddddddk
kllllllllllllllllllk
kllllllllllllllllllk
kkkkkkkkkkkkkkkkkkkk
....................
"""),
]


# The toolband's pictures (0.78): 16x16, in colour, for the netBook's
# toolband (on the 5mx its 16 greys show them as greys).
# (1.03) Redrawn to one grid and Coda's recipe (style sheet 2.3), so that
# they sit alike on their buttons: each object is filled within the same
# 12x12 square (pixels 2 to 13 both ways, centred in the 16x16 picture,
# which EIKON centres on the button), with a 1-pixel 333333 outline round
# it (so 14x14 in all), flat fills from the 6x6x6 cube, a lighter edge at
# the top left and a darker one at the bottom right, and no drop shadow
# (on a 16-pixel picture it would move the object off centre). Back and
# Forward have a dimmed pair, 999999 and cccccc with no edges, for when
# there is no page to go to.
N = 16
OUT16 = (0x33, 0x33, 0x33)
C16 = {'n': (0x00, 0x33, 0x66), 'b': (0x00, 0x66, 0xcc), 'p': (0x99, 0xcc, 0xff), 't': (0x00, 0x99, 0x99),
       'o': (0xff, 0x66, 0x00), 'w': (0xff, 0xff, 0xff), 'g': (0xcc, 0xcc, 0xcc), 'v': (0xcc, 0xcc, 0xff),
       'y': (0xff, 0xcc, 0x00), 'd': (0x66, 0x66, 0x66), 'm': (0x99, 0x99, 0x99), 'r': (0xcc, 0x33, 0x00)}
def g16(): return [[None] * N for _ in range(N)]
def f16(g, test, c, edge=True):
    """fills the pixels at least half inside test(x, y) (8x8 samples a
    pixel: no single-pixel spurs); c a C16 letter; edge: the recipe's light
    and dark edges apply to this fill"""
    for y in range(N):
        for x in range(N):
            n = sum(1 for i in range(8) for j in range(8) if test(x + (i + .5) / 8, y + (j + .5) / 8))
            if n >= 32: g[y][x] = (C16[c], edge)
def a16(g, art, c, edge=True):
    """fills from 16 lines of art: any letter is the fill, '.' nothing"""
    rows = [r for r in art.strip('\n').split('\n')]
    assert len(rows) == N and all(len(r) == N for r in rows), art
    for y, r in enumerate(rows):
        for x, ch in enumerate(r):
            if ch != '.': g[y][x] = (C16[c], edge)
def inpoly(p, x, y):
    n, c = len(p), False
    for i in range(n):
        x1, y1 = p[i]; x2, y2 = p[(i+1) % n]
        if (y1 > y) != (y2 > y) and x < (x2-x1)*(y-y1)/(y2-y1) + x1: c = not c
    return c
def fin16(g, outline=OUT16, edges=True):
    """the outline round the object; the light and dark edges"""
    filled = lambda x, y: 0 <= x < N and 0 <= y < N and g[y][x] is not None
    out = [[None] * N for _ in range(N)]
    for y in range(N):
        for x in range(N):
            if not filled(x, y): continue
            c, e = g[y][x]
            if edges and e and (not filled(x - 1, y) or not filled(x, y - 1)): c = step(c, 1)
            elif edges and e and (not filled(x + 1, y) or not filled(x, y + 1)): c = step(c, -1)
            out[y][x] = c
    for y in range(N):
        for x in range(N):
            if not filled(x, y) and any(filled(x + dx, y + dy) for dx, dy in ((1,0),(-1,0),(0,1),(0,-1))):
                out[y][x] = outline
    return out
def step(c, d):
    return tuple(max(0, min(255, v + d * 0x33)) for v in c)
# Back, drawn by hand on the 12x12 square: the head's two halves and the
# shaft mirror each other about the middle, and Forward mirrors Back
ARROW = """
................
................
.......#........
......##........
.....###........
....##########..
...###########..
..############..
..############..
...###########..
....##########..
.....###........
......##........
.......#........
................
................
"""
def arrow(left, dim=False):
    g = g16()
    art = ARROW if left else '\n'.join(r[::-1] for r in ARROW.strip('\n').split('\n'))
    a16(g, art, 'g' if dim else 'b')
    return fin16(g, (0x99, 0x99, 0x99), False) if dim else fin16(g)
RELOAD = """
................
................
....#####.#.....
...######.##....
..##......###...
..##......##....
..##......#.....
..##............
..##........##..
..##........##..
..###......###..
...####..####...
....########....
.....######.....
................
................
"""
def reload_():
    # a ring, open at the top right, with the arrowhead there turning it
    # clockwise; by hand on the 12x12 square
    g = g16()
    a16(g, RELOAD, 'b', False)           # (a 2-pixel stroke: no edges, which would stripe it)
    return fin16(g)
def stop():
    # a stop sign: an octagon in Coda's orange-red, a white cross
    g = g16()
    oct_ = [(5.5, 2), (10.5, 2), (14, 5.5), (14, 10.5), (10.5, 14), (5.5, 14), (2, 10.5), (2, 5.5)]
    f16(g, lambda x, y: inpoly(oct_, x, y), 'r')
    def cross(x, y):
        u, v = x - 8, y - 8
        return abs(u) <= 3.2 and abs(v) <= 3.2 and (abs(u - v) <= 1.0 or abs(u + v) <= 1.0)
    f16(g, cross, 'w', False)
    return fin16(g)
STAR = """
................
................
.......##.......
.......##.......
......####......
..############..
...##########...
....########....
.....######.....
.....######.....
....########....
....###..###....
...###....###...
...##......##...
................
................
"""
def star16():
    # Start: the favourites' star, blue as the toolbar's Start; by hand,
    # its two sides mirror each other
    g = g16()
    a16(g, STAR, 'b')
    return fin16(g)
def pictures():
    # a framed picture: pale sky, white sun, teal and blue hills (the
    # toolbar's Pictures), 12x10 inside its outline
    g = g16()
    box = lambda x, y: 2 <= x <= 14 and 3 <= y <= 13
    f16(g, box, 'v', False)
    f16(g, lambda x, y: math.hypot(x - 10.8, y - 5.8) <= 1.6, 'w', False)
    f16(g, lambda x, y: box(x, y) and y >= 13.2 - 5 * math.exp(-((x - 5.5) / 2.8) ** 2), 't', False)
    f16(g, lambda x, y: box(x, y) and y >= 13.2 - 2.6 * math.exp(-((x - 11) / 2.2) ** 2), 'b', False)
    return fin16(g)
def lock(closed):
    # (0.86) the route light's padlock: (1.03) shut (teal) for a secure
    # page checked on the way, open (grey) for any other; the body 10x7,
    # the shackle above it
    g = g16()
    f16(g, lambda x, y: 3 <= x <= 13 and 7.5 <= y <= 14, 't' if closed else 'g')
    if closed:
        shackle = lambda x, y: 2.2 <= math.hypot(x - 8, y - 6.5) <= 3.8 and y <= 7.5
    else:
        shackle = lambda x, y: 2.2 <= math.hypot(x - 10.5, y - 5) <= 3.8 and y <= 6 and not (x < 8.5 and y > 4)
    f16(g, shackle, 'm', False)
    f16(g, lambda x, y: 7 <= x <= 9 and 9.5 <= y <= 12, 'n' if closed else 'd', False)
    return fin16(g)
BAND_ICONS = [('BandBack', arrow(True)), ('BandForward', arrow(False)), ('BandReload', reload_()),
         ('BandStop', stop()), ('BandHome', star16()), ('BandPictures', pictures()),
         ('BandLock', lock(True)), ('BandNoLock', lock(False))]
# (1.03) after every other picture, so the ids before them stay as they were
BAND_DIM = [('BandBackDim', arrow(True, True)), ('BandForwardDim', arrow(False, True))]


# (0.99) The toolbar's pictures in colour, for the netBook (Coda's style
# sheet 2.3): one object, a 1-pixel 333333 outline, flat fills from the
# 6x6x6 cube with a lighter edge at the top left and a darker one at the
# bottom right, and a 2-pixel 999999 shadow at the bottom right; Coda's
# blues, at most three hues. 24x20 (the shadow inside), hard edges: the
# masks are 1-bit.
CW, CH = 24, 20
CODA = {'n': (0x00, 0x33, 0x66), 'b': (0x00, 0x66, 0xcc), 'p': (0x99, 0xcc, 0xff),
        't': (0x00, 0x99, 0x99), 'o': (0xff, 0x66, 0x00), 'w': (0xff, 0xff, 0xff),
        'g': (0xcc, 0xcc, 0xcc), 'v': (0xcc, 0xcc, 0xff), 'y': (0xff, 0xcc, 0x00)}
OUT, SHADOW = (0x33, 0x33, 0x33), (0x99, 0x99, 0x99)
def step(c, d):
    return tuple(max(0, min(255, v + d * 0x33)) for v in c)
def cgrid(): return [[None] * CW for _ in range(CH)]
def cfill(g, test, c):
    for y in range(CH):
        for x in range(CW):
            if test(x + .5, y + .5): g[y][x] = CODA[c] if isinstance(c, str) else c
def finish(g):
    """the outline round the object, the light and dark edges, the shadow"""
    H, W = CH, CW
    filled = lambda x, y: 0 <= x < W and 0 <= y < H and g[y][x] is not None
    out = [r[:] for r in g]
    for y in range(H):
        for x in range(W):
            if not filled(x, y): continue
            c = g[y][x]
            if not filled(x - 1, y) or not filled(x, y - 1): out[y][x] = step(c, 1)
            elif not filled(x + 1, y) or not filled(x, y + 1): out[y][x] = step(c, -1)
    o2 = [r[:] for r in out]
    for y in range(H):
        for x in range(W):
            if filled(x, y): continue
            if any(filled(x + dx, y + dy) for dx, dy in ((1,0),(-1,0),(0,1),(0,-1))): o2[y][x] = OUT
    o3 = [r[:] for r in o2]
    for y in range(H):
        for x in range(W):
            if o2[y][x] is None and 0 <= y - 2 and 0 <= x - 2 and o2[y-2][x-2] is not None: o3[y][x] = SHADOW
    return o3
def c_globe():
    g = cgrid()
    cx, cy, r = 10.5, 9.5, 8.2
    disc = lambda x, y: math.hypot(x - cx, y - cy) <= r
    cfill(g, disc, 'p')
    # the land in the Coda blue, the meridians and equator in navy
    cfill(g, lambda x, y: disc(x, y) and ((x - 7) ** 2 / 9 + (y - 7) ** 2 / 12 <= 1 or (x - 14) ** 2 / 6 + (y - 13) ** 2 / 9 <= 1), 'b')
    cfill(g, lambda x, y: disc(x, y) and (abs(y - cy) < .6 or abs(x - cx) < .6 or abs(math.hypot((x - cx) * 2.1, y - cy) - r) < 1.0), 'n')
    return finish(g)
def c_arrow():
    g = cgrid()
    p = [(1.5, 9.5), (9, 2), (9, 6), (19.5, 6), (19.5, 13), (9, 13), (9, 17)]
    cfill(g, lambda x, y: inpoly(p, x, y), 'b')
    return finish(g)
def c_star():
    g = cgrid()
    pts = [(10.5 + 9.3 * math.cos(math.radians(-90 + 36 * k)) * (1 if k % 2 == 0 else 0.42),
            10 + 9.3 * math.sin(math.radians(-90 + 36 * k)) * (1 if k % 2 == 0 else 0.42)) for k in range(10)]
    cfill(g, lambda x, y: inpoly(pts, x, y), 'b')
    return finish(g)
def c_pictures():
    # 20 wide, its last 4 columns left empty (cut off below), so that the
    # button's word "Pictures" fits beside it, as the greys' picture does
    g = cgrid()
    box = lambda x, y: 1 <= x <= 16 and 2 <= y <= 16
    cfill(g, box, 'v')
    cfill(g, lambda x, y: math.hypot(x - 12.5, y - 6) <= 2.1, 'w')
    cfill(g, lambda x, y: box(x, y) and y >= 16.5 - 7 * math.exp(-((x - 6) / 3.6) ** 2), 't')
    cfill(g, lambda x, y: box(x, y) and y >= 16 - 3.5 * math.exp(-((x - 13) / 2.8) ** 2), 'b')
    return [row[:20] for row in finish(g)]
COLOUR_ICONS = [('ToolOpenC', c_globe()), ('ToolBackC', c_arrow()), ('ToolHomeC', c_star()), ('ToolPicturesC', c_pictures())]


def star_art():
    # the 5mx's Start: the star in four greys, as the other pictures
    W, H = 24, 20
    pts = [(12 + 9.6 * math.cos(math.radians(-90 + 36 * k)) * (1 if k % 2 == 0 else 0.42),
            10.6 + 9.6 * math.sin(math.radians(-90 + 36 * k)) * (1 if k % 2 == 0 else 0.42)) for k in range(10)]
    a = [['.'] * W for _ in range(H)]
    for y in range(H):
        for x in range(W):
            if inpoly(pts, x + .5, y + .5): a[y][x] = 'w'
    o = [r[:] for r in a]
    for y in range(H):
        for x in range(W):
            if a[y][x] != 'w': continue
            if any(not (0 <= x+dx < W and 0 <= y+dy < H) or a[y+dy][x+dx] == '.' for dx, dy in ((1,0),(-1,0),(0,1),(0,-1))):
                o[y][x] = 'k'
    return '\n' + '\n'.join(''.join(r) for r in o) + '\n'
ICONS = [(n, star_art() if art is None else art) for n, art in ICONS]


def parse(art):
    rows = [r for r in art.strip('\n').split('\n')]
    w = max(len(r) for r in rows)
    rows = [r.ljust(w, '.') for r in rows]
    return rows, w, len(rows)


def main():
    out, header = sys.argv[1], sys.argv[2]
    os.makedirs(out, exist_ok=True)
    files = []
    ids = []
    for name, art in ICONS:
        rows, w, h = parse(art)
        if h != 20 or not 16 <= w <= 24:
            raise SystemExit(f'{name}: {w}x{h}, toolbar pictures are 24x20 (or narrower)')
        img = Image.new('L', (w, h), 255)
        mask = Image.new('1', (w, h), 1)
        for y, r in enumerate(rows):
            for x, c in enumerate(r):
                if c not in GREY:
                    raise SystemExit(f'{name}: bad pixel {c!r}')
                img.putpixel((x, y), GREY[c])
                if c != '.':
                    mask.putpixel((x, y), 0)
        fi = os.path.join(out, f'{name}.bmp')
        fm = os.path.join(out, f'{name}_m.bmp')
        img.convert('RGB').save(fi)
        mask.convert('RGB').save(fm)
        files += [fi, fm]
        ids.append(name)
    kinds = ['/2', '/1'] * len(ids)
    for name, a in BAND_ICONS:
        img = Image.new('RGB', (N, N), (255, 255, 255))
        mask = Image.new('1', (N, N), 1)
        for y in range(N):
            for x in range(N):
                if a[y][x] is not None:
                    img.putpixel((x, y), a[y][x])
                    mask.putpixel((x, y), 0)
        fi = os.path.join(out, f'{name}.bmp')
        fm = os.path.join(out, f'{name}_m.bmp')
        img.save(fi)
        mask.convert('RGB').save(fm)
        files += [fi, fm]
        kinds += ['/c8', '/1']
        ids.append(name)
    for name, g in COLOUR_ICONS + [(n, a) for n, a in BAND_DIM]:   # (0.99, 1.03) after the others: the ids before stay as they were
        gw, gh = len(g[0]), len(g)       # (24, or 20 for Pictures; 16x16 the dimmed toolband pair)
        img = Image.new('RGB', (gw, gh), (255, 255, 255))
        mask = Image.new('1', (gw, gh), 1)
        for y in range(gh):
            for x in range(gw):
                if g[y][x] is not None:
                    img.putpixel((x, y), g[y][x])
                    mask.putpixel((x, y), 0)
        fi = os.path.join(out, f'{name}.bmp')
        fm = os.path.join(out, f'{name}_m.bmp')
        img.save(fi)
        mask.convert('RGB').save(fm)
        files += [fi, fm]
        kinds += ['/c8', '/1']
        ids.append(name)
    with open(header, 'w') as f:
        f.write('/* pwicons.h - made by web/tools/mkicons.py: PsiWeb.mbm ids */\n')
        f.write('#ifndef PWICONS_H\n#define PWICONS_H\nenum TPwIconId\n\t{\n')
        for i, n in enumerate(ids):
            f.write(f'\tEMbm{n} = {2 * i}, EMbm{n}Mask = {2 * i + 1},\n')
        f.write(f'\tEMbmCount = {2 * len(ids)}\n\t}};\n#endif\n')
    # the same ids for the resource file (the toolband's buttons name
    # their pictures there: a picture-less, word-less button cannot be made)
    with open(os.path.splitext(header)[0] + '.rh', 'w') as f:
        f.write('// pwicons.rh - made by web/tools/mkicons.py: PsiWeb.mbm ids for psiweb.rss\n')
        for i, n in enumerate(ids):
            f.write(f'#define EMbm{n} {2 * i}\n#define EMbm{n}Mask {2 * i + 1}\n')
    with open(os.path.join(out, 'files.txt'), 'w') as f:
        for k, p in zip(kinds, files):
            f.write(k + os.path.basename(p) + '\n')


if __name__ == '__main__':
    main()
