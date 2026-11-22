/* pwproblem.c - the "Page not opened" page (pwproblem.h). */
#include <string.h>
#include "pwproblem.h"
#define PG_SU_HOST                 /* pgsetup.h's plain C only */
#include "pgsetup.h"

static int starts(const char *s, const char *p)
{
	while (*p) {
		int a = *s, b = *p;
		if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
		if (b >= 'A' && b <= 'Z') b += 'a' - 'A';
		if (a != b) return 0;
		s++, p++;
	}
	return 1;
}

static int has(const char *s, const char *p)
{
	for (; *s; s++)
		if (starts(s, p)) return 1;
	return 0;
}

int pw_problem_kind(const char *aEngineWhy, const char *aConnWhy)
{
	const char *c = aConnWhy ? aConnWhy : "";
	const char *e = aEngineWhy ? aEngineWhy : "";
	if (*c) {
		/* (1.03) psiglue's hold after a failed connection: this one was not
		   tried at all, so it says nothing about the cable or the network
		   (it read "There is no connection - the network card's cable may be
		   out" while the network was up) */
		if (has(c, "failed a moment ago") || has(c, "busy with the last attempt")
		    || has(c, "connection is busy - try again"))
			return PW_PROB_RETRY;
		if (starts(c, "Clock not set")) return PW_PROB_CLOCK;     /* (0.98) pwcert.c */
		if (has(c, "clock") || has(c, "out of date") || has(c, "expired") || has(c, "not valid yet"))
			return PW_PROB_CERT_DATE;
		if (starts(c, "Certificate")) return PW_PROB_CERT;
		if (starts(c, "Secure connection refused")) return PW_PROB_TLS;
		if (starts(c, "Secure connection not made by the Atom")) return PW_PROB_ATOM_TLS;
		if (has(c, "serial port") && (has(c, "in use") || has(c, "has the")))
			return PW_PROB_PORT_BUSY;
		if (has(c, "modem") || has(c, "dial"))
			return PW_PROB_NO_LINK;            /* (the modem or the dial-up, not the site) */
		if (has(c, "name resolver"))
			return PW_PROB_NO_LINK;            /* (no TCP/IP at all) */
		if (has(c, "look up") || has(c, "looking up") || has(c, "not found") || has(c, "no such name")
			|| has(c, "name server") || has(c, "dns") || has(c, "not known"))
			return PW_PROB_NAME;
		if (has(c, "refused")) return PW_PROB_REFUSED;
		if (has(c, "timed out") || has(c, "did not answer") || has(c, "no answer"))
			return PW_PROB_TIMEOUT;
		return PW_PROB_NO_LINK;
	}
	if (starts(e, "Host not found")) return PW_PROB_NAME;
	if (starts(e, "Connection refused")) return PW_PROB_REFUSED;
	if (starts(e, "Receive timeout")) return PW_PROB_TIMEOUT;
	if (starts(e, "SSL error")) return PW_PROB_TLS;
	if (starts(e, "Error writing to socket") || starts(e, "Error reading from socket")) return PW_PROB_BROKE;
	if (starts(e, "Bad HTTP response") || starts(e, "Server returned empty response")) return PW_PROB_BAD_ANSWER;
	if (starts(e, "File not found")) return PW_PROB_NOT_FOUND;
	return PW_PROB_OTHER;
}

/* (1.03) a web helper on a computer: PsiProxy (not the Atom modem's own) */
static int computer_helper(const PwRoute *r)
{
	return r && r->use_proxy && r->proxy_host && r->proxy_host[0] && !pg_su_is_atom_host(r->proxy_host);
}

int pw_problem_for_route(int aKind, const char *aConnWhy, const PwRoute *aRoute)
{
	const char *c = aConnWhy ? aConnWhy : "";
	if (!computer_helper(aRoute))
		return aKind;
	if (aKind != PW_PROB_NO_LINK && aKind != PW_PROB_REFUSED && aKind != PW_PROB_TIMEOUT && aKind != PW_PROB_NAME)
		return aKind;
	/* the link itself: the dial-up, the modem, the serial port, TCP/IP */
	if (has(c, "dial") || has(c, "modem") || has(c, "serial port") || has(c, "name resolver")
	    || has(c, "internet service") || has(c, "internet account") || has(c, "login"))
		return aKind;
	return PW_PROB_HELPER;
}

typedef struct { char *p; int left; } Out;

static void put(Out *o, const char *s)
{
	while (*s && o->left > 1) { *o->p++ = *s++; o->left--; }
	*o->p = 0;
}

