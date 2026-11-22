#!/bin/bash
# test/simp/check.sh - the page simplifier built into psiweb.exe
# (links/simp/) is the PsiConnect Atom's with PsiWeb's own change
# (links/simp/psiweb.diff, 1.03): compare the copy with psiconnect's (the
# checkout beside this one, or $PSICONNECT) with that change applied, and
# run psiconnect's own simplifier tests (hosttest/test_proxy) on it.
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
TOP=$(cd "$HERE/../.." && pwd)
PC=${PSICONNECT:-$TOP/../psiconnect}
[ -d "$PC/src" ] || { echo "no psiconnect checkout at $PC (set PSICONNECT)"; exit 2; }
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
cp "$PC/src/htmlsimp.cpp" "$PC/src/htmlsimp.h" "$T/"
(cd "$T" && patch -s -p1 < "$TOP/links/simp/psiweb.diff") || { echo "psiweb.diff does not apply to psiconnect's copy"; exit 1; }
for f in htmlsimp.cpp htmlsimp.h; do
	if cmp -s "$TOP/links/simp/$f" "$T/$f"; then
		echo "$f: psiconnect's with psiweb.diff"
	else
		echo "$f: differs from psiconnect's with psiweb.diff - copy it again (links/simp/README.md)"; exit 1
	fi
done
make -s -C "$PC/hosttest" test_proxy > /dev/null
(cd "$PC/hosttest" && ./test_proxy | tail -3)
