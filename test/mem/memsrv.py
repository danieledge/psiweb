#!/usr/bin/env python3
"""memsrv.py PORT DIR - the pages of test/mem/run.sh (PsiWeb 1.00, the memory
review), on 127.0.0.1 only:
  /pics.html   a page of pictures (mktestpage.py's, in DIR)
  /text.html   a page of text alone
  /big.html    about 400 KB of news stories
  /bomb.html   48 KB of gzip that inflates to 12 MB (Content-Encoding: gzip)
  /prog.html   one 1600x1200 progressive JPEG (prog.jpg, made here)
  /cookies/N   a page that sets 40 cookies on this host, names N-0 to N-39
  /link/N      a small page with a link to /link/N+1
"""
import gzip, io, os, sys
from http.server import BaseHTTPRequestHandler, HTTPServer
PORT, DIR = int(sys.argv[1]), sys.argv[2]

def prog_jpeg():
    path = os.path.join(DIR, 'prog.jpg')
    if not os.path.exists(path):
        from PIL import Image
        im = Image.new('RGB', (1600, 1200))
        px = im.load()
        for y in range(0, 1200, 4):
            for x in range(0, 1600, 4):
                c = ((x * 7) ^ (y * 5)) & 255, (x + y) & 255, (x * y >> 6) & 255
                for dy in range(4):
                    for dx in range(4):
                        px[x + dx, y + dy] = c
        im.save(path, 'JPEG', quality=80, progressive=True)
    return open(path, 'rb').read()

BOMB = None
def bomb():
    global BOMB
    if BOMB is None:
        line = b'<p>' + b'All work and no play makes the Psion a dull machine. ' * 4 + b'</p>\n'
        body = b'<html><body>' + line * (12 * 1024 * 1024 // len(line)) + b'</body></html>'
        BOMB = gzip.compress(body, 9)
    return BOMB

class H(BaseHTTPRequestHandler):
    def log_message(self, *a): pass
    def send(self, body, ctype='text/html', extra=()):
        self.send_response(200)
        self.send_header('Content-Type', ctype)
        self.send_header('Content-Length', str(len(body)))
        for k, v in extra: self.send_header(k, v)
        self.end_headers()
        self.wfile.write(body)
    def do_GET(self):
        p = self.path
        if p == '/text.html':
            self.send(b'<html><body><h1>Text</h1>' + b'<p>Plain words on a page of their own.</p>' * 40 + b'</body></html>')
        elif p == '/big.html':
            self.send(b'<html><body><h1>News</h1>' + b''.join(b'<div class="story"><h2>Story %d</h2><p>' % i + b'A long paragraph of the news, with words enough to fill a line or two. ' * 6 + b'</p></div>\n' for i in range(800)) + b'</body></html>')
        elif p == '/bomb.html':
            self.send(bomb(), extra=[('Content-Encoding', 'gzip')])
        elif p == '/prog.html':
            self.send(b'<html><body><p>One big progressive picture:</p><img src="/prog.jpg"></body></html>')
        elif p == '/prog.jpg':
            self.send(prog_jpeg(), 'image/jpeg')
        elif p.startswith('/cookies/'):
            n = p.split('/')[-1]
            self.send(b'<html><body><p>Cookies ' + n.encode() + b'</p></body></html>',
                      extra=[('Set-Cookie', '%s-%d=v; Max-Age=86400; Path=/' % (n, i)) for i in range(40)])
        elif p.startswith('/link/'):
            n = int(p.split('/')[-1])
            self.send(('<html><body><p>Page %d</p><a href="/link/%d">next</a></body></html>' % (n, n + 1)).encode())
        else:
            f = os.path.join(DIR, p.lstrip('/'))
            if os.path.isfile(f):
                ct = 'text/html' if f.endswith('.html') else 'image/png' if f.endswith('.png') else 'image/gif' if f.endswith('.gif') else 'image/jpeg'
                self.send(open(f, 'rb').read(), ct)
            else:
                self.send_error(404)

HTTPServer(('127.0.0.1', PORT), H).serve_forever()
