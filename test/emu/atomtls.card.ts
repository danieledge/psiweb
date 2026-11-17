// atomtls.card.ts - a card with PsiMail and PsiWeb (0.90), for direct https
// through the Atom modem's own TLS (test/emu/atomtls.net.py): PsiMail's
// setup assistant sets "Modem provides TLS" for the Atom, PsiWeb reads it.
// PsiMail's files come from its checkout beside this one (build/mail-pkg/,
// after its build.sh).
export const cards = {
	atomtls: (c) => {
		const M = c.REPO + '../psimail/build/mail-pkg/';
		for (const n of ['psimail.app', 'psimail.rsc', 'psimail.exe', 'psimail.aif', 'psimail.mbm', 'pmbutton.exe'])
			c.w('SYSTEM/APPS/PSIMAIL/' + n.toUpperCase(), c.f(M + n));
		const W = c.pkg('build/web-pkg/');
		for (const n of ['psiweb.app', 'psiweb.rsc', 'psiweb.exe', 'psiweb.aif', 'psiweb.mbm'])
			c.w('SYSTEM/APPS/PSIWEB/' + n.toUpperCase(), c.f(W + n));
	},
};