/* the caller's text, escaped for HTML; at most aMaxChars of it */
static void put_esc(Out *o, const char *s, int aMaxChars)
{
	int n = 0;
	for (; *s && n < aMaxChars; s++, n++) {
		char one[2] = { *s, 0 };
		if (*s == '<') put(o, "&lt;");
		else if (*s == '>') put(o, "&gt;");
		else if (*s == '&') put(o, "&amp;");
		else if (*s == '"') put(o, "&quot;");
		else if ((unsigned char)*s < ' ') put(o, " ");
		else put(o, one);
	}
	if (*s) put(o, "...");
}

/* (1.03) the caller's text as it is (for a window line, not a page), at most aMaxChars */
static void put_esc_plain(Out *o, const char *s, int aMaxChars)
{
	int n = 0;
	for (; *s && n < aMaxChars; s++, n++) {
		char one[2] = { *s, 0 };
		put(o, (unsigned char)*s < ' ' ? " " : one);
	}
	if (*s) put(o, "...");
}

/* the host of aUrl ("www.bbc.co.uk"), into aHost */
static void host_of(const char *aUrl, char *aHost, int aMax)
{
	const char *s = strstr(aUrl, "://");
	int n = 0;
	s = s ? s + 3 : aUrl;
	while (*s && *s != '/' && *s != ':' && *s != '?' && *s != '#' && n < aMax - 1)
		aHost[n++] = *s++;
	aHost[n] = 0;
}

static const char *cause(int aKind, const PwRoute *r)
{
	int helper = r && r->use_proxy && r->proxy_host && r->proxy_host[0];
	switch (aKind) {
	case PW_PROB_PORT_BUSY:
		return "Another program has the serial port - the Remote link may be on (System screen, Ctrl+L), or PsiTerm or PsiMail may still be connected";
	case PW_PROB_NO_LINK:
		return r && r->net_mode
			? "There is no connection - the network card's cable may be out, the netBook may have no address from the network yet, or the dial-up did not start"
			: "There is no connection - the modem may be off or not on the cable, or it has no WiFi";
	case PW_PROB_NAME:
		return "The name was not found - the address may be mistyped, or the connection has no name servers";
	case PW_PROB_REFUSED:
		return helper ? "The connection was refused - the web helper may not be running" : "The server refused the connection";
	case PW_PROB_TIMEOUT:
		return "The server did not answer in time - the connection may be slow or down";
	case PW_PROB_TLS:
		return helper ? "The secure connection was refused"
		              : "The site needs older security than PsiWeb has - it may open through PsiProxy on a computer";
	case PW_PROB_CERT:
		return "The site's certificate is not trusted - it may not be the site it says it is, or PsiWeb does not know who signed it";
	case PW_PROB_CERT_DATE:
		return "The site's certificate is out of date for the Psion's clock - the Psion's date may be wrong (Time, Ctrl+T)";
	case PW_PROB_BROKE:
		return "The connection broke part way - the line may have dropped";
	case PW_PROB_BAD_ANSWER:
		return "The server's answer made no sense - the site may be down, or need a full browser";
	case PW_PROB_NOT_FOUND:
		return "The file was not found on the Psion";
	case PW_PROB_ATOM_TLS:
		return "The Atom modem could not make a secure connection to the site - the site's certificate may not be trusted, the site may be down, or the Atom may have no WiFi (the Atom's log says which)";
	case PW_PROB_CLOCK:
		return "The Psion's clock is not set, so no site's certificate can be checked - set the date first (Time, Ctrl+T)";
	case PW_PROB_RETRY:
		return "The connection before this one failed a moment ago, so this page could not be tried - Try again in a few seconds";
	case PW_PROB_HELPER:	/* (its address: pw_problem_html) */
		return "The web helper did not answer - PsiProxy may not be running on that computer, or the computer may be off or off the network";
	default:
		return "The page did not load";
	}
}

/* (0.89) The same causes in a line of the Test the connection window
   (PW_PROBLEM_SHORT_MAX with its 0) */
