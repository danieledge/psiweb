/* pwcert.c - PsiWeb's side of the certificate check
 *
 * From 0.84 PsiWeb's own HTTPS checks who it is talking to, as PsiMail
 * does: tls13.c is built with TLS_VERIFY (tls/includes.h) and psion-core's
 * certificate check (core/mail/engine/tlsv/certcheck.c: RSA in pmrsa.c with
 * libtommath, ECDSA on P-256 and P-384 in ecverify.h, the built-in roots in
 * roots.h) runs in the handshake:
 *
 *  - the server's certificate chain must reach one of the built-in roots,
 *    every certificate on the way in date, the first one for the host
 *    asked for (its subjectAltName);
 *  - the server must prove it holds that certificate's key (its
 *    CertificateVerify signature).
 *
 * Anything else ends the connection before a byte of the request is sent,
 * and the page is not loaded: "Certificate not trusted - <host>". There is
 * no way round it: PsiMail lets its user trust one server's key (pins.txt),
 * PsiWeb keeps no such list (st_pin_check below always says no). A page
 * that cannot be checked can still be read through a proxy, which does the
 * TLS itself (Connection settings > Use proxy).
 *
 * This file gives certcheck.c what it asks of the program (calport.h): the
 * folder for certs.txt (intermediate certificates already checked, so the
 * slow work is done once), the time, progress on the status line.
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "calport.h"
#include "pwcert.h"

extern void pwb_set_status(const char *utf8);
extern void pw_log(const char *text);
extern long pw_utc(void);                  /* seconds since 1970, UTC: pwepoc.cpp (emu: links_rt.c) */
extern int mkdir(const char *path, int mode);
extern void tlsv_set_host(const char *host, int port);
extern void tlsv_start(void);

/* ---- what certcheck.c asks for */

const char *calp_dir(void)
{
#ifdef __EPOC32__
	return "C:\\System\\Apps\\PsiWeb\\";
#else
	return "./";
#endif
}

long pm_time(void) { return pw_utc(); }

int pm_mkdir(const char *path)
{
	char p[160];
	int n;
	pm_copy(p, path, sizeof(p));
	n = (int)strlen(p);
	if (n > 3 && (p[n - 1] == '\\' || p[n - 1] == '/'))
		p[n - 1] = 0;
	mkdir(p, 0755);
	return 0;
}

void pm_progress(const char *fmt, ...)
{
	char line[100];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(line, sizeof(line), fmt, ap);
	va_end(ap);
	pwb_set_status(line);
}

/* no exceptions: a certificate that cannot be checked is never trusted */
int st_pin_check(const char *hostport, const char *fp)
{
	(void)hostport; (void)fp;
	return 0;
}

void pm_copy(char *dst, const char *src, int max)
{
	int n;
	if (max <= 0) return;
	n = src ? (int)strlen(src) : 0;
	if (n > max - 1) n = max - 1;
	if (n) memcpy(dst, src, n);
	dst[n] = 0;
}

static int lower(int c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }

int pm_strcasecmp(const char *a, const char *b)
{
	while (*a && lower((unsigned char)*a) == lower((unsigned char)*b)) { a++; b++; }
	return lower((unsigned char)*a) - lower((unsigned char)*b);
}

/* ---- PsiWeb's own */

void pw_cert_host(const char *host, int port)
{
	tlsv_start();              /* (no reason left from the last connection) */
	tlsv_set_host(host, port);
}

/* tls13.c's and certcheck.c's words for a certificate that failed; the
   first group: checked and refused; the second: of a kind PsiWeb cannot
   check (a key or signature of another kind: Ed25519, P-521...) */
static const char *const k_untrusted[] = {
	"the server's certificate is not trusted",
	"the server's signature is wrong",
	"the server did not prove who it is",
	"bad certificate message",
	"could not read the server's certificate",
	"the server sent no certificate",
	"CertificateVerify without a certificate",
	"bad CertificateVerify",
	"unexpected certificate in a resumed session",
};

int pw_cert_why(const char *host, char *why, int whymax)
{
	const char *problem = tlsv_problem();
	char line[160];
	int untrusted = problem && problem[0], cannot = 0;
	unsigned i;
	if (!why || whymax <= 0)
		return 0;
	/* (0.98) with the clock reset no chain is trusted (certcheck.c): say
	   that, not that the site is not trusted (pwproblem.c PW_PROB_CLOCK) */
	if (problem && strstr(problem, "clock is not set")) {
		snprintf(line, sizeof(line), "certificate of %.80s: %.60s - %.70s", host, why, problem);
		pw_log(line);
		snprintf(why, whymax, "Clock not set - the certificate of %s cannot be checked", host);
		return 1;
	}
	for (i = 0; !untrusted && i < sizeof(k_untrusted) / sizeof(k_untrusted[0]); i++)
		if (!strcmp(why, k_untrusted[i])) untrusted = 1;
	if (!untrusted && (strstr(why, "key is of a kind") || strstr(why, "used a signature")))
		cannot = 1;
	if (!untrusted && !cannot) {
		/* a server that turns PsiWeb's handshake down (it offers RSA and
		   ECDSA on P-256 and P-384 only, and one cipher): say so plainly */
		if (!strcmp(why, "server refused the connection (TLS alert)")) {
			snprintf(why, whymax, "Secure connection refused - %s", host);
			return 1;
		}
		return 0;
	}
	snprintf(line, sizeof(line), "certificate of %.80s: %.60s%s%.70s", host, why,
		problem && problem[0] ? " - " : "", problem ? problem : "");
	pw_log(line);
	snprintf(why, whymax, "%s - %s", untrusted ? "Certificate not trusted" : "Certificate cannot be checked", host);
	return 1;
}
