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
# Home page: a house
('ToolHome', """
...........kk...........
..........kwwk..........
.........kwwwwk.........
........kwwwwwwk........
.......kwwwwwwwwk.......
......kwwwwwwwwwwk......
.....kwwwwwwwwwwwwk.....
....kwwwwwwwwwwwwwwk....
...kwwwwwwwwwwwwwwwwk...
..kkkkwwwwwwwwwwwwkkkk..
.....kwwwwwwwwwwwwk.....
.....kwwkkkwwwkkkwk.....
.....kwwkwkwwwkwkwk.....
.....kwwkkkwwwkkkwk.....
.....kwwwwwwwwwwwwk.....
.....kwwwwwkkkkwwwk.....
.....kwwwwwkwwkwwwk.....
.....kwwwwwkwwkwwwk.....
.....kkkkkkkkkkkkkk.....
........................
"""),
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
BAND_ICONS = [('BandBack', arrow(True)), ('BandForward', arrow(False)), ('BandReload', reload_()),
         ('BandStop', stop()), ('BandHome', home()), ('BandPictures', pictures())]


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
