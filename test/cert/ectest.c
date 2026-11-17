/* ectest.c - core's ECDSA check (ecverify.h, through certcheck.c) on the
 * cases mkvec.py makes: ectest VECTORS. Prints each failure and a count. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int tlsv_ecdsa_verify(int bits, const unsigned char *pub, int publen, const unsigned char *hash, int hlen,
                      const unsigned char *sig, int siglen);

static int unhex(const char *h, unsigned char *out, int max)
{
	int n = 0;
	while (h[0] && h[1] && n < max) {
		unsigned v;
		sscanf(h, "%2x", &v);
		out[n++] = (unsigned char)v;
		h += 2;
	}
	return n;
}

int main(int argc, char **argv)
{
	static char line[2048], pk[512], hh[256], sg[512], why[64];
	unsigned char p[128], h[64], s[160];
	int bits, want, pass = 0, fail = 0;
	FILE *f = argc > 1 ? fopen(argv[1], "r") : 0;
	if (!f) { fprintf(stderr, "usage: ectest VECTORS\n"); return 2; }
	while (fgets(line, sizeof(line), f)) {
		int pl, hl, sl, got;
		if (sscanf(line, "%d %511s %255s %511s %d %63s", &bits, pk, hh, sg, &want, why) != 6) continue;
		pl = unhex(pk, p, sizeof(p)); hl = unhex(hh, h, sizeof(h)); sl = unhex(sg, s, sizeof(s));
		got = tlsv_ecdsa_verify(bits, p, pl, h, hl, s, sl) == 0;
		if (got == want) pass++;
		else { fail++; printf("FAIL P-%d %s: %s\n", bits, why, got ? "accepted" : "refused"); }
	}
	printf("ecdsa: %d passed, %d failed\n", pass, fail);
	return fail != 0;
}
