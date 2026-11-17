/* emu_back.c - pwback.h and the psiglue pg_* calls for running the Links
 * PsiWeb's ARM code in the emulator (web/links/emu/run_links.py). A copy of
 * web/emu/emu_back.c (NetSurf's), with: no start page (Links has no
 * about:welcome) and the "load pictures" setting from the harness.
 * The screen is converted with the
 * real pw_grey_convert into a 16-grey buffer the harness saves as
 * screenshots; events, commands and the network are hypercalls. */
#include "psiweb.h"
#include "fb/pwback.h"

char *strcpy(char *d, const char *s);

int emu_hc(int op, int a, int b, int c, int d);
void emu_mem_report(void);
enum {
	HB = 200, HB_OPEN = HB, HB_PRESENT, HB_EVENT, HB_CMD, HB_STATUS, HB_TITLE,
	HB_URL, HB_BUSY, HB_READY, HB_MS, HB_CONFIG, HB_UPDCFG, HB_IMAGES, HB_DEPTH, HB_STYLE, HB_COOKIES, HB_MODEMTLS,
	HP = 300, HP_DIAL = HP, HP_HANGUP, HP_AVAIL, HP_READ, HP_WRITE, HP_WAIT, HP_ENTROPY
};

static PwShared g_sh;
/* the screen: the 5mx's 640x240 in 16 greys, or (run_links.py --netbook)
   the netBook's 640x480 in EPOC's 256 colours; it goes in g_sh.fb, as on
   the Psion */
static int g_depth = -1;
int pwb_depth(void)
{
	if (g_depth < 0) g_depth = emu_hc(HB_DEPTH, 0, 0, 0, 0) == 8 ? 8 : 4;
	return g_depth;
}

/* the harness's settings (proxy, connection) go in on first use: main()
   reads use_proxy before pwb_open runs, as the real chunk is filled in by
   the app before the engine starts */
static void emu_config(void)
{
	static int done;
	if (done) return;
	done = 1;
	emu_hc(HB_CONFIG, (int)&g_sh.use_proxy, (int)g_sh.proxy_host, (int)&g_sh.proxy_port, (int)&g_sh.net.net_mode);
	g_sh.net.modem_tls = emu_hc(HB_MODEMTLS, 0, 0, 0, 0);   /* (0.90) run_links.py --atom-tls */
}
void *pwb_shared(void) { emu_config(); return &g_sh; }
/* for the harness's "auth" and "save" script lines (run_links.py): the
   chunk and the offsets of the question fields */
PwShared *emu_shared = &g_sh;
const int emu_ask_offsets[] = {
	(int)&((PwShared *)0)->auth_state, (int)&((PwShared *)0)->auth_user, (int)&((PwShared *)0)->auth_pass,
	(int)&((PwShared *)0)->save_state, (int)&((PwShared *)0)->save_path, (int)&((PwShared *)0)->auth_realm,
	(int)&((PwShared *)0)->save_name, (int)&((PwShared *)0)->save_type, (int)&((PwShared *)0)->save_size,
	(int)&((PwShared *)0)->progress,
	/* (0.89) 10..17: Test the connection's web part, the busy message's figures */
	(int)&((PwShared *)0)->test_state, (int)&((PwShared *)0)->test_bytes, (int)&((PwShared *)0)->test_ms,
	(int)&((PwShared *)0)->test_why, (int)&((PwShared *)0)->load_host, (int)&((PwShared *)0)->load_kb,
	(int)&((PwShared *)0)->pics_done, (int)&((PwShared *)0)->pics_all
};
PsiShared *pg_shared(void) { return &g_sh.net; }

