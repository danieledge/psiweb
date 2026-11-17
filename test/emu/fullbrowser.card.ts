// fullbrowser.card.ts - a card with PsiWeb and PsiTerm (0.88), for File >
// Open in full browser handing a page to PsiTerm:
//   node --experimental-strip-types core/tools/emu/mkcard.ts fullbrowser
// PsiTerm's files come from its checkout beside this one (pkg/, after its
// build.sh).
export const cards = {
	fullbrowser: (c) => {
		const W = c.pkg('build/web-pkg/');
		for (const n of ['psiweb.app', 'psiweb.rsc', 'psiweb.exe', 'psiweb.aif', 'psiweb.mbm'])
			c.w('SYSTEM/APPS/PSIWEB/' + n.toUpperCase(), c.f(W + n));
		const T = c.REPO + '../psiterm/pkg/';
		for (const n of ['psiterm.app', 'psiterm.rsc', 'psissh.exe', 'psiterm.aif', 'psiterm.gdr', 'psiterm.mbm'])
			c.w('SYSTEM/APPS/PSITERM/' + n.toUpperCase(), c.f(T + n));
	},
};
