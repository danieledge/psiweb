/* psi_colour.c - the Links engine's frame on a colour screen (the netBook):
 * RGB565 to EPOC's 256 colours, one byte a pixel (an EColor256 bitmap's
 * layout), and the tables psi_drv.c dithers pictures with.
 *
 * EPOC's palette (psi_pal.h) is a 6x6x6 cube with 16-step ramps of grey,
 * red, green and blue. Every 565 colour gets its nearest entry, by a
 * distance weighted for the eye (red 2, green 4, blue 3, as PsiView's).
 * The distance is a sum over the channels, so the nearest is found
 * exactly from five candidates rather than all 256:
 *   - in the cube, each channel's own nearest step;
 *   - on the grey ramp, the step nearest the weighted mean;
 *   - on each colour ramp (one channel, the others 0), that channel's step.
 * The table (64 KB) is made once, at the first use, only on a colour
 * screen; the 5mx never makes it.
 *
 * Everything but pictures (text and its anti-aliased edges, rules,
 * backgrounds) takes the nearest colour, with no dither: text stays crisp,
 * and the 16 greys keep its edges smooth. Pictures are dithered once, by
 * error diffusion, when psi_drv.c packs them to one byte a pixel (palette
 * indexes); drawing turns an index back into a 565 pixel that this table
 * maps to the same index (pw_colour_565), so a picture costs nothing more
 * at each redraw. */
#include <stdlib.h>
#include <string.h>
#include "pwback.h"
#include "psi_pal.h"

static unsigned char *ct;		/* 565 -> palette index */
static unsigned short i2p[256];		/* palette index -> its 565 pixel */
static unsigned char rgb[3 * 256];	/* palette index -> r, g, b */

#define WR 2
#define WG 4
#define WB 3

static int dist(int r, int g, int b, int i)
{
	int dr = r - rgb[3 * i], dg = g - rgb[3 * i + 1], db = b - rgb[3 * i + 2];
	return WR * dr * dr + WG * dg * dg + WB * db * db;
}

/* 1 if made (or there already) */
int pw_colour_init(void)
{
	static unsigned char cube[6][6][6], ramp[4][16];	/* grey, red, green, blue */
	unsigned char n6[256], n16[256];	/* the nearest cube step and ramp step */
	int i, p;
	if (ct) return 1;
	ct = malloc(65536);
	if (!ct) return 0;
	memset(cube, 0, sizeof(cube));
	memset(ramp, 0, sizeof(ramp));
	for (i = 0; i < 256; i++) {
		unsigned long c = psi_epoc256[i];		/* 0x00BBGGRR */
		int r = (int)(c & 255), g = (int)((c >> 8) & 255), b = (int)((c >> 16) & 255);
		rgb[3 * i] = (unsigned char)r;
		rgb[3 * i + 1] = (unsigned char)g;
		rgb[3 * i + 2] = (unsigned char)b;
		i2p[i] = (unsigned short)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
		if (r % 51 == 0 && g % 51 == 0 && b % 51 == 0) cube[r / 51][g / 51][b / 51] = (unsigned char)i;
		if (r == g && g == b && r % 17 == 0) ramp[0][r / 17] = (unsigned char)i;
		if (!g && !b && r % 17 == 0) ramp[1][r / 17] = (unsigned char)i;
		if (!r && !b && g % 17 == 0) ramp[2][g / 17] = (unsigned char)i;
		if (!r && !g && b % 17 == 0) ramp[3][b / 17] = (unsigned char)i;
	}
	/* (no divide instruction on the ARM: the divisions once, in tables) */
	for (i = 0; i < 256; i++) {
		n6[i] = (unsigned char)((i + 25) / 51);
		n16[i] = (unsigned char)((i + 8) / 17);
	}
	for (p = 0; p < 65536; p++) {
		int r5 = p >> 11, g6 = (p >> 5) & 63, b5 = p & 31;
		int r = (r5 << 3) | (r5 >> 2), g = (g6 << 2) | (g6 >> 4), b = (b5 << 3) | (b5 >> 2);
		/* the weighted mean: (2r + 4g + 3b + 4) / 9, exact for these sums */
		int m = ((WR * r + WG * g + WB * b + 4) * 7282) >> 16;
		int c[5], k, best, bd;
		c[0] = cube[n6[r]][n6[g]][n6[b]];
		c[1] = ramp[0][n16[m]];
		c[2] = ramp[1][n16[r]];
		c[3] = ramp[2][n16[g]];
		c[4] = ramp[3][n16[b]];
		best = c[0];
		bd = dist(r, g, b, best);
		for (k = 1; k < 5; k++) {
			int d = dist(r, g, b, c[k]);
			if (d < bd) bd = d, best = c[k];
		}
		ct[p] = (unsigned char)best;
	}
	/* each entry's own 565 pixel comes back to it (they are all different:
	   two entries differ by at least 17 in a channel) */
	for (i = 0; i < 256; i++) ct[i2p[i]] = (unsigned char)i;
	return 1;
}

const unsigned char *pw_colour_table(void) { return ct; }
const unsigned char *pw_colour_rgb(void) { return rgb; }
unsigned int pw_colour_565(int i) { return i2p[i & 255]; }

/* the frame's rows y0 to y1 (columns x0 to x1) into 'out', a byte a pixel.
   A page is mostly paper and ink: a pair of white or of black pixels (one
   word) is written straight out */
void pw_colour_convert(const unsigned short *fb, int fbw, unsigned char *out, int stride,
                       int x0, int y0, int x1, int y1)
{
	const unsigned char *t;
	int y;
	if (!ct && !pw_colour_init()) return;
	t = ct;
	x0 &= ~1;
	if (x1 & 1) x1++;
	if (x1 > fbw) x1 = fbw;
	for (y = y0; y < y1; y++) {
		/* (fb is word-aligned and fbw even, so each pair is one word) */
		const unsigned int *s = (const unsigned int *)(fb + y * fbw + x0);
		const unsigned int *e = s + ((x1 - x0) >> 1);
		unsigned char *d = out + y * stride + x0;
		unsigned char white = t[0xffff], black = t[0];
		for (; s < e; s++, d += 2) {
			unsigned int w = *s;
			if (w == 0xffffffffu) d[0] = d[1] = white;
			else if (!w) d[0] = d[1] = black;
			else {
				d[0] = t[w & 0xffff];
				d[1] = t[w >> 16];
			}
		}
	}
}