int pwb_open(int *w, int *h)
{
	*w = 640; *h = pwb_depth() == 8 ? 480 : 240;
	emu_config();
	emu_hc(HB_UPDCFG, (int)&g_sh.upd_source, (int)g_sh.upd_host, (int)&g_sh.upd_port, (int)g_sh.net.version);
	strcpy(g_sh.net.save_as, "C:\\PsiWeb-update.sis");
	return emu_hc(HB_OPEN, (int)g_sh.fb, pwb_depth() == 8 ? PW_STRIDE8 : PW_STRIDE, 0, 0);
}
void pwb_close(void) { emu_mem_report(); }
void pwb_present(const unsigned short *fb, int fbw, int x0, int y0, int x1, int y1)
{
	if (pwb_depth() == 8) pw_colour_convert(fb, fbw, g_sh.fb, PW_STRIDE8, x0, y0, x1, y1);
	else pw_grey_convert(fb, fbw, g_sh.fb, PW_STRIDE, x0, y0, x1, y1);
	emu_hc(HB_PRESENT, y0, y1, 0, 0);
}
int pwb_next_event(pwb_event *ev, int timeout_ms) { return emu_hc(HB_EVENT, (int)ev, timeout_ms, 0, 0); }
int pwb_take_command(char *arg, int max) { return emu_hc(HB_CMD, (int)arg, max, 0, 0); }
void pwb_set_status(const char *s) { emu_hc(HB_STATUS, (int)s, 0, 0, 0); }
void pwb_set_title(const char *s) { emu_hc(HB_TITLE, (int)s, 0, 0, 0); }
void pwb_set_url(const char *s) { emu_hc(HB_URL, (int)s, 0, 0, 0); }
void pwb_set_busy(int b) { emu_hc(HB_BUSY, b, 0, 0, 0); if (!b) emu_mem_report(); }
void pwb_set_nav(int b, int f) { (void)b; (void)f; }
void pwb_fatal(const char *s) { emu_hc(HB_STATUS, (int)s, 0, 0, 0); }
void pwb_ready(void) { emu_hc(HB_READY, 0, 0, 0, 0); emu_mem_report(); }
const char *pwb_home_url(void) { return "about:blank"; }
const char *pwb_first_url(void) { return "about:blank"; }
const char *pwb_res_dir(void) { return "/tmp/psiwebemu/"; }
int pwb_load_images(void) { return emu_hc(HB_IMAGES, 0, 0, 0, 0); }
int pwb_style(void) { return emu_hc(HB_STYLE, 0, 0, 0, 0); }
int pwb_keep_cookies(void) { return emu_hc(HB_COOKIES, 0, 0, 0, 0); }   /* (0.89) run_links.py --keep-cookies */
long psi_save_max(void) { return 4194304L; }
int pwb_zoom(void) { return 100; }
unsigned long pwb_ms(void) { return (unsigned long)emu_hc(HB_MS, 0, 0, 0, 0); }

int pg_dial(char *why, int max) { return emu_hc(HP_DIAL, (int)g_sh.net.host, g_sh.net.port, (int)why, max); }
void pg_hangup(void) { emu_hc(HP_HANGUP, 0, 0, 0, 0); }
void pg_link_close(void) { emu_hc(HP_HANGUP, 0, 0, 0, 0); }
/* PsiWeb.log on the Psion: here, Links' stdout/stderr lines (psi_os.c) go
   on to the harness's stderr as before */
typedef struct emu_file EMU_FILE;
extern EMU_FILE *__stderr(void);
extern unsigned int fwrite(const void *, unsigned int, unsigned int, EMU_FILE *);
unsigned int strlen(const char *);
void pw_log(const char *t) { fwrite(t, 1, strlen(t), __stderr()); fwrite("\n", 1, 1, __stderr()); }
int pg_link_is_open(void) { return 0; }
int pg_link_keep(void) { return 0; }
int pg_net_closed(void) { return 0; }
void pg_link_suspect(void) { }
int pg_net_avail(void) { return emu_hc(HP_AVAIL, 0, 0, 0, 0); }
int pg_net_read(void *b, int m) { return emu_hc(HP_READ, (int)b, m, 0, 0); }
int pg_serial_write(const void *b, int n) { return emu_hc(HP_WRITE, (int)b, n, 0, 0); }
int pg_wait(int ms, int n, int k) { return emu_hc(HP_WAIT, ms, n, k, 0); }
void pg_msleep(int ms) { emu_hc(HP_WAIT, ms, 0, 0, 0); }
/* (0.81) no doorbells in the emulated engine: the input poll stays quick */
int pg_bells(void) { return 0; }
int pg_bell_wait(int ms) { pg_msleep(ms); return 0; }
void pg_ring_app(void) { }
int pg_entropy(unsigned char *o, int m) { return emu_hc(HP_ENTROPY, (int)o, m, 0, 0); }

/* Tools > Update PsiWeb: not in the emulator build */
int pw_update_run(void) { return 4; }
int pw_putlog_run(void) { return 4; }
