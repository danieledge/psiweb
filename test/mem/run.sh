#!/bin/bash
# test/mem/run.sh [WORK] - PsiWeb 1.00's memory, in the ARM harness
# (links/emu/run_links.py: the exact psiweb.exe code, the netBook's screen)
# against memsrv.py on 127.0.0.1. Each line of the summary is the engine's
# own "[mem]" figure (its heap in use) after a page or a Trim:
#   pics-then-text  the picture page, then a text page: the pictures of the
#                   page before go when the text page is done
#   prog            one 1600x1200 progressive JPEG: its page peak (measured
#                   without decoding; shown on the netBook, up to 8 MB to
#                   decode; refused as too big on the 5mx, over 4 MB)
#   bomb            48 KB of gzip that inflates to 12 MB: the engine ends the
#                   page as "too big" and lives
#   trim-lighter    a 380 KB news page in the Lighter style, then Trim (cmd 21)
#   cookies-links   3 pages that set 40 cookies each, then 60 linked pages:
#                   cookies.txt keeps 30 for the site
# PW_BUILD names another build folder (an older engine, to compare).
# Build the harness engine first: core/tools/docker/psibuild "make -f links/epoc.mk -j8 emu"
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
TOP=$(cd "$HERE/../.." && pwd)
WORK=${1:-$TOP/build/memtest}
rm -rf "$WORK"; mkdir -p "$WORK/www" "$WORK/c"
python3 "$TOP/links/mktestpage.py" "$WORK/www" > /dev/null
PORT=$(python3 -c 'import socket; s=socket.socket(); s.bind(("127.0.0.1",0)); print(s.getsockname()[1])')
python3 "$HERE/memsrv.py" $PORT "$WORK/www" &
SRV=$!
trap 'kill $SRV 2>/dev/null' EXIT
sleep 1
U=http://127.0.0.1:$PORT
run() {   # name opts lines...
	local name=$1 opts=$2; shift 2
	printf '%s\n' "$@" quit > "$WORK/$name.txt"
	PW_CDISK=$WORK/c timeout 1800 python3 "$TOP/links/emu/run_links.py" --count --netbook $opts "$WORK/$name.txt" > "$WORK/$name.log" 2>&1 || true
	echo "$name"
	grep -E "\[mem\]|CRASH|rc=" "$WORK/$name.log" | sed 's/^/    /'
}
run pics-then-text --images "open $U/images.html" "idle 3000" "open $U/text.html" "idle 3000"
run prog --images "open $U/prog.html" "idle 3000"
run bomb "" "open $U/bomb.html" "idle 3000"
run trim-lighter "--style 1" "open $U/big.html" "idle 3000" "cmd 21" "wait 500" "idle 2000"
L=""
for i in 1 2 3; do L="$L open $U/cookies/$i|idle 1500|"; done
for i in $(seq 0 59); do L="$L open $U/link/$i|idle 500|"; done
IFS='|' read -r -a LINES <<< "$L"
run cookies-links --keep-cookies "${LINES[@]# }"
echo "cookies kept: $(grep -c 127.0.0.1 "$WORK/c/System/Apps/PsiWeb/cookies.txt" 2>/dev/null || echo 0)"
