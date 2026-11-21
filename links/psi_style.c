/* psi_style.c - View > Page style (0.86): the page as it comes (Full), or
 * made lighter on the Psion before Links parses it, with the PsiConnect
 * Atom's simplifier (simp/, psi_simp.cpp): Lighter (scripts, styles, hidden
 * parts, SVG and tracking pixels out), Reader (menus, sidebars, banners and
 * link-only lists out too) and Text only (no pictures). Called from
 * session.c's format_html (links-2.30-style.diff) for each HTML page of
 * http and https, each time Links lays the page out; the simplified page is
 * kept for the page showing, so zooming or showing its pictures does not
 * simplify it again.
 *
 * A line or two goes at the top of a simplified page when there is
 * something to say: the page was cut short at the machine's limit (with
 * Show the rest), it looks drawn by scripts (with the reader service), or
 * its pictures were left out (with Show pictures). The page's figures go to
 * the app for Page information and the route light (psiweb.h page_*). */
#include "cfg.h"

#ifdef GRDRV_PSI

#include "links.h"
#include "pwback.h"
#include "psiweb_cmds.h"
#ifdef PSI_EPOC
#include "psiweb.h"
#include "../engine/pwatom.h"
#endif
#include "simp/psi_simp.h"

/* the simplified page of the page showing, before any note */
static struct cache_entry *k_ce;
static tcount k_gen;		/* (0.98) k_ce's count2: a new entry, or new data from the start */
static int k_style = -1;
static unsigned char *k_body;	/* (Links' mem_alloc) */
static int k_off;		/* (MEM-73) the body starts here in k_body: the notes go before it */
static int k_len;
static unsigned long k_text;
static int k_scripts;
static int k_imgs;
static int k_raw;
static int k_fed;		/* bytes of the page fed so far */
static int k_done;		/* the page was whole: the simplifier has ended */
static unsigned char k_head[64];	/* the page's first bytes: the same page? */
/* the page handed to Links: the notes and the body */
static unsigned char *k_page;

static unsigned char *g_url_rest;	/* Show the rest: this address is not cut */

/* the most simplified HTML a page keeps: the netBook has the memory */
#define PSI_STYLE_MAX_5MX	(512L * 1024)
#define PSI_STYLE_MAX_NB	(2048L * 1024)
#define PSI_SCRIPT_TEXT		200	/* less text than this, and scripts: drawn by scripts */

int pwb_style(void);
int psi_pictures_for(unsigned char *url);	/* psi_drv.c */

/* (MEM-73) Trim: the copies go (the next layout simplifies the page
   again from its cache entry), and when a page is done that is not the
   one kept */
void psi_style_trim(void)
{
	if (k_body) mem_free(k_body);
	k_body = NULL;
	k_off = 0;
	k_len = 0;
	if (k_page) mem_free(k_page);
	k_page = NULL;
	k_ce = NULL;
}

void psi_style_page_done(struct cache_entry *ce)
{
	if ((k_body || k_page) && k_ce != ce) psi_style_trim();
}

static void put(void *ctx, const char *data, unsigned long len)
{
	(void)ctx;
	add_bytes_to_str(&k_body, &k_len, (unsigned char *)data, len);
}

static int count_imgs(const unsigned char *s, int n)
{
	int i, c = 0;
	for (i = 0; i + 4 < n; i++)
		if (s[i] == '<' && (s[i + 1] | 0x20) == 'i' && (s[i + 2] | 0x20) == 'm' && (s[i + 3] | 0x20) == 'g') c++;
	return c;
}

/* an address in another's query: everything but letters and digits %-escaped */
static void add_escaped(unsigned char **s, int *l, unsigned char *u)
{
	static const char hex[] = "0123456789ABCDEF";
	for (; *u; u++) {
		unsigned char c = *u;
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_')
			add_chr_to_str(s, l, c);
		else {
			add_chr_to_str(s, l, '%');
			add_chr_to_str(s, l, hex[c >> 4]);
			add_chr_to_str(s, l, hex[c & 15]);
		}
	}
}

/* (0.87) A lighter edition of the site, offered (not forced) in a line at
   the top of its page: a site's own text or mobile edition, which suits
   Links. The address's path is kept where the edition has the same pages. */
