/* problemtest.c - host test for engine/pwproblem.c: each failure finds its
 * kind, the page escapes the address, stays within its buffer and never
 * says "error" or "please".
 *   cc -Wall -Wextra -fsanitize=address,undefined -I engine -I core/ssh \
 *      test/problem/problemtest.c engine/pwproblem.c -o problemtest && ./problemtest
 */
#include <stdio.h>
#include <string.h>
#include "pwproblem.h"

static int gFail;
#define CHECK(c) do { if (!(c)) { printf("FAIL %d: %s\n", __LINE__, #c); gFail++; } } while (0)

int main(void)
{
	char page[PW_PROBLEM_MAX];
	PwRoute direct = { 1, 0, "" }, atom = { 0, 1, "psiproxy" }, pp = { 1, 1, "10.0.0.5" };
	int k;

	CHECK(pw_problem_kind("Host not found", "") == PW_PROB_NAME);
	CHECK(pw_problem_kind("Connection refused", NULL) == PW_PROB_REFUSED);
	CHECK(pw_problem_kind("Receive timeout", "") == PW_PROB_TIMEOUT);
	CHECK(pw_problem_kind("SSL error", "") == PW_PROB_TLS);
	CHECK(pw_problem_kind("Error reading from socket", "") == PW_PROB_BROKE);
	CHECK(pw_problem_kind("File not found", "") == PW_PROB_NOT_FOUND);
	CHECK(pw_problem_kind("Something new", "") == PW_PROB_OTHER);
	CHECK(pw_problem_kind("x", "the serial port is in use by another program") == PW_PROB_PORT_BUSY);
	CHECK(pw_problem_kind("x", "Certificate not trusted - www.x.org") == PW_PROB_CERT);
	CHECK(pw_problem_kind("x", "Clock not set - the certificate of www.x.org cannot be checked") == PW_PROB_CLOCK);   /* (0.98) */
	CHECK(pw_problem_kind("x", "Certificate cannot be checked - www.x.org") == PW_PROB_CERT);
	CHECK(pw_problem_kind("x", "its certificate has expired (is the Psion's clock right?)") == PW_PROB_CERT_DATE);
	CHECK(pw_problem_kind("x", "Secure connection refused - www.ebay.co.uk") == PW_PROB_TLS);
	CHECK(pw_problem_kind("x", "Could not look up www.x.org") == PW_PROB_NAME);
	CHECK(pw_problem_kind("x", "Could not connect: no CONNECT from the modem") == PW_PROB_NO_LINK);
	CHECK(pw_problem_kind("x", "Could not connect: looking up www.x.org failed") == PW_PROB_NAME);
	CHECK(pw_problem_kind("x", "name not found") == PW_PROB_NAME);
	CHECK(pw_problem_kind("x", "the name lookup (DNS) failed") == PW_PROB_NAME);
	CHECK(pw_problem_kind("x", "no name resolver - is TCP/IP installed?") == PW_PROB_NO_LINK);
	CHECK(pw_problem_kind("x", "the server refused the connection") == PW_PROB_REFUSED);
	CHECK(pw_problem_kind("x", "timed out") == PW_PROB_TIMEOUT);
	CHECK(pw_problem_kind("x", "Could not connect - no answer from the modem") == PW_PROB_NO_LINK);
	/* (0.90) the Atom modem's own TLS (engine/pwatom.h) did not come up: not "the modem", not "older security" */
	CHECK(pw_problem_kind("x", "Secure connection not made by the Atom modem - example.com") == PW_PROB_ATOM_TLS);

	for (k = 0; k < PW_PROB_COUNT; k++) {
		const PwRoute *r = k % 3 == 0 ? &direct : k % 3 == 1 ? &atom : &pp;
		int n = pw_problem_html(page, sizeof(page), "https://www.example.com/a?b=<c>&d=\"e\"", k, "why <it> failed", r);
		CHECK(n > 0 && n < (int)sizeof(page) && (int)strlen(page) == n);
		CHECK(strstr(page, "Page not opened"));
		CHECK(strstr(page, "about:cmd-test") && strstr(page, "about:cmd-setup"));
		CHECK(!strstr(page, "<c>") && !strstr(page, "<it>"));
		CHECK(!strstr(page, "rror") && !strstr(page, "lease"));
		CHECK(strstr(page, "Route: "));
	}
	pw_problem_html(page, sizeof(page), "https://www.ebay.co.uk/", PW_PROB_TLS, "", &direct);
	CHECK(strstr(page, "www.ebay.co.uk") && strstr(page, "PsiProxy on a computer") && strstr(page, "Route: Direct, on the network card"));
	pw_problem_html(page, sizeof(page), "https://example.com/", PW_PROB_ATOM_TLS, "", &direct);
	CHECK(strstr(page, "example.com") && strstr(page, "Atom modem could not make a secure connection") && !strstr(page, "older security"));
	pw_problem_html(page, sizeof(page), "http://x.org/", PW_PROB_REFUSED, "", &atom);
	CHECK(strstr(page, "Route: Atom modem helper, through the modem"));
	pw_problem_html(page, sizeof(page), "http://x.org/", PW_PROB_REFUSED, "", &pp);
	CHECK(strstr(page, "Route: PsiProxy on a computer"));

	/* a very long address: cut, the page still whole and bounded */
	{
		char url[3000];
		memset(url, 'a', sizeof(url) - 1);
		memcpy(url, "http://", 7);
		url[sizeof(url) - 1] = 0;
		int n = pw_problem_html(page, sizeof(page), url, PW_PROB_OTHER, "", &direct);
		CHECK(n > 0 && n < (int)sizeof(page) && strstr(page, "</html>"));
	}
	/* too small a buffer: nothing written past it */
	{
		char small[600];
		memset(small, 'z', sizeof(small));
		pw_problem_html(small, 300, "http://x/", 0, "", &direct);
		CHECK(small[0] == 0 && small[300] == 'z');
	}
	/* (0.89) the short causes, for Test the connection: one line each, in
	   the style's words, the helper's refusal said as such */
	for (k = 0; k < PW_PROB_COUNT; k++) {
		const char *t = pw_problem_short(k, &direct);
		CHECK(t && t[0] && strlen(t) < PW_PROBLEM_SHORT_MAX);
		CHECK(strlen(pw_problem_short(k, &pp)) < PW_PROBLEM_SHORT_MAX);
		CHECK(!strstr(t, "error") && !strstr(t, "rror") && !strstr(t, "please") && !strstr(t, "!"));
		CHECK(t[strlen(t) - 1] != '.');
	}
	CHECK(strstr(pw_problem_short(PW_PROB_REFUSED, &pp), "web helper") != NULL);
	CHECK(strstr(pw_problem_short(PW_PROB_REFUSED, &direct), "web helper") == NULL);
	CHECK(strstr(pw_problem_short(PW_PROB_REFUSED, &atom), "web helper") != NULL);
	CHECK(pw_problem_short(-1, NULL) != NULL);
	printf(gFail ? "problemtest: %d failed\n" : "problemtest: all passed\n", gFail);
	return gFail != 0;
}
