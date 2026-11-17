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
	default:                 return "The page did not come";
	}
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
	put(&o, cause(aKind, aRoute));
	put(&o, "</p>\n");
	if (aKind == PW_PROB_OTHER && aConnWhy && aConnWhy[0]) {
		put(&o, "<p>");
		put_esc(&o, aConnWhy, 120);
		put(&o, "</p>\n");
	}
	put(&o, "<p><a href=\"");
	put_esc(&o, url, 480);
	put(&o, "\">Try again</a> &nbsp; &nbsp; <a href=\"about:cmd-test\">Test the connection</a> &nbsp; &nbsp; <a href=\"about:cmd-setup\">Set up PsiWeb</a></p>\n");
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
