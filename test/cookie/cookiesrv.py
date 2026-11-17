#!/usr/bin/env python3
"""test/cookie/cookiesrv.py PORT LOG - a small web server for PsiWeb's cookie
tests (0.89), on 127.0.0.1 only. Pages are made in memory, never read from
disk.

  /set      gives three cookies: psikept (Expires, a date a year away) and
            psiage (Max-Age, a year), both kept for next time when
            Preferences > Cookies says so, and psisession (no date: only
            until PsiWeb closes)
  /read     shows the cookies PsiWeb sent back
  anything  else: 404

Every request's path and Cookie header go to LOG, one line each:
"GET /read cookie=psikept=yes; psisession=yes", which the test reads.
"""
import sys, time
from http.server import BaseHTTPRequestHandler, HTTPServer
from email.utils import formatdate

PORT, LOG = int(sys.argv[1]), sys.argv[2]


class H(BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.0'

    def do_GET(self):
        cookie = self.headers.get('Cookie', '')
        with open(LOG, 'a') as f:
            f.write('GET %s cookie=%s\n' % (self.path, cookie))
        path = self.path.split('?')[0]
        if path == '/set':
            body = ('<html><head><title>Cookies set</title></head><body><h1>Cookies set</h1>'
                    '<p>psikept and psiage (kept for a year) and psisession (until PsiWeb closes).</p>'
                    '<p><a href="/read">See what PsiWeb sends back</a></p></body></html>\n')
            extra = [('Set-Cookie', 'psikept=yes; Path=/; Expires=%s' % formatdate(time.time() + 365 * 86400, usegmt=True)),
                     ('Set-Cookie', 'psiage=yes; Path=/; Max-Age=31536000'),
                     ('Set-Cookie', 'psisession=yes; Path=/')]
            code = 200
        elif path == '/read':
            shown = cookie.replace('&', '&amp;').replace('<', '&lt;') or '(none)'
            body = ('<html><head><title>Cookies read</title></head><body><h1>Cookies read</h1>'
                    '<p>PsiWeb sent: <b>%s</b></p></body></html>\n' % shown)
            extra, code = [], 200
        else:
            body, extra, code = '<html><body><h1>Not found</h1></body></html>\n', [], 404
        data = body.encode('latin-1')
        self.send_response(code)
        self.send_header('Content-Type', 'text/html; charset=iso-8859-1')
        self.send_header('Content-Length', str(len(data)))
        for k, v in extra:
            self.send_header(k, v)
        self.end_headers()
        self.wfile.write(data)

    def log_message(self, fmt, *args):
        pass


HTTPServer(('127.0.0.1', PORT), H).serve_forever()