const char *pw_problem_short(int aKind, const PwRoute *r)
{
	int helper = r && r->use_proxy && r->proxy_host && r->proxy_host[0];
	switch (aKind) {
	case PW_PROB_PORT_BUSY:  return "Another program has the serial port";
	case PW_PROB_NO_LINK:    return "There is no connection";
	case PW_PROB_NAME:       return "The name was not found - the connection may have no name servers";
	case PW_PROB_REFUSED:    return helper ? "Refused - the web helper may not be running" : "The server refused the connection";
	case PW_PROB_TIMEOUT:    return "No answer in time - the connection may be slow or down";
	case PW_PROB_TLS:        return "The secure connection was refused";
	case PW_PROB_CERT:       return "The site's certificate was not trusted";
	case PW_PROB_CERT_DATE:  return "The certificate's dates and the Psion's clock disagree (Time, Ctrl+T)";
	case PW_PROB_BROKE:      return "The connection broke part way";
	case PW_PROB_BAD_ANSWER: return "The answer made no sense - the site may be down";
	case PW_PROB_NOT_FOUND:  return "The file was not found on the Psion";
	case PW_PROB_ATOM_TLS:   return "The Atom modem did not make the secure connection";
	case PW_PROB_CLOCK:      return "The Psion's clock is not set - set the date first (Time, Ctrl+T)";
	case PW_PROB_HELPER:     return "The web helper did not answer - PsiProxy may not be running";
	case PW_PROB_RETRY:      return "A connection failed a moment ago - try again in a few seconds";
	default:                 return "The page did not come";
	}
}

void pw_problem_short_line(char *aOut, int aMax, int aKind, const PwRoute *aRoute)
{
	Out o;
	char port[12];
	int n = 0, v;
	if (!aOut || aMax <= 0) return;
	o.p = aOut;
	o.left = aMax;
	aOut[0] = 0;
	if (aKind != PW_PROB_HELPER || !computer_helper(aRoute)) {
		put(&o, pw_problem_short(aKind, aRoute));
		return;
	}
	put(&o, "The web helper at ");
	put_esc_plain(&o, aRoute->proxy_host, 40);
	v = aRoute->proxy_port > 0 ? aRoute->proxy_port : 8080;
	port[11] = 0;
	n = 11;
	do { port[--n] = (char)('0' + v % 10); v /= 10; } while (v && n > 0);
	put(&o, ":");
	put(&o, port + n);
	put(&o, " did not answer");
}

int pw_problem_html(char *aOut, int aMax, const char *aUrl, int aKind, const char *aConnWhy, const PwRoute *aRoute)
{
	Out o;
	char host[96];
	const char *url = aUrl ? aUrl : "";
	if (!aOut || aMax < 512) {
		if (aOut && aMax > 0) aOut[0] = 0;
		return 0;
	}
	o.p = aOut;
	o.left = aMax;
	aOut[0] = 0;
	host_of(url, host, sizeof(host));
	put(&o, "<html><head><title>Page not opened</title></head><body>\n<h2>Page not opened</h2>\n<p>");
	if (aKind == PW_PROB_NOT_FOUND || !host[0]) {
		put(&o, "PsiWeb could not open <b>");
		put_esc(&o, url, 120);
	} else {
		put(&o, "PsiWeb could not reach <b>");
		put_esc(&o, host, 90);
	}
	put(&o, "</b></p>\n<p>");
	if (aKind == PW_PROB_HELPER && computer_helper(aRoute)) {
		/* (1.03) which helper: "The web helper at 192.0.2.7:8081 did not answer - ..." */
		char line[PW_PROBLEM_SHORT_MAX + 64];
		pw_problem_short_line(line, sizeof(line), aKind, aRoute);
		put_esc(&o, line, sizeof(line));
		put(&o, " - PsiProxy may not be running on that computer, or the computer may be off or off the network");
	} else
		put(&o, cause(aKind, aRoute));
	put(&o, "</p>\n");
	if (aKind == PW_PROB_OTHER && aConnWhy && aConnWhy[0]) {
		put(&o, "<p>");
		put_esc(&o, aConnWhy, 120);
		put(&o, "</p>\n");
	}
	put(&o, "<p><a href=\"");
	put_esc(&o, url, 480);
	put(&o, "\">Try again</a> &nbsp; &nbsp; ");
	if (aKind == PW_PROB_HELPER && computer_helper(aRoute))
		put(&o, "<a href=\"about:cmd-direct\">Open directly this time</a> &nbsp; &nbsp; ");	/* (1.03) without the helper, this page */
	put(&o, "<a href=\"about:cmd-test\">Test the connection</a> &nbsp; &nbsp; <a href=\"about:cmd-setup\">Set up PsiWeb</a></p>\n");
	put(&o, "<p><small>Route: ");
	if (aRoute) {
		int helper = aRoute->use_proxy && aRoute->proxy_host && aRoute->proxy_host[0];
		put(&o, pg_su_route_name(helper, helper ? aRoute->proxy_host : ""));
		put(&o, aRoute->net_mode ? ", on the network card or dial-up" : ", through the modem");
	} else
		put(&o, "-");
	put(&o, "</small></p>\n</body></html>\n");
	return (int)(o.p - aOut);
}
