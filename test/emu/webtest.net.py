# webtest.net.py - PsiWeb 0.89's Test the connection with its web part, in
# the 5mx emulator through the emulated modem (core/tools/emu/net.py loads
# this file; asked for by name):
#   python3 core/tools/emu/net.py webtest
# The modem sends every call to a local TLS server with a self-signed
# certificate (cert.net.py's), so a first page fails with "Page not opened"
# (the certificate); its "Test the connection" link then runs the link's
# test (the modem answers AT) and the web part: PsiWeb fetches its small
# file from GitHub direct, which the modem's redirect takes to the same
# server, so the result window must say that no page came and why. The
# page that does come (from the real GitHub) is tested in the ARM harness
# (links/emu/run_links.py, script line "test URL"). Screenshots in
# build/emu/net/webtest/.

def test_webtest(ctx):
    port = free_port()
    log = _cert_server(ctx, port)
    if not wait_port(port):
        return False, 'openssl s_server did not start'
    card = os.path.join(WORK, 'webtest.img')
    make_card('psiweb', card, 'EMU_PKG_WEB')
    modem = ctx.modem('webtest', redirect='127.0.0.1:%d' % port)
    s = Script(60).key(ESC, gap=2).ctrl('o').wait(1).type('https://localhost:4433/\r', gap=0.4)
    # the "Page not opened" page is up by ~85 s: its Test the connection link
    s.at(int(os.environ.get('EMU_WEBTEST_TAP_AT', 95))).tap(int(os.environ.get('EMU_WEBTEST_X', 220)), int(os.environ.get('EMU_WEBTEST_Y', 112)))
    out = emulate('webtest', card, ctx.sock, s, secs=int(os.environ.get('EMU_WEBTEST_SECS', 60)))
    # two calls reached the server: the page, then the test's own; no
    # request followed either handshake
    n = open(log, 'rb').read().decode('latin-1').count('ACCEPT')
    ok = n >= 2 and not grep(log, 'GET /')
    return ok, out


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
