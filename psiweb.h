/* psiweb.h - memory shared between PsiWeb.app and psiweb.exe
 *
 * PsiWeb.app (the EIKON front: menus, dialogs, the screen) creates a global
 * chunk with this layout and launches psiweb.exe (Links). psiweb draws each
 * page into 'fb' and the app copies the changed rows to the screen. 'depth'
 * (the app's choice, from the screen it finds) says how:
 *   4: 16 greys, the layout of an EGray16 CFbsBitmap (the Series 5mx);
 *   8: EPOC's 256 colours, one byte a pixel, the layout of an EColor256
 *      CFbsBitmap (the netBook and other colour machines).
 * The chunk is made only as big as the screen needs (PW_SHARED_SIZE): 'fb'
 * comes last, and on the 5mx its far end is never there.
 *
 * 'net' comes first so the PsiTerm network glue (ssh/psiglue.cpp, which
 * expects a PsiShared at the start of the chunk) works unchanged: the app
 * fills in the serial/dial-up settings; the fetcher sets host/port per
 * connection.
 *
 * Every ring and field has one writer, so no locking is needed on the
 * single-CPU Psion.
 */
#ifndef PSIWEB_H
#define PSIWEB_H

#ifndef PSI_SHARED_NAME
#define PSI_SHARED_NAME "PsiWebShared"
#endif
#include "psishared.h"     /* ssh/ - PsiTerm's shared-memory layout */

#define PW_MAGIC        0x42455750u     /* 'PWEB' */
#define PW_MAX_W        640
#define PW_MAX_H        480             /* the netBook; the 5mx has 240 */
#define PW_STRIDE       (PW_MAX_W / 2)  /* bytes per 4bpp row (16 greys) */
#define PW_STRIDE8      PW_MAX_W        /* bytes per 8bpp row (256 colours) */
#define PW_EVQ          64
#define PW_URL_MAX      512

/* input events: app writes, psiweb reads */
enum
	{
	PW_EV_KEYDOWN = 1,              /* code = NSFB key code (see pwkeys) */
	PW_EV_KEYUP,
	PW_EV_PENDOWN,                  /* x, y in page-area pixels */
	PW_EV_PENUP,
	PW_EV_PENMOVE
	};

typedef struct
	{
	int type;
	int code;
	int x;
	int y;
	} PwEvent;

#include "psiweb_cmds.h"

/* psiweb state */
enum
	{
	PW_STATE_STARTING = 0,
	PW_STATE_READY,
	PW_STATE_EXITED
	};

