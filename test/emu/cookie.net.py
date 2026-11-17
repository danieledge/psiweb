# cookie.net.py - PsiWeb's kept cookies end to end in the 5mx emulator
# (0.89), through the emulated modem (core/tools/emu/net.py loads this
# file; asked for by name):
#   python3 core/tools/emu/net.py cookies
# The modem sends every call to test/cookie/cookiesrv.py, which sets two
# cookies on /set (psikept with a date, psisession without) and logs the
# Cookie header of every request. PsiWeb is set to Preferences > Cookies >
# Kept for next time (Ctrl+K, Down five times, Right, Enter), opens /set and
# /read, is closed (Ctrl+E) and opened again from the Extras bar, and opens
# /read: the server must see both cookies the first time and psikept alone
# after PsiWeb was closed. (The same, and "Until PsiWeb closes", in the ARM
# harness: test/cookie/run.sh.) Screenshots in build/emu/net/cookies/.

def test_cookies(ctx):
    port = free_port()
    log = os.path.join(WORK, 'cookies-server.log')
    if os.path.exists(log):
        os.remove(log)
    ctx.procs.start('cookiesrv', [sys.executable, os.path.join(REPO, 'test', 'cookie', 'cookiesrv.py'), str(port), log],
                    os.path.join(WORK, 'cookiesrv.out'))
    if not wait_port(port):
        return False, 'cookiesrv.py did not start'
    card = os.path.join(WORK, 'cookies.img')
    make_card('psiweb', card, 'EMU_PKG_WEB')
    ctx.modem('cookies', redirect='127.0.0.1:%d' % port)
    s = Script(60).key(ESC, gap=2)
    s.ctrl('k', gap=2)
    for _ in range(5):
        s.key(DOWN, gap=1)
    s.key(RIGHT, gap=2).key(ENTER, gap=30)            # (the engine starts again with the setting)
    s.ctrl('o', gap=2).type('127.0.0.1/set\r', gap=0.7).wait(15)
    s.ctrl('o', gap=2).type('127.0.0.1/read\r', gap=0.7).wait(12)
    s.ctrl('e', gap=12)
    s.tap(670, 262).wait(2).tap(335, 215).wait(25)        # Extras, then PsiWeb again
    s.ctrl('o', gap=2).type('127.0.0.1/read\r', gap=0.7)
    out = emulate('cookies', card, ctx.sock, s, secs=30)
    try:
        reads = [l for l in open(log).read().splitlines() if l.startswith('GET /read')]
    except OSError:
        reads = []
    ok = (len(reads) >= 2 and 'psikept=yes' in reads[0] and 'psisession=yes' in reads[0]
          and 'psikept=yes' in reads[-1] and 'psiage=yes' in reads[-1] and 'psisession' not in reads[-1])
    return ok, out


TESTS['cookies'] = test_cookies
