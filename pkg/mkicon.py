#!/usr/bin/env python3
"""PsiWeb's icon: a flat globe in Coda's icon style (0.99; the UI review's
style sheet 2.3 and its design audit DA-19), drawn with Coda's icon kit
(codaicon.py, a copy of psion-netbook's psiicons/art/codaicon.py, with
epoc256.py, EPOC's palette) at 24, 32 and 48 pixels, and written to
PsiWeb.aif as 8-bit colour bitmaps with their masks, compressed as Psion's
own .aif files are. One .aif serves both machines: the netBook shows the
colours, the Series 5mx each colour by its brightness in its greys.

The globe: pale blue (99ccff) seas, Coda's blue (0066cc) land and navy
(003366) meridians, on a 1-pixel 333333 outline with the kit's edges and
shadow; three hues at most, cube colours only (the kit checks). At 24 the
globe, one piece of land, the equator and the middle meridian; the curved
meridians and the second piece of land from 32; the parallels at 48.

    python3 pkg/mkicon.py [OUT.aif]     (build.sh: the package's PsiWeb.aif)
    python3 pkg/mkicon.py               also icon-preview.png, the three sizes x3
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import codaicon  # noqa: E402
from codaicon import BLUE, NAVY  # noqa: E402

SEA = '99ccff'


def globe(c):
    # the object within y 2..44 and x 1..45 of the 48 grid (the kit's margins)
    c.ellipse(3, 2, 45, 44, SEA)
    # the land, flat (a map's print: no outline of its own)
    c.poly([(10, 12), (19, 8), (25, 13), (22, 21), (25, 28), (17, 33), (12, 26), (8, 19)], BLUE, edge=False, bevel=False)
    c.poly([(29, 27), (37, 25), (40, 32), (33, 39), (28, 35)], BLUE, only=(32, 48), edge=False, bevel=False)
    # the lines, flat, inside the outline
    c.line([(6, 23), (42, 23)], 2, NAVY)                             # the equator
    c.line([(24, 5), (24, 41)], 2, NAVY)                             # the middle meridian
    c.arc(13, 3, 35, 43, 0, 360, 1.5, NAVY, only=(32, 48))           # the curved meridians
    c.line([(9, 13), (39, 13)], 1.5, NAVY, only=(48,))               # the parallels
    c.line([(9, 33), (39, 33)], 1.5, NAVY, only=(48,))


def icons():
    return codaicon.icons(globe, 'PsiWeb')


if __name__ == '__main__':
    ic = icons()
    out = sys.argv[1] if len(sys.argv) > 1 else 'psiweb.aif'
    print(out, codaicon.write_aif(out, 0x01000A7A, [(1, 'PsiWeb'), (10, 'PsiWeb')], ic), 'bytes')
    if len(sys.argv) == 1:
        codaicon.preview('icon-preview.png', [('PsiWeb', ic)])
