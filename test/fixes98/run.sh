#!/bin/bash
# test/fixes98/run.sh [WORK] - 0.98's engine changes in the ARM harness
# (links/emu/run_links.py: the exact psiweb.exe code) against a local server
# (srv.py, 127.0.0.1 only):
#   about:cmd- links work from PsiWeb's own pages (a "Page not opened"
#     page's Set up PsiWeb and Test the connection) and from nowhere else
#     (a web page's link, a redirect, a refresh)
#   a '<' inside an attribute value is not taken for a tag (a refresh and a
#     form written inside title attributes)
#   a picture shown wider than it is is drawn across its whole width
#   Reload in Lighter style shows the new page, not the old one's text
#   Sleep keeps every cookie (the session's too) for the waking engine, which
#     deletes the file at once; one never woken from is deleted unread
#   with the Psion's clock reset, an https page says to set the date
# Build the harness engine first: core/tools/docker/psibuild "make -f links/epoc.mk -j8 emu"
# (PW_BUILD names another build folder).
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
TOP=$(cd "$HERE/../.." && pwd)
WORK=${1:-$TOP/build/fixes98}
rm -rf "$WORK"; mkdir -p "$WORK"
PORT=$(python3 -c 'import socket; s=socket.socket(); s.bind(("127.0.0.1",0)); print(s.getsockname()[1])')
LOG=$WORK/server.log
: > "$LOG"
python3 "$HERE/srv.py" $PORT "$LOG" &
SRV=$!
trap 'kill $SRV 2>/dev/null' EXIT
sleep 1
U=http://127.0.0.1:$PORT
fails=0
run() {       # name opts script-lines...
	local name=$1 opts=$2; shift 2
	mkdir -p "$WORK/c-$name"
	printf '%s\n' "$@" quit | sed "s|@U|$U|g; s|@W|$WORK|g" > "$WORK/$name.txt"
	echo "--- $name" >> "$LOG"
	PW_CDISK=${CDISK:-$WORK/c-$name} timeout 600 python3 "$TOP/links/emu/run_links.py" $opts "$WORK/$name.txt" > "$WORK/$name.log" 2>&1 || true
}
check() {     # what, condition
	if eval "$2"; then echo "ok    $1"; else echo "FAIL  $1"; fails=$((fails + 1)); fi
}
served() { sed -n "/^--- $1\$/,/^---/p" "$LOG" | grep -c "GET $2" || true; }

# about:cmd- from PsiWeb's own "Page not opened" page (about:problem)
run own "" "open http://127.0.0.1:1/" "idle 2000" "pen 270 98" "wait 1500" "pen 150 98" "wait 1500"
check "own page: Set up PsiWeb reaches the app" 'grep -q "\[app_cmd\] 2" "$WORK/own.log"'
check "own page: Test the connection reaches the app" 'grep -q "\[app_cmd\] 1" "$WORK/own.log"'
# ...and from a web page: a link, a redirect, a refresh
run web "" "open @U/cmdlink" "idle 1500" "pen 120 11" "wait 1500" "open @U/redir" "idle 1500" "open @U/refresh" "idle 3000"
check "web page: its link, redirect and refresh do nothing" '! grep -q "\[app_cmd\]" "$WORK/web.log"'
check "web page: all three pages were served" '[ "$(served web /cmdlink)" = 1 ] && [ "$(served web /redir)" = 1 ] && [ "$(served web /refresh)" = 1 ]'
# ...and a redirect from a page opened from PsiWeb's own page: Try again on
# a "Page not opened" page, once the server is there, sends it about:cmd-setup
PORT2=$(python3 -c 'import socket; s=socket.socket(); s.bind(("127.0.0.1",0)); print(s.getsockname()[1])')
(sleep 5; exec python3 "$HERE/srv.py" $PORT2 "$WORK/server2.log") &
SRV2=$!
trap 'kill $SRV $SRV2 2>/dev/null' EXIT
run again "" "open http://127.0.0.1:$PORT2/redir" "idle 1500" "wait 7000" "pen 50 98" "wait 2500"
check "own page: a redirect from Try again does nothing" '! grep -q "\[app_cmd\]" "$WORK/again.log"'
check "own page: Try again reached the server" 'grep -q "GET /redir" "$WORK/server2.log"'

# '<' inside attribute values: no Refresh line to /evil, no form to /evilform
run attr "" "open @U/attr" "idle 1500" "shot @W/attr.png" "open @U/attrclean" "idle 1500" "shot @W/attrclean.png" \
	"open @U/form" "idle 1500" "pen 150 38" "wait 500" "type psion" "key 13" "wait 2000"
check "attribute: no Refresh line from inside a title" 'python3 "$HERE/same.py" "$WORK/attr.png" "$WORK/attrclean.png" same'
check "attribute: no form from inside a title" '! grep -q "GET /evilform" "$LOG"'

