# atomtls.net.py - PsiWeb 0.90/0.91: direct https through the Atom modem's own
# TLS, in the 5mx emulator (core/tools/emu/net.py loads this file; asked for
# by name, needs the Internet):
#   python3 core/tools/emu/net.py atomtls
# The card has PsiWeb only (atomtls.card.ts). PsiWeb, the first program in
# Extras, starts with its setup assistant: OK on its standard choices for a
# 5mx (the Atom modem, Direct) writes PsiLink.ini with "Modem provides TLS"
# on, as PsiMail's assistant does for the Atom (0.91; before, only PsiMail's
# assistant set it). The engine starts again with it and
# https://example.com/ is opened. The emulated modem (psiconnect's
# hostmodem, AT$TLS=1 as standard) makes the TLS connection on 443 itself,
# checked against the PC's roots and the name, so PsiWeb must send its
# request in plain: the modem's log shows "GET / HTTP" after the CONNECT and
# the site's "200 OK" coming back. Without the setting PsiWeb wraps its own
# TLS inside the modem's (the site answers "400 Bad Request"). Then the
# toolband (its route light) and Page information, for the screenshots:
# "Atom" with the padlock shut, "Direct, checked by the Atom modem".
# EMU_ATOMTLS_* move the steps (simulated seconds).

import re


def test_atomtls(ctx):
    card = os.path.join(WORK, 'atomtls.img')
    make_card('atomtls', card, 'EMU_PKG_WEB')
    modem = ctx.modem('atomtls')
    E = lambda n, d: float(os.environ.get('EMU_ATOMTLS_' + n, d))
    s = Script(E('WEBSETUP', 88)).key(ENTER, gap=4)        # PsiWeb's setup assistant: OK (the Atom modem, Direct)
    s.at(E('OPEN', 130)).ctrl('o', gap=1.5).type('https://example.com/\r', gap=0.4)
    s.at(E('INFO', 190)).ctrl('t', shift=True, gap=4)       # View > Show > Toolband: the route light
    s.ctrl('q', shift=True, gap=4)                          # View > Page information
    out = emulate('atomtls', card, ctx.sock, s, secs=int(E('SECS', 12)))
    # once the call to example.com:443 was up, inside the modem's TLS: the
    # Psion's request in plain ("GET / HTTP/1.1", not a TLS handshake) and
    # the site's answer to it, as the modem logs them
    log = open(modem, 'rb').read().decode('latin-1')
    i = log.find('[tls example.com: handshake')
    after = log[i:] if i >= 0 else ''
    j = after.find('GET / HTTP/1.1')
    answer = ''.join(re.findall(r'^-> \d+ bytes \(total \d+\): (.*)$', after[j:], re.M)) if j >= 0 else ''
    ok = i >= 0 and j >= 0 and answer.startswith('HTTP/1.1 200')
    print('atomtls: request %s, answer %r' % ('in plain' if j >= 0 else 'not seen', answer[:15]), flush=True)
    return ok, out


TESTS['atomtls'] = test_atomtls
