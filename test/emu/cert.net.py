# cert.net.py - PsiWeb 0.84's certificate check in the 5mx emulator, through
# the emulated modem (core/tools/emu/net.py loads this file; asked for by name):
#   python3 core/tools/emu/net.py certbad certbadec
# A local TLS 1.3 server (openssl s_server) with a self-signed certificate
# stands in for the site: PsiWeb must end the handshake when the certificate
# comes, send no request, and say "Certificate not trusted - localhost"
# (an infoprint, top right: see build/emu/net/certbad/). certbadec: the
# same with an ECDSA P-256 certificate. The address uses
# port 4433, not 443: the Atom modem terminates TLS itself on 443 as standard
# (AT$TLS), which would leave PsiWeb's own TLS out of it. Real sites (the
# chain checked to a root, the page loaded) are tested in the ARM harness
# (links/emu/run_pages.sh) and test/cert/run.sh.

def _cert_server(ctx, port, kind='rsa:2048'):
    d = os.path.join(WORK, 'certbad-tls')
    os.makedirs(d, exist_ok=True)
    key, crt = os.path.join(d, 'key.pem'), os.path.join(d, 'cert.pem')
    run(['openssl', 'req', '-x509', '-newkey', kind] + (['-pkeyopt', 'ec_paramgen_curve:P-256'] if kind == 'ec' else []) +
        ['-nodes', '-keyout', key, '-out', crt,
         '-days', '30', '-subj', '/CN=localhost', '-addext', 'subjectAltName=DNS:localhost'],
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    log = os.path.join(WORK, 'tls.log')
    ctx.procs.start('s_server', ['openssl', 's_server', '-www', '-tls1_3', '-accept', '127.0.0.1:%d' % port,
                                 '-key', key, '-cert', crt], log)
    return log


def test_certbad(ctx, kind='rsa:2048', name='certbad'):
    port = free_port()
    log = _cert_server(ctx, port, kind)
    if not wait_port(port):
        return False, 'openssl s_server did not start'
    card = os.path.join(WORK, name + '.img')
    make_card('psiweb', card, 'EMU_PKG_WEB')
    modem = ctx.modem(name, redirect='127.0.0.1:%d' % port)
    s = Script(60).ctrl('o').wait(1).type('https://localhost:4433/\r', gap=0.4)
    out = emulate(name, card, ctx.sock, s, secs=40)
    # the modem's call reached the server, PsiWeb's handshake went far enough
    # for the certificate to be sent, and no request followed it
    ok = grep(modem, 'connected]') and grep(log, 'ACCEPT') and not grep(log, 'GET /')
    return ok, out


TESTS['certbad'] = test_certbad
TESTS['certbadec'] = lambda ctx: test_certbad(ctx, 'ec', 'certbadec')     # the same with an ECDSA P-256 certificate
