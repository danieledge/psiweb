#!/usr/bin/env python3
"""same.py A B same|differ - whether two screenshots differ"""
import sys
from PIL import Image, ImageChops
d = ImageChops.difference(Image.open(sys.argv[1]).convert('L'), Image.open(sys.argv[2]).convert('L')).getbbox()
print('differ' if d else 'same', d or '')
sys.exit(0 if (d is not None) == (sys.argv[3] == 'differ') else 1)
