#!/bin/bash
# test/cookie/run.sh [WORK] - cookies end to end in the ARM harness (0.89):
# the exact psiweb.exe code (links/emu/run_links.py) against a local server
# that sets and reads cookies (cookiesrv.py, 127.0.0.1 only). Each "session"
# is a fresh engine with the same C: folder, as PsiWeb closing and opening
# again on the Psion:
#   1. Kept for next time: /set, then /read sends both cookies back;
#      cookies.txt keeps psikept (it has a date), not psisession
#   2. ...the next session: /read sends psikept only
#   3. Until PsiWeb closes (the standard): /set, then /read sends both
#   4. ...the next session: /read sends nothing, and there is no cookies.txt
# Build the harness engine first: core/tools/docker/psibuild "make -f links/epoc.mk -j8 emu"
# (PW_BUILD names another build folder).
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
TOP=$(cd "$HERE/../.." && pwd)
WORK=${1:-$TOP/build/cookietest}
rm -rf "$WORK"; mkdir -p "$WORK"
PORT=$(python3 -c 'import socket; s=socket.socket(); s.bind(("127.0.0.1",0)); print(s.getsockname()[1])')
LOG=$WORK/server.log
python3 "$HERE/cookiesrv.py" $PORT "$LOG" &
SRV=$!
trap 'kill $SRV 2>/dev/null' EXIT
sleep 1
U=http://127.0.0.1:$PORT
fails=0
session() {   # name cdisk opts pages...
	local name=$1 cdisk=$2 opts=$3; shift 3
	: > "$WORK/$name.txt"
	for p in "$@"; do
		printf 'open %s%s\nidle 1500\nshot %s/%s-%s.png\n' "$U" "$p" "$WORK" "$name" "${p#/}" >> "$WORK/$name.txt"
	done
	echo quit >> "$WORK/$name.txt"
	echo "--- $name" >> "$LOG"
	PW_CDISK=$cdisk timeout 600 python3 "$TOP/links/emu/run_links.py" $opts "$WORK/$name.txt" > "$WORK/$name.log" 2>&1
}
check() {     # what, condition
	if eval "$2"; then echo "ok    $1"; else echo "FAIL  $1"; fails=$((fails + 1)); fi
}
last_read() { sed -n "/^--- $1\$/,/^---/p" "$LOG" | grep "GET /read" | tail -1; }
KEPT=$WORK/c-kept/System/Apps/PsiWeb/cookies.txt
OFF=$WORK/c-off/System/Apps/PsiWeb/cookies.txt

session kept1 "$WORK/c-kept" --keep-cookies /set /read
check "kept, same session: both cookies sent" '[[ "$(last_read kept1)" == *psikept=yes* && "$(last_read kept1)" == *psisession=yes* ]]'
check "kept: cookies.txt has psikept" 'grep -q psikept "$KEPT"'
check "kept: cookies.txt has no psisession" '! grep -q psisession "$KEPT"'
session kept2 "$WORK/c-kept" --keep-cookies /read
check "kept, next session: psikept sent (Expires)" '[[ "$(last_read kept2)" == *psikept=yes* ]]'
check "kept, next session: psiage sent (Max-Age)" '[[ "$(last_read kept2)" == *psiage=yes* ]]'
check "kept, next session: psisession not sent" '[[ "$(last_read kept2)" != *psisession* ]]'
session off1 "$WORK/c-off" "" /set /read
check "until closed, same session: both cookies sent" '[[ "$(last_read off1)" == *psikept=yes* && "$(last_read off1)" == *psisession=yes* ]]'
session off2 "$WORK/c-off" "" /read
check "until closed, next session: nothing sent" '[[ "$(last_read off2)" == "GET /read cookie=" ]]'
check "until closed: no cookies.txt" '[ ! -e "$OFF" ]'
grep -h "CRASH\|Traceback" "$WORK"/*.log && fails=$((fails + 1))
echo "cookie test: $fails failed (server log: $LOG)"
[ $fails = 0 ]
