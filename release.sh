#!/bin/bash
# Signs a PsiWeb build and writes the files the updater reads.
#   bash release.sh <version>        (after bash build.sh; e.g. 0.70)
#
# Installed PsiWebs fetch dist/PsiWeb-version.txt, dist/PsiWeb.sis and
# dist/PsiWeb.sis.sig from the psiterm repository (its branch's dist/),
# as they did before the repositories were split; the updater's address in
# the code has not changed. So this writes the three files here, and, when a
# psiterm checkout is beside this one ($PSITERM, default ../psiterm), copies
# them into its dist/ together, to be committed and pushed there.
#
# The key: --key, $PSITERM_RELEASE_KEY or ~/.psiterm-signing/release.key
# (core/tools/release/sign.py); it is never in a repository.
set -e
VERSION=${1:?version, e.g. 0.70}
HERE=$(cd "$(dirname "$0")" && pwd)
CORE=${PSION_CORE:-$HERE/core}
SIS=$HERE/dist/PsiWeb.sis
[ -f "$SIS" ] || { echo "no $SIS: build first (bash build.sh)"; exit 1; }
grep -q "0,${VERSION#*.},0" "$HERE/pkg/psiweb.pkg" || echo "warning: pkg/psiweb.pkg does not say version $VERSION"
python3 "$CORE/tools/release/sign.py" --product PsiWeb "$SIS" "$VERSION"
echo "$VERSION" > "$HERE/dist/PsiWeb-version.txt"
echo "signed: dist/PsiWeb.sis, dist/PsiWeb.sis.sig, dist/PsiWeb-version.txt ($VERSION)"

PSITERM=${PSITERM:-$HERE/../psiterm}
if [ -d "$PSITERM/dist" ]; then
	# all three together: a .sis without its .sig would fail every updater
	cp "$SIS" "$PSITERM/dist/PsiWeb.sis.new"
	cp "$SIS.sig" "$PSITERM/dist/PsiWeb.sis.sig.new"
	mv "$PSITERM/dist/PsiWeb.sis.new" "$PSITERM/dist/PsiWeb.sis"
	mv "$PSITERM/dist/PsiWeb.sis.sig.new" "$PSITERM/dist/PsiWeb.sis.sig"
	echo "$VERSION" > "$PSITERM/dist/PsiWeb-version.txt"
	echo "copied to $PSITERM/dist: commit and push dist/PsiWeb.sis, dist/PsiWeb.sis.sig and dist/PsiWeb-version.txt there"
else
	echo "(no psiterm checkout at $PSITERM: copy the three dist files into psiterm's dist/ yourself)"
fi