static const struct { const char *host; const char *light; int keep_path; } k_light[] = {
	{ "www.cnn.com", "https://lite.cnn.com/", 0 },
	{ "edition.cnn.com", "https://lite.cnn.com/", 0 },
	{ "cnn.com", "https://lite.cnn.com/", 0 },
	{ "www.npr.org", "https://text.npr.org/", 0 },
	{ "npr.org", "https://text.npr.org/", 0 },
	{ "www.reddit.com", "https://old.reddit.com", 1 },
	{ "reddit.com", "https://old.reddit.com", 1 },
	{ "en.wikipedia.org", "https://en.m.wikipedia.org", 1 },
	{ "duckduckgo.com", "https://html.duckduckgo.com/html/", 0 },
	{ "www.duckduckgo.com", "https://html.duckduckgo.com/html/", 0 },
};

static int light_edition(unsigned char *url, unsigned char **s, int *l)
{
	unsigned char *h = url, *e;
	size_t hn;
	unsigned i;
	if (!casecmp(h, cast_uchar "https://", 8)) h += 8;
	else if (!casecmp(h, cast_uchar "http://", 7)) h += 7;
	else return 0;
	for (e = h; *e && *e != '/' && *e != ':' && *e != '?' && *e != '#'; e++)
		;
	hn = (size_t)(e - h);
	for (i = 0; i < sizeof(k_light) / sizeof(k_light[0]); i++) {
		if (strlen(k_light[i].host) != hn || casecmp(h, cast_uchar k_light[i].host, hn)) continue;
		add_to_str(s, l, cast_uchar "<p><small>A lighter edition of this site exists - <a href=\"");
		add_to_str(s, l, cast_uchar k_light[i].light);
		if (k_light[i].keep_path && *e == '/') {
			unsigned char *q;
			for (q = e; *q && *q != '#'; q++)
				if (*q == '"' || *q == '<' || *q == '>') break;
			if (!*q || *q == '#') add_bytes_to_str(s, l, e, (size_t)(q - e));
		}
		add_to_str(s, l, cast_uchar "\">open it</a></small></p>");
		return 1;
	}
	return 0;
}

/* Show the rest (about:cmd-rest): the page showing is laid out again, uncut */
void psi_style_rest(unsigned char *url)
{
	if (g_url_rest) mem_free(g_url_rest);
	g_url_rest = url ? stracpy(url) : NULL;
}

static void report(int style, int raw, int kept, int pics_out, int scripts, unsigned char *url)
{
#ifdef PSI_EPOC
	PwShared *s = (PwShared *)pwb_shared();
	if (!s) return;
	s->page_style = style;
	s->page_bytes = raw;
	s->page_kept = kept;
	s->page_pics_out = pics_out;
	s->page_scripts = scripts;
	if (casecmp(url, cast_uchar "https:", 6)) s->page_secure = PW_SECURE_NO;
	else if (s->use_proxy && s->proxy_host[0]) s->page_secure = PW_SECURE_HELPER;
	else if (pw_atom_tls(&s->net, get_port(url))) s->page_secure = PW_SECURE_MODEM;   /* (0.90) engine/pwatom.h */
	else s->page_secure = PW_SECURE_CHECKED;
#else
	(void)style; (void)raw; (void)kept; (void)pics_out; (void)scripts; (void)url;
#endif
}

