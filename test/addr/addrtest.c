/* addrtest.c - host test for app/pwaddr.h: what is an address and what is
 * words to search for, and the search address made from the words.
 *   cc -Wall -Wextra -fsanitize=address,undefined -I app test/addr/addrtest.c -o addrtest && ./addrtest
 */
#include <stdio.h>
#include <string.h>
#include "pwaddr.h"

static int gFail;
static void Is(const char* aIn, int aSearch)
	{
	int r = pw_addr_is_search((const unsigned char*)aIn, (int)strlen(aIn));
	if (r != aSearch) { printf("FAIL is_search(\"%s\") = %d\n", aIn, r); gFail++; }
	}
static void Make(const char* aIn, int aEngine, const char* aWant)
	{
	char out[120];
	int r = pw_addr_make((const unsigned char*)aIn, (int)strlen(aIn), aEngine, out, sizeof(out));
	if (r < 0 || strcmp(out, aWant)) { printf("FAIL make(\"%s\") = \"%s\" (%d), want \"%s\"\n", aIn, r < 0 ? "" : out, r, aWant); gFail++; }
	}

int main(void)
	{
	Is("bbc.co.uk", 0);
	Is("https://www.bbc.co.uk/news", 0);
	Is("http://10.0.0.5:8081/", 0);
	Is("10.0.0.5:8081", 0);
	Is("about:welcome", 0);
	Is("mailto:a@b.c", 0);
	Is("localhost", 0);
	Is("/index.html", 0);
	Is("psion", 1);
	Is("psion series 5mx", 1);
	Is("what is 2.5 kg", 1);
	Is("", 0);
	Make("psion series 5mx", 0, "https://html.duckduckgo.com/html/?q=psion+series+5mx");
	Make("  psion  ", 1, "http://frogfind.com/?q=psion");
	Make("bbc.co.uk", 0, "bbc.co.uk");
	Make("  https://x.org/  ", 0, "https://x.org/");
	Make("caf\xe9 & bar", 0, "https://html.duckduckgo.com/html/?q=caf%C3%A9+%26+bar");
	Make("\x80 euro", 0, "https://html.duckduckgo.com/html/?q=%E2%82%AC+euro");
	Make("a+b=c?", 0, "https://html.duckduckgo.com/html/?q=a%2Bb%3Dc%3F");
	/* too long: refused, never cut */
	char longw[200];
	memset(longw, 'x', sizeof(longw) - 1);
	longw[50] = ' ';
	longw[sizeof(longw) - 1] = 0;
	char out[64];
	if (pw_addr_make((const unsigned char*)longw, (int)strlen(longw), 0, out, sizeof(out)) != -1) { printf("FAIL long\n"); gFail++; }
	if (pw_addr_make((const unsigned char*)"a b", 3, 0, out, 0) != -1) { printf("FAIL zero room\n"); gFail++; }
	/* (0.88) the full browser's command: one word for the shell */
	{
		char c[101];
		const char *u = "https://x.org/a'b;rm -rf ~$(id)`x`\n";
		int r = pw_shell_command("carbonyl", (const unsigned char *)u, (int)strlen(u), c, sizeof(c));
		if (r < 0 || strcmp(c, "carbonyl 'https://x.org/a'\\''b;rm -rf ~$(id)`x`'")) { printf("FAIL shell \"%s\"\n", c); gFail++; }
		char lng[300];
		memset(lng, 'a', sizeof(lng) - 1); lng[sizeof(lng) - 1] = 0;
		if (pw_shell_command("w3m", (const unsigned char *)lng, 299, c, sizeof(c)) != -1) { printf("FAIL shell long\n"); gFail++; }
		if (pw_shell_command("w3m", (const unsigned char *)"\'\'\'\'\'\'\'\'\'\'\'\'\'\'\'\'\'\'\'\'\'\'\'\'", 24, c, 40) != -1) { printf("FAIL shell quotes\n"); gFail++; }
	}
	/* (0.89) zoom per machine (pw_zoom_load, pw_zoom_save) */
	{
		int z, o, b1, b2;
		/* a file older than 0.89: its one byte is the zoom of the machine that wrote it */
		pw_zoom_load(1, 125, -1, &z, &o); if (z != 125 || o != 0) { printf("FAIL zoom old tall %d %d\n", z, o); gFail++; }
		pw_zoom_load(0, 80, -1, &z, &o); if (z != 80 || o != 0) { printf("FAIL zoom old short %d %d\n", z, o); gFail++; }
		pw_zoom_load(0, 0, -1, &z, &o); if (z != 100) { printf("FAIL zoom none %d\n", z); gFail++; }
		/* the netBook saves 150; the file goes to a 5mx: it keeps its own 80 */
		pw_zoom_save(1, 150, 80, &b1, &b2); if (b1 != 80 || b2 != 150) { printf("FAIL zoom save tall %d %d\n", b1, b2); gFail++; }
		pw_zoom_load(0, b1, b2, &z, &o); if (z != 80 || o != 150) { printf("FAIL zoom 5mx %d %d\n", z, o); gFail++; }
		/* the 5mx changes to 90 and saves; back on the netBook: 150 still */
		pw_zoom_save(0, 90, o, &b1, &b2); if (b1 != 90 || b2 != 150) { printf("FAIL zoom save short %d %d\n", b1, b2); gFail++; }
		pw_zoom_load(1, b1, b2, &z, &o); if (z != 150 || o != 90) { printf("FAIL zoom netBook %d %d\n", z, o); gFail++; }
		/* a netBook that never chose a zoom (0) gets 100, not the 5mx's */
		pw_zoom_save(0, 70, 0, &b1, &b2); if (b1 != 70 || b2 != 0) { printf("FAIL zoom save none %d %d\n", b1, b2); gFail++; }
		pw_zoom_load(1, b1, b2, &z, &o); if (z != 100 || o != 70) { printf("FAIL zoom tall none %d %d\n", z, o); gFail++; }
		/* a new netBook (no other zoom) writes 100 for the 5mx */
		pw_zoom_save(1, 110, 0, &b1, &b2); if (b1 != 100 || b2 != 110) { printf("FAIL zoom new tall %d %d\n", b1, b2); gFail++; }
	}
	printf(gFail ? "addrtest: %d failed\n" : "addrtest: all passed\n", gFail);
	return gFail != 0;
	}
