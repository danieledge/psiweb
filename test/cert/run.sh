#!/bin/bash
# run.sh - PsiWeb's certificate check (0.84) on a PC: psion-core's
# certcheck.c (with its ECDSA, ecverify.h), pmrsa.c and libtommath with
# engine/pwcert.c, built for the host (cc) as psiweb.exe builds them:
#  - ectest: ECDSA on P-256 and P-384 against cases made by python's
#    cryptography (good, high s, r or s changed, 0 or n, the hash changed,
#    the key off the curve or of the other curve, damaged DER);
#  - certhost: real chains fetched now with openssl, RSA and ECDSA (needs
#    the internet, python3 with cryptography), and spoilt ones.
#   test/cert/run.sh            (from the checkout; writes build/cert-test/)
# PSION_CORE=... tests another psion-core checkout. The end to end checks
# are the ARM harness (links/emu/run_pages.sh) and test/emu/cert.net.py.
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
TOP=$(cd "$HERE/../.." && pwd)
CORE=${PSION_CORE:-$TOP/core}
B=$TOP/build/cert-test
rm -rf "$B"; mkdir -p "$B/ltc" "$B/ltm" "$B/run" "$B/lib"
LTC=$CORE/ssh/db/libtomcrypt/src
LTM=$CORE/ssh/db/libtommath
CF="-O1 -g -w -std=gnu99 -D_DEFAULT_SOURCE"
TF="$CF -I$TOP/tls -I$CORE/web/tls -I$LTC/headers -DLTC_SOURCE -DLTC_NO_ASM"
for s in hashes/sha2/sha256 hashes/sha2/sha384 hashes/sha2/sha512 misc/zeromem misc/burn_stack misc/crypt/crypt_argchk; do
	cc -c $TF "$LTC/$s.c" -o "$B/ltc/$(echo $s | tr / _).o"
done
for s in $(sed -n 's|^libtommath/||p' "$CORE/ssh/ltm_sources.txt"); do
	cc -c $CF -I"$TOP/tls" -I"$LTM" "$LTM/$s" -o "$B/ltm/$(basename "$s" .c).o"
