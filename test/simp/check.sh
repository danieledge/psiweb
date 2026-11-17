#!/bin/bash
# test/simp/check.sh - the page simplifier built into psiweb.exe
# (links/simp/) is the PsiConnect Atom's, unchanged: compare the copy with
# psiconnect's (the checkout beside this one, or $PSICONNECT) and run
# psiconnect's own simplifier tests (hosttest/test_proxy) on it.
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
TOP=$(cd "$HERE/../.." && pwd)
PC=${PSICONNECT:-$TOP/../psiconnect}
[ -d "$PC/src" ] || { echo "no psiconnect checkout at $PC (set PSICONNECT)"; exit 2; }
for f in htmlsimp.cpp htmlsimp.h; do
	if cmp -s "$TOP/links/simp/$f" "$PC/src/$f"; then
		echo "$f: the same as psiconnect's"
	else
		echo "$f: differs from psiconnect's - copy it again (links/simp/README.md)"; exit 1
	fi
done
make -s -C "$PC/hosttest" test_proxy > /dev/null
(cd "$PC/hosttest" && ./test_proxy | tail -3)
