// atomtls.card.ts - a card with PsiWeb (0.91), for direct https through the
// Atom modem's own TLS (test/emu/atomtls.net.py): PsiWeb's own setup
// assistant sets "Modem provides TLS" for the Atom, and PsiWeb reads it.
export const cards = {
	atomtls: (c) => {
		const W = c.pkg('build/web-pkg/');
		for (const n of ['psiweb.app', 'psiweb.rsc', 'psiweb.exe', 'psiweb.aif', 'psiweb.mbm'])
			c.w('SYSTEM/APPS/PSIWEB/' + n.toUpperCase(), c.f(W + n));
	},
};