# a 16x4 picture shown 320x40: black over its left 160 pixels
run pic "--images" "open @U/pic" "idle 2500" "shot @W/pic.png"
check "picture: drawn across its width" 'python3 "$HERE/picwidth.py" "$WORK/pic.png" 140 180'

# Lighter style, Reload: the new version's text
run style "--style 1" "open @U/style" "idle 2000" "shot @W/style1.png" "cmd 4" "idle 2500" "shot @W/style2.png"
check "style: first page is version ONE" 'grep -q "\[title\] Version ONE" "$WORK/style.log"'
check "style: Reload shows version TWO" 'grep -q "\[title\] Version TWO" "$WORK/style.log"'
check "style: Reload's text is version TWO" 'python3 "$HERE/same.py" "$WORK/style1.png" "$WORK/style2.png" differ'

# Sleep keeps the cookies an engine awake would hold, the session's too:
# written to Sleep\cookies.txt, read once by the waking engine and deleted at
# once; a file never woken from is deleted unread at the next start
for keep in off on; do
	opt=""; [ $keep = on ] && opt="--keep-cookies"
	SL=$WORK/c-sleep-$keep/System/Apps/PsiWeb/Sleep
	CDISK=$WORK/c-sleep-$keep run sleep-$keep "$opt" "open @U/set" "idle 1500" "sleep"
	check "sleep ($keep): history written" '[ -s "$SL/history.txt" ]'
	check "sleep ($keep): the session cookie written" 'grep -q psisession "$SL/cookies.txt"'
	check "sleep ($keep): the dated cookie written" 'grep -q psikept "$SL/cookies.txt"'
	CDISK=$WORK/c-sleep-$keep run wake-$keep "--waking $opt" "idle 1500" "open @U/attrclean" "idle 2000"
	check "wake ($keep): the file deleted" '[ ! -e "$SL/cookies.txt" ]'
	check "wake ($keep): both cookies sent" 'sed -n "/^--- wake-$keep\$/,/^---/p" "$LOG" | grep "GET /attrclean" | grep -q "psisession=yes" && sed -n "/^--- wake-$keep\$/,/^---/p" "$LOG" | grep "GET /attrclean" | grep -q "psikept=yes"'
done
check "kept for next time: no session cookie in cookies.txt after a waking" '! grep -q psisession "$WORK/c-sleep-on/System/Apps/PsiWeb/cookies.txt"'
check "until PsiWeb closes: no cookies.txt after a waking" '[ ! -e "$WORK/c-sleep-off/System/Apps/PsiWeb/cookies.txt" ]'
# a Sleep never woken from: PsiWeb started afresh deletes it unread
SL=$WORK/c-stale/System/Apps/PsiWeb/Sleep
mkdir -p "$SL"
printf '"127.0.0.1" "127.0.0.1" "/" "psiold" "yes" 0 "Thu, 01 Jan 2037 00:00:00 GMT" "Thu, 01 Jan 2037 00:00:00 GMT"\n' > "$SL/cookies.txt"
CDISK=$WORK/c-stale run stale "" "idle 1500" "open @U/attrclean" "idle 2000"
check "stale: the file deleted" '[ ! -e "$SL/cookies.txt" ]'
check "stale: its cookies not used" '! sed -n "/^--- stale\$/,/^---/p" "$LOG" | grep -q psiold'

# the clock reset (psion-core CR-20): an https page says to set the date
# (example.com played back from test/stop/example, the clock put at 2001)
printf 'open https://example.com/\nidle 3000\nquit\n' > "$WORK/clock.txt"
mkdir -p "$WORK/c-clock"
PW_CDISK=$WORK/c-clock PW_CLOCK_OFFSET=-800000000 timeout 300 python3 "$TOP/links/emu/run_links.py" --replay "$TOP/test/stop/example" "$WORK/clock.txt" > "$WORK/clock.log" 2>&1 || true
check "clock reset: the reason is the clock" 'grep -q "\[status\] Clock not set - the certificate of example.com" "$WORK/clock.log"'
check "clock reset: a Page not opened page" 'grep -q "\[url\] about:problem" "$WORK/clock.log"'
printf 'open https://example.com/\nidle 3000\nquit\n' > "$WORK/clockok.txt"
mkdir -p "$WORK/c-clockok"
PW_CDISK=$WORK/c-clockok timeout 300 python3 "$TOP/links/emu/run_links.py" --replay "$TOP/test/stop/example" "$WORK/clockok.txt" > "$WORK/clockok.log" 2>&1 || true
check "clock right: the page comes" 'grep -q "\[title\] Example Domain" "$WORK/clockok.log"'
grep -h "CRASH\|Traceback" "$WORK"/*.log && fails=$((fails + 1))
echo "fixes98 test: $fails failed (server log: $LOG)"
[ $fails = 0 ]