typedef struct
	{
	PsiShared net;                  /* MUST stay first - see above */

	unsigned int magic;
	int width;                      /* page area size in pixels (app sets) */
	int height;
	volatile int state;             /* PW_STATE_* (psiweb) */
	volatile int quitting;          /* app: psiweb should exit now. (net.quit
	                                   only interrupts a dial or a download:
	                                   the app sets it for Stop too) */
	volatile int exit_code;
	char exit_msg[128];

	/* connection (app sets) */
	int use_proxy;                  /* 1 = send every request to an HTTP proxy
	                                   such as WebOne, which also does HTTPS */
	char proxy_host[64];
	int proxy_port;
	int load_images;
	int zoom;                       /* percent */
	char home_url[PW_URL_MAX];
	char res_dir[96];               /* e.g. C:\System\Apps\PsiWeb\ */
	char start_url[PW_URL_MAX];     /* first page instead of home_url, when
	                                   another app (PsiMail) opened PsiWeb */
	int start_taken;                /* psiweb: the first page has been picked */

	/* input */
	volatile unsigned int ev_head;  /* app */
	volatile unsigned int ev_tail;  /* psiweb */
	PwEvent ev[PW_EVQ];

	volatile int cmd;               /* app sets, psiweb clears when taken */
	char cmd_arg[PW_URL_MAX];

	/* output: psiweb writes, app reads */
	volatile unsigned int frame_seq;   /* bumped after each update */
	volatile int dirty_y0;             /* rows changed since the app last looked */
	volatile int dirty_y1;             /* (app resets to y0=height, y1=0) */
	volatile int busy;                 /* a page is loading */
	char status[128];                  /* status bar text (UTF-8) */
	char title[128];                   /* page title (UTF-8) */
	char url[PW_URL_MAX];              /* current URL */
	volatile int can_back;
	volatile int can_forward;
	volatile unsigned int free_ram;    /* bytes, for the status line */

	/* Update PsiWeb (engine/pwupdate.c). The app also sets net.version
	   (its own version) and net.save_as (where the .sis goes). */
	int upd_source;                    /* 0 = GitHub, 1 = PsiTerm's local server */
	char upd_host[64];
	int upd_port;
	volatile int update_state;         /* PW_UPD_* (psiweb) */
	char update_msg[128];
	char update_version[16];
	volatile unsigned int heap_used;
	volatile unsigned int app_beat;    /* app: +1 every tick; psiweb quits if it stops */

	/* Questions psiweb asks the user through the app (Links phase 5): the
	   engine fills in the question and sets the state to PW_ASK_ASKING;
	   the app shows its dialog, fills in the answer and sets PW_ASK_OK
	   or PW_ASK_CANCEL; the engine takes it and sets PW_ASK_NONE. */
	/* a page or the proxy needs a user name and password (HTTP Basic or
	   Digest authentication) */
	volatile int auth_state;           /* PW_ASK_* */
	int auth_proxy;                    /* psiweb: 1 = the proxy asks */
	char auth_host[64];                /* psiweb: who asks (UTF-8) */
	char auth_realm[96];               /* psiweb: the server's name for it */
	char auth_user[64];                /* app: the answer (UTF-8) */
	char auth_pass[64];                /* app; psiweb wipes it once used */
	/* a file PsiWeb cannot show: save it? */
	volatile int save_state;           /* PW_ASK_* */
	char save_name[64];                /* psiweb: the server's filename */
	char save_type[64];                /* psiweb: its type, e.g. application/pdf */
	int save_size;                     /* psiweb: bytes, or -1 if not known */
	int save_max;                      /* psiweb: the biggest file it saves */
	char save_path[256];               /* app: where, e.g. D:\Documents\x.pdf */

	/* display work (docs/display.md): app sets before the engine starts */
	int display;                       /* PW_DISPLAY_* bits */

	/* the page showing, for the app's scroll bar (EIKON's, beside the page)
	   and its Pictures button: psiweb writes them before each new frame.
	   Pixels; page_h 0 when there is nothing to scroll (no page, a frameset) */
	volatile int page_h;               /* the whole page's height */
	volatile int page_y;               /* the top of the view, down the page */
	volatile int page_vh;              /* the view's height */
	volatile int page_pics;            /* its pictures are shown (View > Show pictures) */

	/* Tools > Send log to server (PW_CMD_PUTLOG, engine/pwupdate.c) */
	volatile int log_state;            /* PW_LOG_* (psiweb) */
	char log_msg[96];                  /* the reason, if it failed */

	/* the screen (app sets before the engine starts): PW_DEPTH_* and the
	   bytes in a row of fb */
	int depth;
	int stride;
	volatile int progress;             /* psiweb: how far the page has come,
	                                      0 to 100; -1 when nothing loads */

	/* (0.85) the engine asks the app to act: a "Page not opened" page's
	   Test the connection and Set up PsiWeb links (PW_APP_*); the app
	   clears it when taken. problem_url: the address that did not open,
	   for the address box while that page shows */
	volatile int app_cmd;
	char problem_url[PW_URL_MAX];

	/* (0.86) the page style (app sets: PW_STYLE_*) and what came of the
	   page showing (psiweb writes them with the page): for the route
	   light and Page information */
	volatile int style;
	volatile int page_style;           /* the style the page was made in */
	volatile int page_secure;          /* PW_SECURE_* */
	volatile int page_bytes;           /* HTML as it came */
	volatile int page_kept;            /* HTML after the style's simplifier */
	volatile int page_pics_out;        /* pictures left out */
	volatile int page_scripts;         /* 1: the page looks drawn by scripts */
	volatile int keep_cookies;         /* (0.87) app sets: cookies kept between sessions */
	/* (0.87) the start page (about:start): the app makes it (speed dial,
	   recent pages, the route) and the engine serves it; field_active:
	   the engine says a text field on the page has the cursor, so keys 1
	   to 8 are typed, not tiles */
	volatile int field_active;
	char start_page[6144];
	/* (0.89) Test the connection's web part (PW_CMD_TEST): the engine
	   fetches cmd_arg, a small page, through the route in use and says
	   how it went (psiweb writes them; the app sets test_state RUNNING
	   before the command) */
	volatile int test_state;           /* PW_TEST_* */
	volatile int test_bytes;           /* the page's size */
	volatile int test_ms;              /* how long it took */
	volatile int test_kind;            /* PW_PROB_* (engine/pwproblem.h) when it failed */
	char test_why[96];                 /* the connection's or the engine's words */
	/* (0.89) the busy message's figures (psiweb writes them with each
	   frame): the host of the page coming ("" when none, or one of
	   PsiWeb's own), how much of it has come, and its pictures */
	char load_host[64];
	volatile int load_kb;
	volatile int pics_done;
	volatile int pics_all;
	/* (0.92) memory: sleep_ok is the engine's word that ending it now and
	   starting it again on the same address would lose nothing (no
	   transfer, no form filled in, no page that came from a form); waking
	   is the app's word to a starting engine that Sleep's history and
	   cookies are to be taken up (else they are deleted): links/psi_drv.c */
	volatile int sleep_ok;
	volatile int waking;
	volatile int sleeping;             /* set with quitting: the engine writes Sleep's files as it goes */
	volatile int direct_now;           /* (1.03) psiweb: "Open directly this time" - the page coming is
	                                      fetched without the web helper */
	/* (1.05) start_gen: the app counts each new start page (s->start_page),
	   so the engine drops the copy of about:start in its cache, which
	   Links would show again (psi_os.c psi_start_page_check); prefetch:
	   the app's word that pages may be fetched ahead now (psi_drv.c) */
	volatile int start_gen;
	volatile int prefetch;
	int spare[2];

	unsigned char fb[PW_MAX_H * PW_STRIDE8];   /* MUST stay last: see above */
	} PwShared;

#define PW_DEPTH_GREY16	4
#define PW_DEPTH_COLOR256	8
/* the chunk for a screen of h rows of 'stride' bytes */
#define PW_SHARED_SIZE(h, stride)	((int)((char *)((PwShared *)0)->fb - (char *)0) + (h) * (stride))

/* PwShared.display (Preferences > Text; all 0 is the standard) */
#define PW_DISPLAY_SCALED_TEXT	1	/* Links' scaled fonts, not the pre-drawn sharp ones */

#endif
