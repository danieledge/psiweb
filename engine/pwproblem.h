/* pwproblem.h - the "Page not opened" page (0.85): what happened, the
 * likely causes in plain words, and what to do, in place of a two-second
 * infoprint. psi_drv.c builds it when a page does not load and shows it as
 * about:problem; its links are Try again (the address again), Test the
 * connection (about:cmd-test) and Set up PsiWeb (about:cmd-setup), which
 * the engine hands to PsiWeb.app (psiweb.h app_cmd). Plain C with bounded
 * writes, so test/problem/problemtest.c checks it on a PC. */
#ifndef PWPROBLEM_H
#define PWPROBLEM_H

/* the kinds of failure, each with its own text */
enum
	{
	PW_PROB_OTHER = 0,
	PW_PROB_PORT_BUSY,      /* the serial port is in use (the Remote link, another program) */
	PW_PROB_NO_LINK,        /* no connection: the modem, the network card, the dial-up */
	PW_PROB_NAME,           /* the name was not found */
	PW_PROB_REFUSED,        /* the server refused the connection */
	PW_PROB_TIMEOUT,        /* no answer in time */
	PW_PROB_TLS,            /* the secure connection was refused (older security, another cipher) */
	PW_PROB_CERT,           /* the certificate is not trusted */
	PW_PROB_CERT_DATE,      /* the certificate's dates and the Psion's clock disagree */
	PW_PROB_BROKE,          /* the connection broke part way */
	PW_PROB_BAD_ANSWER,     /* the server's answer made no sense, or was empty */
	PW_PROB_NOT_FOUND,      /* a file on the Psion was not found */
	PW_PROB_COUNT
	};

/* aEngineWhy: Links' words for the failure ("Host not found"...);
 * aConnWhy: what the connection layer said, if anything (psiglue's words,
 * pwcert's "Certificate not trusted - host"...). Returns PW_PROB_*. */
int pw_problem_kind(const char *aEngineWhy, const char *aConnWhy);

/* The route, for the page's last line and its advice */
typedef struct
	{
	int net_mode;           /* 1 network card or dial-up, 0 modem */
	int use_proxy;          /* a web helper is set */
	const char *proxy_host; /* its address ("psiproxy": the Atom modem helper) */
	} PwRoute;

/* Writes the page (HTML, UTF-8 safe: only the address is the caller's, and
 * it is escaped) into aOut, at most aMax bytes with its 0. aUrl is the
 * address that did not open. Returns the length (0 if aMax is too small
 * for even the page's frame). */
int pw_problem_html(char *aOut, int aMax, const char *aUrl, int aKind, const char *aConnWhy, const PwRoute *aRoute);

/* The engine side (psi_drv.c): the page is kept here until shown */
#define PW_PROBLEM_MAX 3072
#endif
