/* htmltest.c - host test for app/pwhtml.h (0.98): the start page's link
 * addresses go in whole, made safe, or not at all; never cut, never with
 * "..." on the end.
 *   cc -Wall -Wextra -fsanitize=address,undefined -I app test/start/htmltest.c -o htmltest && ./htmltest
 */
#include <stdio.h>
#include <string.h>
#include "pwhtml.h"

static int gFail;
static void Esc(const char* aIn, int aRoom, const char* aWant)
	{
	unsigned char out[1200];
	memset(out, 0x55, sizeof(out));
	int r = pw_attr_esc(out, aRoom, (const unsigned char*)aIn, (int)strlen(aIn));
	if (!aWant)
		{
		if (r != -1 || out[0] != 0x55) { printf("FAIL \"%.40s\" room %d: want refused, got %d\n", aIn, aRoom, r); gFail++; }
		return;
		}
	if (r != (int)strlen(aWant) || memcmp(out, aWant, r) || out[r] != 0x55)
		{ printf("FAIL \"%.40s\" room %d: got %d \"%.*s\", want \"%s\"\n", aIn, aRoom, r, r < 0 ? 0 : r, out, aWant); gFail++; }
	}

int main(void)
	{
	Esc("https://x.org/", 100, "https://x.org/");
	Esc("https://x.org/?a=1&b=\"2\"<>", 100, "https://x.org/?a=1&amp;b=&quot;2&quot;&lt;&gt;");
	Esc("a\tb", 10, "a b");
	Esc("abc", 3, "abc");          /* exactly fits */
	Esc("abc", 2, NULL);           /* too long: nothing, not "ab" */
	Esc("a&", 5, NULL);            /* the entity would not fit */
	Esc("", 0, "");
	Esc("x", -1, NULL);
	/* a favourite of 400 characters (more than the old 300): whole, no 0x85 */
	char longu[401];
	memset(longu, 'u', 400);
	memcpy(longu, "https://e.org/", 14);
	longu[400] = 0;
	Esc(longu, 600, longu);
	Esc(longu, 399, NULL);
	printf(gFail ? "%d FAILED\n" : "all passed\n", gFail);
	return gFail != 0;
	}
