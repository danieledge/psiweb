/* pwatom.h - (0.90) direct https through the PsiConnect Atom's own TLS.
 *
 * The Atom modem, with its standard AT$TLS=1, makes the TLS connection
 * itself when it is dialled on a TLS port (443 among them) and hands the
 * Psion the plain HTTP inside it. PsiWeb doing its own TLS on top of that
 * wrapped it twice, and every direct https page through the Atom failed.
 * The shared connection setting "Modem provides TLS" (PsiLink.ini
 * iModemTls, psion-core's psilink.h; PsiMail's Connection settings and its
 * setup assistant set it for the Atom) says the modem does the TLS: PsiWeb
 * then dials port 443 and speaks plain HTTP to the Atom, as PsiMail does
 * for IMAP and SMTP. The Atom checks the site's certificate against the
 * Mozilla roots in its firmware (src/cabundle.h) and its name, unless its
 * AT$TLSV is 0, so the padlock then says the Atom checked it, not PsiWeb
 * (page_secure PW_SECURE_MODEM).
 *
 * Only the modem route (on the Psion Internet route PsiWeb always does its
 * own TLS) and only port 443: the Atom's standard TLS ports are 443, 465,
 * 993 and 995, so an https address with a port of its own keeps PsiWeb's
 * own TLS. Through a web helper (the Atom's or PsiProxy) the helper does
 * the TLS and none of this applies. s is psiglue's PsiShared. */
#ifndef PWATOM_H
#define PWATOM_H

#define PW_ATOM_TLS_PORT 443

static __inline__ int pw_atom_tls(const PsiShared *s, int port)
{
	return s && !s->net_mode && s->modem_tls && port == PW_ATOM_TLS_PORT;
}

#endif