done
cc -c $CF -I"$TOP/tls" -I"$LTM" "$CORE/mail/engine/tlsv/pmrsa.c" -o "$B/lib/pmrsa.o"
cc -c $TF -I"$CORE/mail/engine/cal" -I"$CORE/ssh" '-DTLSV_APP_NAME="PsiWeb"' "$CORE/mail/engine/tlsv/certcheck.c" -o "$B/lib/certcheck.o"
cc -c $TF -I"$CORE/mail/engine/cal" "$TOP/engine/pwcert.c" -o "$B/lib/pwcert.o"
cc -c $CF "$HERE/stubs.c" -o "$B/lib/stubs.o"
cc -c $CF -I"$TOP/engine" "$HERE/certhost.c" -o "$B/certhost.o"
cc -c $CF "$HERE/ectest.c" -o "$B/ectest.o"
cc -o "$B/certhost" "$B/certhost.o" "$B"/lib/*.o "$B"/ltc/*.o "$B"/ltm/*.o
cc -o "$B/ectest" "$B/ectest.o" "$B"/lib/*.o "$B"/ltc/*.o "$B"/ltm/*.o

pass=0 fail=0
python3 "$HERE/mkvec.py" "$B/vectors.txt"
if "$B/ectest" "$B/vectors.txt"; then pass=$((pass + 1)); else fail=$((fail + 1)); fi

RSA="rsa_pss_rsae_sha256:rsa_pkcs1_sha256:rsa_pkcs1_sha384:rsa_pkcs1_sha512"
ECDSA="ecdsa_secp256r1_sha256:ecdsa_secp384r1_sha384"
fetch() {   # name host sigalgs -> $B/<name>.msg (and <name>-spoilt.msg)
	echo | openssl s_client -showcerts -tls1_3 -sigalgs "$3" -servername "$2" -connect "$2:443" 2>/dev/null |
		sed -n '/BEGIN CERT/,/END CERT/p' > "$B/$1.pem"
	python3 "$HERE/mkmsg.py" "$B/$1.pem" "$B/$1.msg"
	python3 "$HERE/mkmsg.py" "$B/$1.pem" "$B/$1-spoilt.msg" --spoil
}
fetch npr text.npr.org $RSA
fetch bbc www.bbc.co.uk $RSA
fetch guardian www.theguardian.com $RSA
fetch wikipedia en.m.wikipedia.org $ECDSA
fetch hn news.ycombinator.com $ECDSA
fetch frogfind frogfind.com $ECDSA
fetch cloudflare www.cloudflare.com $ECDSA
fetch cloudflare-rsa www.cloudflare.com $RSA
fetch google www.google.com $ECDSA
fetch cnnlite lite.cnn.com $ECDSA
fetch example example.com $ECDSA
openssl req -x509 -newkey rsa:2048 -nodes -keyout "$B/self.key" -out "$B/self.pem" -days 30 \
	-subj "/CN=localhost" -addext "subjectAltName=DNS:localhost" 2>/dev/null
python3 "$HERE/mkmsg.py" "$B/self.pem" "$B/self.msg"
openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:P-256 -nodes -keyout "$B/selfec.key" -out "$B/selfec.pem" -days 30 \
	-subj "/CN=localhost" -addext "subjectAltName=DNS:localhost" 2>/dev/null
python3 "$HERE/mkmsg.py" "$B/selfec.pem" "$B/selfec.msg"

check() {   # name expected-start command...
	local name=$1 want=$2; shift 2
	local got
	got=$(cd "$B/run" && "$@" 2>>"$B/log.txt") || true
	if [ "${got#"$want"}" != "$got" ]; then pass=$((pass + 1)); echo "ok    $name: $got"
	else fail=$((fail + 1)); echo "FAIL  $name: $got (wanted $want...)"; fi
}
C=$B/certhost
check "npr (RSA, ISRG Root X1)"       "trusted" $C "$B/npr.msg" text.npr.org
check "npr again (certs.txt)"         "trusted" $C "$B/npr.msg" text.npr.org
check "bbc (RSA, GlobalSign R3)"      "trusted" $C "$B/bbc.msg" www.bbc.co.uk
check "guardian (RSA)"                "trusted" $C "$B/guardian.msg" www.theguardian.com
check "wikipedia (ECDSA, ISRG Root X2)" "trusted" $C "$B/wikipedia.msg" en.m.wikipedia.org
check "hacker news (ECDSA)"           "trusted" $C "$B/hn.msg" news.ycombinator.com
check "frogfind (ECDSA)"              "trusted" $C "$B/frogfind.msg" frogfind.com
check "cloudflare (ECDSA, GTS R4)"    "trusted" $C "$B/cloudflare.msg" www.cloudflare.com
check "cloudflare (RSA)"              "trusted" $C "$B/cloudflare-rsa.msg" www.cloudflare.com
check "google (ECDSA)"                "trusted" $C "$B/google.msg" www.google.com
check "cnn lite (ECDSA P-384 leaf)"   "trusted" $C "$B/cnnlite.msg" lite.cnn.com
check "example.com (ECDSA, SSL.com 2022)" "trusted" $C "$B/example.msg" example.com
check "example.com, signature spoilt" "Certificate not trusted - example.com" $C "$B/example-spoilt.msg" example.com
check "wikipedia, signature spoilt"   "Certificate not trusted - en.m.wikipedia.org" $C "$B/wikipedia-spoilt.msg" en.m.wikipedia.org
check "cloudflare, signature spoilt"  "Certificate not trusted - www.cloudflare.com" $C "$B/cloudflare-spoilt.msg" www.cloudflare.com
check "npr, signature spoilt"         "Certificate not trusted - text.npr.org" $C "$B/npr-spoilt.msg" text.npr.org
check "wikipedia's chain, other name" "Certificate not trusted - www.example.com | its certificate is for a different name" \
	$C "$B/wikipedia.msg" www.example.com
check "npr's chain, other name"       "Certificate not trusted - www.example.com | its certificate is for a different name" \
	$C "$B/npr.msg" www.example.com
check "self-signed RSA"               "Certificate not trusted - localhost | its certificate is not from an authority PsiWeb knows" \
	$C "$B/self.msg" localhost
check "self-signed ECDSA"             "Certificate not trusted - localhost | its certificate is not from an authority PsiWeb knows" \
	$C "$B/selfec.msg" localhost
check "npr in 2040 (expired)"         "Certificate not trusted - text.npr.org | its certificate has expired" \
	env PW_NOW=2208988800 $C "$B/npr.msg" text.npr.org
check "handshake turned down"         "Secure connection refused - example.org" $C --alert example.org
echo "$pass passed, $fail failed"
[ $fail = 0 ]
