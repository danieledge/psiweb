# atomtls.net.py - PsiWeb 0.90: direct https through the Atom modem's own TLS,
# in the 5mx emulator (core/tools/emu/net.py loads this file; asked for by
# name, needs the Internet):
#   python3 core/tools/emu/net.py atomtls
# The card has PsiMail and PsiWeb (atomtls.card.ts). PsiWeb, the first program
# in Extras, starts first: its setup assistant is cancelled and PsiWeb
# closed. PsiMail is started from Extras, its first Mail account dialog
# cancelled (and the Email icon question answered No), and Tools > Connection settings > Set up... opens the setup
# assistant: OK on its standard choices for a 5mx (the Atom modem, no web
# helper) writes PsiLink.ini with "Modem provides TLS" on, as PsiMail's
# assistant does for the Atom. PsiMail is closed, PsiWeb
# started again and https://example.com/ opened. The emulated modem
# (psiconnect's hostmodem, AT$TLS=1 as standard) makes the TLS connection on
# 443 itself, checked against the PC's roots and the name, so PsiWeb must
# send its request in plain: the modem's log shows "GET / HTTP" after the
# CONNECT and the site's "200 OK" coming back. Before 0.90 PsiWeb wrapped its
# own TLS inside the modem's (the site answered "400 Bad Request"). Then the
# toolband (its route light) and Page information, for the screenshots:
# "Atom" with the padlock shut, "Direct, checked by the Atom modem".
# EMU_ATOMTLS_* move the steps (simulated seconds).

import re


def test_atomtls(ctx):
    card = os.path.join(WORK, 'atomtls.img')
    make_card('atomtls', card, 'EMU_PKG_WEB')
    modem = ctx.modem('atomtls')
    E = lambda n, d: float(os.environ.get('EMU_ATOMTLS_' + n, d))
    s = Script(E('WEBSETUP', 88)).key(ESC, gap=2)          # PsiWeb's setup assistant: Cancel
    s.ctrl('e', gap=4)                                      # File > Close
    s.tap(670, 262, gap=3)                                  # Extras
    s.tap(643, 212, gap=2)                                  # PsiMail
    s.at(E('MAILUP', 125)).key(ESC, gap=4)                  # PsiMail's first Mail account dialog: Cancel
    s.at(E('MAILQ', 138)).key(ESC, gap=3)                   # "Use the Email icon for PsiMail?": No
    s.key(MENU, gap=1.5).key(LEFT, gap=1.5).key(DOWN, gap=1).key(ENTER, gap=3)   # Tools > Connection settings...
    s.ctrl('u', gap=4)                                      # Set up...: the setup assistant
    s.key(ENTER, gap=4)                                     # OK on its choices: the Atom modem, Direct
    s.ctrl('e', gap=5)                                      # File > Close
    s.tap(670, 262, gap=3)                                  # Extras
    s.tap(335, 215, gap=2)                                  # PsiWeb
    s.at(E('OPEN', 210)).ctrl('o', gap=1.5).type('https://example.com/\r', gap=0.4)
    s.at(E('INFO', 270)).ctrl('t', shift=True, gap=4)       # View > Show > Toolband: the route light
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
