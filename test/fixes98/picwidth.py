#!/usr/bin/env python3
"""picwidth.py SHOT MIN MAX - the widest run of black (or near-black) pixels
on any row of a harness screenshot (640 pixels across, drawn at twice that
in the PNG), in screen pixels, must be between MIN and MAX."""
import sys
from PIL import Image
im = Image.open(sys.argv[1]).convert('L')
w, h = im.size
scale = w // 640
best = 0
for y in range(h):
    run = 0
    for x in range(w):
        if im.getpixel((x, y)) < 40:
            run += 1
            best = max(best, run)
        else:
            run = 0
best //= scale
print('widest black run: %d pixels' % best)
sys.exit(0 if int(sys.argv[2]) <= best <= int(sys.argv[3]) else 1)
