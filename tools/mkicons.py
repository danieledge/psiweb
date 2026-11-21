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
# toolband (on the 5mx its 16 greys show them as greys). Drawn from shapes,
# each with a black outline. Colours from EPOC's 6x6x6 cube, so they show exactly on the netBook
COL = {'k': (0,0,0), 'w': (255,255,255), 'b': (51,102,204), 'B': (153,204,255),
       'g': (51,153,51), 'G': (153,204,102), 'r': (204,51,51), 'R': (255,153,153),
       'y': (255,204,51), 'd': (102,102,102), 'l': (204,204,204)}
N = 16
def blank(): return [['.'] * N for _ in range(N)]
def outline(a):
    o = [r[:] for r in a]
    for y in range(N):
        for x in range(N):
            if a[y][x] != '.': continue
            for dx, dy in ((1,0),(-1,0),(0,1),(0,-1)):
                X, Y = x+dx, y+dy
                if 0 <= X < N and 0 <= Y < N and a[Y][X] not in '.k':
                    o[y][x] = 'k'; break
    return o
def fill(a, test, c):
    for y in range(N):
        for x in range(N):
            if test(x + .5, y + .5): a[y][x] = c
def inpoly(p, x, y):
    n, c = len(p), False
    for i in range(n):
        x1, y1 = p[i]; x2, y2 = p[(i+1) % n]
        if (y1 > y) != (y2 > y) and x < (x2-x1)*(y-y1)/(y2-y1) + x1: c = not c
    return c
def arrow(left):
    a = blank()
    p = [(1.5,8),(7,2.5),(7,5.5),(14,5.5),(14,10.5),(7,10.5),(7,13.5)]
    if not left: p = [(N - x, y) for x, y in p]
    fill(a, lambda x, y: inpoly(p, x, y), 'b')
    # a light edge along the top, for depth
    for y in range(N):
        for x in range(N):
            if a[y][x] == 'b' and (y == 0 or a[y-1][x] == '.'): a[y][x] = 'B'
    return outline(a)
def reload_():
    a = blank()
    cx, cy = 8, 8.5
    def ring(x, y):
        r = math.hypot(x-cx, y-cy); ang = math.degrees(math.atan2(-(y-cy), x-cx)) % 360
        return 4.0 <= r <= 6.3 and not (15 <= ang <= 85)
    fill(a, ring, 'b')
    head = [(8.2,0.4),(13.4,3.6),(8.2,6.8)]
    fill(a, lambda x, y: inpoly(head, x, y), 'b')
    return outline(a)
def stop():
    a = blank()
    fill(a, lambda x, y: math.hypot(x-8, y-8) <= 6.9, 'r')
    def cross(x, y):
        return math.hypot(x-8, y-8) <= 4.6 and (abs((x-8)-(y-8)) <= 1.1 or abs((x-8)+(y-8)) <= 1.1)
    fill(a, cross, 'w')
    return outline(a)
def home():
    a = blank()
    roof = [(8,1.2),(15.4,8.2),(0.6,8.2)]
    fill(a, lambda x, y: 3 <= x <= 13 and 8 <= y <= 15, 'l')
    fill(a, lambda x, y: inpoly(roof, x, y), 'r')
    fill(a, lambda x, y: 7 <= x <= 10 and 10.5 <= y <= 15, 'b')
    o = outline(a)
    return o
def pictures():
    a = blank()
    fill(a, lambda x, y: 1 <= x <= 15 and 2 <= y <= 14, 'B')
    fill(a, lambda x, y: math.hypot(x-11.5, y-5.5) <= 2.1, 'y')
    fill(a, lambda x, y: 1 <= x <= 15 and 2 <= y <= 14 and y >= 14 - 5*math.exp(-((x-5.5)/3.2)**2), 'g')
    fill(a, lambda x, y: 1 <= x <= 15 and 2 <= y <= 14 and y >= 13.6 - 2.5*math.exp(-((x-12)/2.5)**2), 'G')
    # the frame
    for y in range(N):
        for x in range(N):
            if (x in (0, 15) and 1 <= y <= 14) or (y in (1, 14) and 0 <= x <= 15): a[y][x] = 'k'
    return a
def lock(closed):
    # (0.86) the route light's padlock: closed for a secure page checked on
    # the way, nothing for a plain one
    a = blank()
    if not closed: return a
    fill(a, lambda x, y: 3 <= x <= 13 and 7 <= y <= 14, 'y')
    def shackle(x, y):
        r = math.hypot(x-8, y-7)
        return 3.0 <= r <= 4.6 and y <= 7.5
    fill(a, shackle, 'd')
    fill(a, lambda x, y: 7 <= x <= 9 and 9.5 <= y <= 12, 'k')
    return outline(a)
def star16():
    # (0.99) Start: the favourites' star, in place of the house
    a = blank()
    pts = [(8 + 7.4 * math.cos(math.radians(-90 + 36 * k)) * (1 if k % 2 == 0 else 0.42),
            8.6 + 7.4 * math.sin(math.radians(-90 + 36 * k)) * (1 if k % 2 == 0 else 0.42)) for k in range(10)]
    fill(a, lambda x, y: inpoly(pts, x, y), 'y')
    return outline(a)
def lock_open():
    # (0.99) the route light's open padlock: a page not checked on the way
    a = blank()
    fill(a, lambda x, y: 3 <= x <= 13 and 7 <= y <= 14, 'l')
    def shackle(x, y):
        r = math.hypot(x-11, y-6)
        return 2.6 <= r <= 4.2 and y <= 7.5 and not (x < 8.5 and y > 5)
    fill(a, shackle, 'd')
    fill(a, lambda x, y: 7 <= x <= 9 and 9.5 <= y <= 12, 'd')
    return outline(a)
BAND_ICONS = [('BandBack', arrow(True)), ('BandForward', arrow(False)), ('BandReload', reload_()),
         ('BandStop', stop()), ('BandHome', star16()), ('BandPictures', pictures()),
         ('BandLock', lock(True)), ('BandNoLock', lock_open())]


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
                if a[y][x] != '.':
                    img.putpixel((x, y), COL[a[y][x]])
                    mask.putpixel((x, y), 0)
        fi = os.path.join(out, f'{name}.bmp')
        fm = os.path.join(out, f'{name}_m.bmp')
        img.save(fi)
        mask.convert('RGB').save(fm)
        files += [fi, fm]
        kinds += ['/c8', '/1']
        ids.append(name)
    for name, g in COLOUR_ICONS:        # (0.99) after the others: the ids before stay as they were
        gw = len(g[0])                   # (24, or 20 for Pictures)
        img = Image.new('RGB', (gw, CH), (255, 255, 255))
        mask = Image.new('1', (gw, CH), 1)
        for y in range(CH):
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
