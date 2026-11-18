#!/bin/bash
# test/stop/run.sh [WORK] - Stop and keys during an https read, in the ARM
# harness (0.93): the exact psiweb.exe code (links/emu/run_links.py) on
# example.com over PsiWeb's own TLS, played back from example/net.json (no
# network needed) at 200 bytes a second, so the page's one TLS record takes
# about 4 s to come. run_links.py --wakes sets net.quit and net.resized as
# PsiWeb.app does.
#   1. Stop in the middle of the record: the read ends, the page says
#      Stopped (it went round for ever before 0.93: psi_read said EINTR,
#      which Links' EINTRLOOP called again at once, with net.quit still set)
#   2. A key in the middle of the record: the page still comes. This needs
#      psion-core's tls13.c to wait through a key (net_wait); with an older
#      core the key ends the connection, which is reported, not failed.
# Build the harness engine first: core/tools/docker/psibuild "make -f links/epoc.mk -j8 emu"
# (PW_BUILD names another build folder).
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
TOP=$(cd "$HERE/../.." && pwd)
WORK=${1:-$TOP/build/stoptest}
rm -rf "$WORK"; mkdir -p "$WORK"
fails=0
run() {       # name, then the script's lines after the page is on its way
	local name=$1; shift
	{ echo "open https://example.com/"; echo "until Request sent"; echo "wait 1000"; printf '%s\n' "$@"; echo quit; } > "$WORK/$name.txt"
	timeout 120 python3 "$TOP/links/emu/run_links.py" --replay "$HERE/example" --rate 200 --wakes \
		"$WORK/$name.txt" > "$WORK/$name.log" 2>&1 && echo 0 > "$WORK/$name.exit" || echo $? > "$WORK/$name.exit"
}
check() {     # what, condition
	if eval "$2"; then echo "ok    $1"; else echo "FAIL  $1"; fails=$((fails + 1)); fi
}
has() { grep -q -- "$2" "$WORK/$1.log"; }

run stop "cmd 5" "wait 5000"
check "Stop during a read: the engine carries on (no endless read)" '[ "$(cat $WORK/stop.exit)" = 0 ] && has stop "> quit"'
check "Stop during a read: Stopped" 'has stop "Stopped"'
check "Stop during a read: no Page not opened" '! has stop "Page not opened"'

run key "key 32" "wait 8000"
check "key during a read: the engine carries on" '[ "$(cat $WORK/key.exit)" = 0 ] && has key "> quit"'
if grep -q "net_wait" "$TOP/core/ssh/tls13.c"; then
	check "key during a read: the page comes" 'has key "\[title\] Example Domain" && has key "\[status\] OK"'
elif has key "\[title\] Example Domain"; then
	echo "ok    key during a read: the page comes"
else
	echo "note  key during a read: the connection ended (this psion-core's tls13.c has no net_wait)"
fi
grep -h "CRASH\|Traceback" "$WORK"/*.log && fails=$((fails + 1))
echo "stop test: $fails failed (logs in $WORK)"
[ $fails = 0 ]
