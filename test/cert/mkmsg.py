#!/usr/bin/env python3
"""mkmsg.py CHAIN.pem OUT [--spoil] - the chain as a TLS 1.3 Certificate
message body (as psimail's test/certtest.py), for certhost. --spoil changes
a byte of the first certificate's signature."""
import sys
from cryptography import x509
from cryptography.hazmat.primitives.serialization import Encoding
certs = x509.load_pem_x509_certificates(open(sys.argv[1], "rb").read())
body = b""
for i, c in enumerate(certs):
    d = c.public_bytes(Encoding.DER)
    if i == 0 and "--spoil" in sys.argv:
        d = d[:-3] + bytes([d[-3] ^ 1]) + d[-2:]
    body += len(d).to_bytes(3, "big") + d + b"\0\0"
open(sys.argv[2], "wb").write(b"\0" + len(body).to_bytes(3, "big") + body)
