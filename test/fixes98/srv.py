#!/usr/bin/env python3
"""test/fixes98/srv.py PORT LOG - a small web server for 0.98's engine tests
(run.sh), on 127.0.0.1 only. Pages are made in memory. Every request goes to
LOG as "GET /path cookie=...".

  /cmdlink   a page whose first line is a big link to about:cmd-setup
  /redir     a redirect to about:cmd-setup
  /refresh   a refresh to about:cmd-setup
  /attr      a '<' inside an attribute value: a refresh to /evil written
             inside a title attribute (Links would show a "Refresh" line)
  /attrclean the same page with plain text in the title attribute
  /form      a form to /evilform written inside a title attribute, then a
             text box (Enter in it would send it there)
  /pic       a 16x4 PNG, black on the left half, shown 320x40
  /p.png     that picture
  /style     a page whose first 64 bytes never change, with a version that
             goes up at each request (VERSION ONE, VERSION TWO...)
  /set       a cookie with a date (psikept) and one without (psisession)
"""
import sys, time, struct, zlib
from http.server import BaseHTTPRequestHandler, HTTPServer
from email.utils import formatdate

PORT, LOG = int(sys.argv[1]), sys.argv[2]
N = {'style': 0}
WORDS = ['ZERO', 'ONE', 'TWO', 'THREE', 'FOUR']


def png():
    w, h = 16, 4
    raw = b''.join(b'\0' + bytes([0] * 8 * 3 + [255] * 8 * 3) for _ in range(h))
    def chunk(t, d):
        return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))
            + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b''))


class H(BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.0'

    def do_GET(self):
        with open(LOG, 'a') as f:
            f.write('GET %s cookie=%s\n' % (self.path, self.headers.get('Cookie', '')))
        path = self.path.split('?')[0]
        extra, code, ctype = [], 200, 'text/html; charset=iso-8859-1'
        if path == '/cmdlink':
            body = ('<html><body><p><font size="7"><a href="about:cmd-setup">SETUPSETUPSETUP</a></font></p>'
                    '<p>A web page with a link to about:cmd-setup</p></body></html>\n')
        elif path == '/redir':
            body, code = '<html><body>Moved</body></html>\n', 302
            extra = [('Location', 'about:cmd-setup')]
        elif path == '/refresh':
            body = ('<html><head><meta http-equiv="refresh" content="0; url=about:cmd-setup">'
                    '</head><body>Refreshing</body></html>\n')
        elif path in ('/attr', '/attrclean'):
            t = '<meta http-equiv=refresh content=\'0;url=/evil\'>' if path == '/attr' else 'just text'
            body = ('<html><head><title>Attr</title></head><body>'
                    '<p title="%s">A comment</p><p>End</p></body></html>\n' % t)
        elif path == '/form':
            body = ('<html><head><title>Form</title></head><body>'
                    '<span title="<form action=/evilform>">x</span>'
                    '<p><input type="text" name="q" size="40"></p></body></html>\n')
        elif path == '/pic':
            body = ('<html><body><p>Picture:</p><img src="/p.png" width="320" height="40" alt="pic">'
                    '<p>End</p></body></html>\n')
        elif path == '/p.png':
            body, ctype = png(), 'image/png'
        elif path == '/style':
            N['style'] += 1
            v = WORDS[min(N['style'], 4)]
            body = ('<html><head><meta charset="iso-8859-1"><!-- the same first 64 bytes every time -->'
                    '<title>Version %s</title></head><body><h1>VERSION %s</h1><p>%s</p></body></html>\n'
                    % (v, v, ('The text of version %s. ' % v) * 20))
            extra = [('Cache-Control', 'no-cache')]
        elif path == '/set':
            body = '<html><head><title>Cookies set</title></head><body><h1>Cookies set</h1></body></html>\n'
            extra = [('Set-Cookie', 'psikept=yes; Path=/; Expires=%s' % formatdate(time.time() + 365 * 86400, usegmt=True)),
                     ('Set-Cookie', 'psisession=yes; Path=/')]
        else:
            body, code = '<html><body><h1>Not found</h1></body></html>\n', 404
        data = body if isinstance(body, bytes) else body.encode('latin-1')
        self.send_response(code)
        self.send_header('Content-Type', ctype)
        self.send_header('Content-Length', str(len(data)))
        for k, v in extra:
            self.send_header(k, v)
        self.end_headers()
        self.wfile.write(data)

    def log_message(self, *a):
        pass


HTTPServer(('127.0.0.1', PORT), H).serve_forever()
