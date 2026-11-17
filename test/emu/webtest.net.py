# webtest.net.py - PsiWeb's Test the connection with its web part (0.89),
# when no page comes, in the 5mx emulator through the emulated modem
# (core/tools/emu/net.py loads this file; asked for by name; offline):
#   python3 core/tools/emu/net.py webtest
# The card is set up as someone with the Atom would set it up: PsiWeb's own
# setup assistant, which opens by itself on a new card, OK on its standard
# choices for a 5mx (the Atom modem, Direct), so "Modem provides TLS" is on
# (0.91). Two calls, both kept on this PC:
# - https://localhost:4433/ (not port 443, so PsiWeb does its own TLS): the
#   modem's redirect takes the plain call to a local TLS server with a
#   self-signed certificate (cert.net.py's), and the page fails with "Page
#   not opened" (the certificate); no request is sent.
# - Its "Test the connection" link then runs the link's test (the modem
#   answers AT) and the web part: PsiWeb's small file from GitHub, https on
#   443, which with the setting on is the Atom's own TLS call. hostmodem's
#   TLS calls go to a local port that refuses them (Ctx.modem
#   tls_redirect='refuse'), so the modem says NO CARRIER at once, as the Atom
#   does for a site it cannot reach, and the result window must say that no
#   page came and why.
# Nothing dials out: the modem's log must show the GitHub call redirected
# and no call connected anywhere but 127.0.0.1. The page that does come is
# webtestok (below, the Internet) and the ARM harness (links/emu/run_links.py,
# "test URL"). Screenshots in build/emu/net/webtest/ (end.png: the result
# window). EMU_WEBTEST_* move the steps (simulated seconds, the tap's x, y).

import re


def test_webtest(ctx):
    port = free_port()
    log = _cert_server(ctx, port)
    if not wait_port(port):
        return False, 'openssl s_server did not start'
    card = os.path.join(WORK, 'webtest.img')
    make_card('psiweb', card, 'EMU_PKG_WEB')
    modem = ctx.modem('webtest', redirect='127.0.0.1:%d' % port, tls_redirect='refuse')
    E = lambda n, d: float(os.environ.get('EMU_WEBTEST_' + n, d))
    s = Script(E('SETUP', 88)).key(ENTER, gap=4)        # the setup assistant: OK (the Atom modem, Direct)
    s.at(E('OPEN', 130)).ctrl('o', gap=1.5).type('https://localhost:4433/\r', gap=0.4)
    # the "Page not opened" page: its Test the connection link
    s.at(E('TAP_AT', 165)).tap(int(E('X', 220)), int(E('Y', 112)))
    out = emulate('webtest', card, ctx.sock, s, secs=int(E('SECS', 60)))
    m = open(modem, 'rb').read().decode('latin-1')
    # every call connected was to this PC; GitHub's was the modem's TLS
    # call, redirected and refused
    connected = re.findall(r'\[dial (\S+): connected\]', m)
    local = connected and all(h.startswith('127.0.0.1:') for h in connected)
    github = re.search(r'\[dial raw\.githubusercontent\.com:443 -> 127\.0\.0\.1:\d+\]', m) and \
        'tls raw.githubusercontent.com:443: no connection' in m
    # the page's call reached the TLS server and PsiWeb sent no request
    page = grep(log, 'ACCEPT') and not grep(log, 'GET /')
    print('webtest: calls connected %s; GitHub %s; page %s' % (connected, 'refused' if github else 'not seen',
          'refused by PsiWeb' if page else 'not as expected'), flush=True)
    return bool(local and github and page), out


TESTS['webtest'] = test_webtest


# webtestok: the same window when the page does come. A new card's setup
# assistant (it opens by itself) is set to the Atom modem helper: Up to the
# page tabs, Right to "Web helper", Down to "Pages are fetched by", Right to
# "Atom modem helper", Enter. Then Ctrl+O and about:cmd-test, which is what
# a "Page not opened" page's link asks for. The link's test has the modem
# answer AT; the web part dials psiproxy, and the modem's own proxy fetches
# the file from the real GitHub (this needs the Internet; not in "all").
def test_webtestok(ctx):
    card = os.path.join(WORK, 'webtestok.img')
    make_card('psiweb', card, 'EMU_PKG_WEB')
    modem = ctx.modem('webtestok')
    s = Script(60).key(UP, gap=1).key(RIGHT, gap=1).key(DOWN, gap=1).key(RIGHT, gap=1).wait(1).key(ENTER, gap=25)
    s.ctrl('o', gap=2).type('about:cmd-test\r', gap=0.7)
    out = emulate('webtestok', card, ctx.sock, s, secs=int(os.environ.get('EMU_WEBTEST_SECS', 60)))
    ok = grep(modem, 'proxy: GET ') and grep(modem, 'raw.githubusercontent.com') and grep(modem, '-> 200')
    return ok, out


TESTS['webtestok'] = test_webtestok


# routebusy: the busy message with the route while a page opens (0.89), for
# the screenshots: the modem sends the call for 68k.news to nethttp.py, so
# the message reads "Opening 68k.news direct...". build/emu/net/routebusy/.
def test_routebusy(ctx):
    port = free_port()
    log = os.path.join(WORK, 'routebusy-http.log')
    ctx.procs.start('http', [sys.executable, os.path.join(HERE, 'nethttp.py'), str(port)], log)
    if not wait_port(port):
        return False, 'nethttp.py did not start'
    card = os.path.join(WORK, 'routebusy.img')
    make_card('psiweb', card, 'EMU_PKG_WEB')
    ctx.modem('routebusy', redirect='127.0.0.1:%d' % port)
    s = Script(60).key(ESC, gap=2).ctrl('o', gap=2).type('68k.news\r', gap=0.7)
    out = emulate('routebusy', card, ctx.sock, s, secs=25)
    return grep(log, 'GET / HTTP'), out


TESTS['routebusy'] = test_routebusy
