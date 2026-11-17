#!/usr/bin/env python3
"""mkvec.py OUT - ECDSA test cases for ectest.c, made with python's
cryptography (OpenSSL): good signatures on P-256 and P-384 with SHA-256,
384 and 512, and each spoilt: r or s changed, r or s 0 or n, the hash
changed, the key off the curve, the other curve's key, a high s (still
good), a bad DER. Lines: curve pubhex hashhex sighex expect(1 good, 0 bad)"""
import sys, os
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric.utils import decode_dss_signature, encode_dss_signature
from cryptography.hazmat.primitives import hashes

N = {256: 0xFFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551,
     384: 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFC7634D81F4372DDF581A0DB248B0A77AECEC196ACCC52973}
P = {256: 2**256 - 2**224 + 2**192 + 2**96 - 1, 384: 2**384 - 2**128 - 2**96 + 2**32 - 1}
out = []
def pub(k, nb):
    n = k.public_key().public_numbers()
    return n.x.to_bytes(nb, "big") + n.y.to_bytes(nb, "big")
def line(bits, pk, h, sig, ok, why):
    out.append("%d %s %s %s %d %s" % (bits, pk.hex(), h.hex(), sig.hex(), ok, why))
for bits, curve in ((256, ec.SECP256R1()), (384, ec.SECP384R1())):
    nb = bits // 8
    other = ec.generate_private_key(ec.SECP384R1() if bits == 256 else ec.SECP256R1())
    for t in range(12):
        k = ec.generate_private_key(curve)
        pk = pub(k, nb)
        alg = (hashes.SHA256(), hashes.SHA384(), hashes.SHA512())[t % 3]
        msg = os.urandom(40 + t)
        sig = k.sign(msg, ec.ECDSA(alg))
        hh = hashes.Hash(alg); hh.update(msg); h = hh.finalize()
        r, s = decode_dss_signature(sig)
        line(bits, pk, h, sig, 1, "good-%s" % alg.name)
        if t < 4:
            line(bits, pk, h, encode_dss_signature(r, N[bits] - s), 1, "high-s")
            line(bits, pk, h, encode_dss_signature(r ^ 1, s), 0, "r-changed")
            line(bits, pk, h, encode_dss_signature(r, s ^ 4), 0, "s-changed")
            line(bits, pk, h, encode_dss_signature(0, s), 0, "r-zero")
            line(bits, pk, h, encode_dss_signature(r, 0), 0, "s-zero")
            line(bits, pk, h, encode_dss_signature(N[bits], s), 0, "r-is-n")
            line(bits, pk, h, encode_dss_signature(r, N[bits] + s if N[bits] + s < 2 ** bits else s), 1 if N[bits] + s >= 2 ** bits else 0, "s-plus-n")
            line(bits, pk, bytes([h[0] ^ 0x80]) + h[1:], sig, 0, "hash-changed")
            y = (int.from_bytes(pk[nb:], "big") + 1) % P[bits]
            line(bits, pk[:nb] + y.to_bytes(nb, "big"), h, sig, 0, "off-curve")
            line(bits, pk[:nb] + P[bits].to_bytes(nb, "big"), h, sig, 0, "y-is-p")
            line(bits, pub(other, 48 if bits == 256 else 32), h, sig, 0, "other-curve-key")
            if bits == 256:   # this key's point, padded, taken as a P-384 one: not on that curve
                line(384, bytes(16) + pk[:32] + bytes(16) + pk[32:], h, sig, 0, "wrong-curve")
            line(bits, pk, h, sig[:-1], 0, "der-cut")
            line(bits, pk, h, sig + b"\0", 0, "der-trailing")
            line(bits, pk, h, b"\x30" + bytes([len(sig) - 1]) + sig[2:], 0, "der-length")
open(sys.argv[1], "w").write("\n".join(out) + "\n")