unsigned char *psi_style_page(struct cache_entry *ce, unsigned char *start, size_t *len)
{
	int style = pwb_style(), mode, l = 0, cut = 0, pics, drawn;
	long max;
	unsigned char *url = ce ? ce->url : NULL;
	if (!ce || !url || (casecmp(url, cast_uchar "http:", 5) && casecmp(url, cast_uchar "https:", 6))) {
		if (url && casecmp(url, cast_uchar "about:", 6)) report(PW_STYLE_FULL, (int)*len, (int)*len, 0, 0, url);
		return start;
	}
	if (style <= PW_STYLE_FULL || style > PW_STYLE_TEXT || !start) {
		unsigned char *p;
		int pl = 0;
		report(PW_STYLE_FULL, (int)*len, (int)*len, 0, 0, url);
		psi_style_trim();	/* (MEM-73) a styled page's copies: not this page's */
		if (!start || ce->incomplete) return start;
		/* (0.87) Full: the page as it comes, with only the offer of a
		   lighter edition, if the site has one */
		p = init_str();
		if (!light_edition(url, &p, &pl)) {
			mem_free(p);
			return start;
		}
		add_bytes_to_str(&p, &pl, start, *len);
		k_page = p;
		k_ce = ce;
		*len = (size_t)pl;
		return k_page;
	}
	mode = style == PW_STYLE_LIGHTER ? PSI_SIMP_LIGHTER : style == PW_STYLE_READER ? PSI_SIMP_READER : PSI_SIMP_TEXT;
	/* Links lays a page out again and again while it comes: the simplifier
	   runs once over the page, a piece at a time, kept between the calls
	   (each call feeds only what came since), and ends when the page is
	   whole. A different page, style or start begins again. (0.98) A
	   page is its cache entry and the entry's count2, which Links
	   changes for a new entry (even one at the old one's address) and
	   for data that does not carry on from the end (a reload): pages
	   with the same first bytes are not taken for each other. */
	if (k_ce != ce || k_gen != ce->count2 || k_style != style || !k_body || (size_t)k_fed > *len
	    || memcmp(k_head, start, *len < sizeof(k_head) ? *len : sizeof(k_head))) {
		if (k_body) mem_free(k_body);
		k_body = init_str();
		k_off = 0;
		k_len = 0;
		k_fed = 0;
		k_done = 0;
		k_text = 0;
		k_scripts = 0;
		memset(k_head, 0, sizeof(k_head));
		memcpy(k_head, start, *len < sizeof(k_head) ? *len : sizeof(k_head));
		psi_simp_begin(mode, put, NULL);
		k_ce = ce;
		k_gen = ce->count2;
		k_style = style;
	}
	if (!k_done && (size_t)k_fed < *len) {
		psi_simp_feed(start + k_fed, (unsigned long)(*len - k_fed));
		k_fed = (int)*len;
	}
	if (!k_done && !ce->incomplete) {
		psi_simp_end(&k_text, &k_scripts);
		k_done = 1;
	}
	k_imgs = count_imgs(k_body + k_off, k_len);
	k_raw = (int)*len;
	if (k_page) mem_free(k_page), k_page = NULL;
	max = pwb_depth() == 8 ? PSI_STYLE_MAX_NB : PSI_STYLE_MAX_5MX;
	l = k_len;
	if (l > max && !(g_url_rest && !strcmp(cast_const_char g_url_rest, cast_const_char url))) {
		l = (int)max;
		while (l > 0 && k_body[k_off + l] != '<') l--;	/* at a tag */
		if (l == 0) l = (int)max;
		cut = 1;
	}
	pics = style != PW_STYLE_TEXT && k_imgs > 0 && !psi_pictures_for(url) ? k_imgs : 0;
	drawn = k_done && k_text < PSI_SCRIPT_TEXT && k_scripts > 0;
	report(style, k_raw, l, pics, drawn, url);
	{
		unsigned char *p = init_str();
		int pl = 0;
		char b[80];
		int light = light_edition(url, &p, &pl);
		if (!cut && !pics && !drawn && !light) {
			mem_free(p);
			*len = (size_t)k_len;
			return k_body + k_off;
		}
		if (drawn) {
			add_to_str(&p, &pl, cast_uchar "<p><b>This page is drawn by scripts, which PsiWeb cannot run</b><br>"
				"<a href=\"http://frogfind.com/read.php?a=");
			add_escaped(&p, &pl, url);
			add_to_str(&p, &pl, cast_uchar "\">Try the reader service (FrogFind)</a> - PsiProxy on a computer can run some scripts</p>");
		}
		if (cut) {
			snprintf(b, sizeof(b), "<p><small>Page cut short at %ld KB - ", max / 1024);
			add_to_str(&p, &pl, cast_uchar b);
			add_to_str(&p, &pl, cast_uchar "<a href=\"about:cmd-rest\">Show the rest</a></small></p>");
		}
		if (pics) {
			snprintf(b, sizeof(b), "<p><small>%d picture%s left out - ", pics, pics == 1 ? "" : "s");
			add_to_str(&p, &pl, cast_uchar b);
			add_to_str(&p, &pl, cast_uchar "<a href=\"about:cmd-pictures\">Show pictures</a></small></p>");
		}
		if (k_done) {
			/* (MEM-73) the page is whole: one copy, the notes and then
			   the whole body (Links is given only as far as the cut),
			   which stays the kept page; no second copy beside it */
			int notes = pl;
			add_bytes_to_str(&p, &pl, k_body + k_off, k_len);
			mem_free(k_body);
			k_body = p;
			k_off = notes;
			*len = (size_t)(notes + l);
			return k_body;
		}
		add_bytes_to_str(&p, &pl, k_body, l);
		k_page = p;
		*len = (size_t)pl;
		return k_page;
	}
}

#endif
