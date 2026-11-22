// PSIWEB.CPP - PsiWeb.app: screen, keyboard, pen, menus and settings for the
// browser engine, psiweb.exe (Links 2 from 0.62: web/links; NetSurf before).
// See psiweb.h.

#include <e32keys.h>
#include <eikkeys.h>
#include <eikscrlb.h>
#include <e32hal.h>
#include <eikchlst.h>
#include <eikedwin.h>
#include <eiklabel.h>
#include <eikmfne.h>
#include <eikcmbut.h>
#include <eikbtpan.h>
#include <txtetext.h>
#include <baclipb.h>
#include <eiktbar.h>
#include <eikimage.h>
#include <apgcli.h>
#include <apgtask.h>
#include <apacmdln.h>
#include <eikdll.h>
#include <eikproc.h>              // (0.99) the "file" name the toolbar shows
#include <eikfnlab.h>
#include <eikcfdlg.h>
#include <eikseced.h>
#include <eikon.rsg>
#include "pwapp.h"
#include "pwicons.h"
#include "psilink.h"
#include "pglinktest.h"
#include <engine/pwproblem.h>    // (0.89) PW_PROB_*: Test the connection's web part
#include "psigreyui.h"
#include "pwaddr.h"              // (0.85) an address, or words to search for
#include "psikeep.h"             // (0.95) settings kept on the card (never the cookies)
#include "psierr.h"              // (0.96) Coda's words for an error (review R105; 1.02: psion-core's ssh/psierr.h)
#include "psimem.h"              // (0.97) short of memory: PsiHub's Free memory dialog, its words and answers (core/ssh)

// The link settings, proxy included, are shared with PsiTerm and PsiMail
// (psilink.h). The proxy fields of TPwSettings are only a copy of the shared
// ones, kept for the code that shows them; the old own proxy (0.71 and
// before) is carried over once, into a PsiLink.ini that has no proxy yet.
static void UseSharedLink(RFs& aFs, TPwSettings& aSettings)
	{
	TPsiLink link;
	link.SetDefaults();
	if (!link.Load(aFs) || link.iLoadedVer < 2)
		{
		// no shared proxy yet: seed it once from PsiWeb's own (0.71)
		if (aSettings.iUseProxy || aSettings.iProxyHost.Length())
			{
			TBool had = link.iLoadedVer > 0;     // (a version 1 file: its other lines stand)
			if (!had)
				{
				link.iBaudIndex = aSettings.iBaudIndex;
				link.iRtsCts = aSettings.iRtsCts ? 1 : 0;
				link.iNetMode = aSettings.iNetMode ? 1 : 0;
				link.iPppStart = aSettings.iPppStart;
				}
			link.iUseProxy = aSettings.iUseProxy ? 1 : 0;
			link.iProxyHost = aSettings.iProxyHost;
			link.iProxyPort = aSettings.iProxyPort > 0 ? aSettings.iProxyPort : 8081;
			link.Save(aFs);
			link.iLoadedVer = 2;
			}
		else if (link.iLoadedVer == 0)
			return;                              // no file, nothing to carry over
		}
	aSettings.iBaudIndex = link.iBaudIndex;
	aSettings.iRtsCts = link.iRtsCts;
	aSettings.iNetMode = link.iNetMode;
	aSettings.iPppStart = link.iPppStart;
	aSettings.iUseProxy = link.iUseProxy;
	aSettings.iProxyHost = link.iProxyHost;
	aSettings.iProxyPort = link.iProxyPort;
	}

// (0.96) is the disk with this letter in? (the card, for a PsiWeb on it)
static TBool DiskThere(RFs& aFs, TChar aLetter)
	{
	TInt drive;
	if (RFs::CharToDrive(aLetter, drive) != KErrNone)
		return ETrue;
	TVolumeInfo v;
	return aFs.Volume(v, drive) == KErrNone;
	}

// (0.96, review R105) a line in PsiWeb.log from the app, while Tools > Log >
// Keep a log is ticked: the number behind a message that says it in words
static void AppLog(PwShared* aShared, RFs& aFs, const TDesC& aText)
	{
	if (!aShared || !aShared->net.log_on)
		return;
	RFile f;
	_LIT(KLog, "C:\\System\\Data\\PsiWeb.log");
	TInt r = f.Open(aFs, KLog, EFileWrite | EFileShareAny);
	if (r == KErrNotFound)
		r = f.Create(aFs, KLog, EFileWrite | EFileShareAny);
	if (r != KErrNone)
		return;
	TInt end = 0;
	f.Seek(ESeekEnd, end);
	TBuf8<140> line(_L8("app: "));
	line.Append(aText.Left(Min(aText.Length(), 130)));
	line.Append(_L8("\r\n"));
	f.Write(line);
	f.Close();
	}

// (0.90) "Modem provides TLS" (PsiLink.ini iModemTls, set in PsiMail's
// Connection settings or its setup assistant for the Atom modem): on the
// modem route the Atom then makes direct https connections and checks their
// certificates, and the engine speaks plain HTTP to it (engine/pwatom.h).
// Read here, not kept in TPwSettings, whose size must not change.
static TInt SharedModemTls(RFs& aFs, const TPwSettings& aSettings)
	{
	TPsiLink link;
	link.SetDefaults();
	if (!link.Load(aFs))
		return 0;
	return !aSettings.iNetMode && link.iModemTls ? 1 : 0;
	}

// Writes the link lines; the proxy in PsiLink.ini is left as it is (the
// Connection settings dialog writes that itself)
static void SaveSharedLink(RFs& aFs, const TPwSettings& aSettings)
	{
	TPsiLink link, old;
	link.SetDefaults();
	TBool had = link.Load(aFs);
	old = link;
	link.iBaudIndex = aSettings.iBaudIndex;
	link.iRtsCts = aSettings.iRtsCts ? 1 : 0;
	link.iNetMode = aSettings.iNetMode ? 1 : 0;
	link.iPppStart = aSettings.iPppStart;
	if (had && old.iBaudIndex == link.iBaudIndex && old.iRtsCts == link.iRtsCts
		&& old.iNetMode == link.iNetMode && old.iPppStart == link.iPppStart)
		return;
	link.Save(aFs);
	}

_LIT(KEngineExe, "psiweb.exe");

// Coda's debug mode (the netBook's Control panel > Diagnostics, or the
// 5mx's with PsiExtras): while C:\System\Data\PsiDiag.on is there the
// engine keeps PsiWeb.log as with Keep a log, and Coda's PsiDiag.exe copies
// its new lines into the system log. Nothing more is logged than Keep a log
// logs.
static TBool PwDiagOn()
	{
	TEntry e;
	return CEikonEnv::Static()->FsSession().Entry(_L("C:\\System\\Data\\PsiDiag.on"), e) == KErrNone;
	}
_LIT(KIniFile, "C:\\System\\Apps\\PsiWeb\\PsiWeb.ini");
_LIT(KUpdIniFile, "C:\\System\\Apps\\PsiWeb\\Update.ini");
_LIT(KFavsKept, "C:\\System\\Apps\\PsiWeb\\Favourites.dat");   // (pwfavs.cpp's KFavsFile)
_LIT(KVersion, "1.04");           // also web/pkg/psiweb.pkg and dist/PsiWeb-version.txt (two digits from 0.54: see the .pkg)
_LIT(KDefaultHome, "http://68k.news/");
const TInt KZoomSteps[] = { 50, 60, 70, 80, 90, 100, 110, 125, 150, 175, 200 };
const TInt KZoomCount = 11;
const TInt KTick = 62500;            // look for new frames 16 times a second
const TInt KTickQuiet = 2000000;     // (0.81) ...or every 2 s when nothing is going on: the doorbell brings news
const TInt KCalmTicks = 16;          // quick ticks with nothing going on before the slow pace (1 s)

static void CopyToC(char* aDst, TInt aMax, const TDesC& aSrc)
	{
	TPtr8 p((TUint8*)aDst, aMax - 1);
	p.Copy(aSrc.Left(aSrc.Length() < aMax - 1 ? aSrc.Length() : aMax - 1));
	p.ZeroTerminate();
	}

// UTF-8 from the engine -> text for the Psion's 8-bit (Windows-1252) UI
void FromUtf8(TDes& aDst, const char* aSrc)
	{
	aDst.Zero();
	const TUint8* s = (const TUint8*)aSrc;
	while (*s && aDst.Length() < aDst.MaxLength())
		{
		TUint c = *s++;
		if (c >= 0x80)
			{
			TInt extra = c >= 0xf0 ? 3 : c >= 0xe0 ? 2 : 1;
			c &= 0x3f >> extra;
			while (extra-- && (*s & 0xc0) == 0x80)
				c = (c << 6) | (*s++ & 0x3f);
			if (c == 0x2018 || c == 0x2019) c = '\'';
			else if (c == 0x201c || c == 0x201d) c = '"';
			else if (c == 0x2013 || c == 0x2014) c = '-';
			else if (c > 0xff) c = '?';
			}
		aDst.Append(TChar(c));
		}
	}

// text from the Psion's 8-bit UI (Latin-1 here) -> UTF-8 for the engine
static void ToUtf8(char* aDst, TInt aMax, const TDesC& aSrc)
	{
	TInt n = 0;
	for (TInt i = 0; i < aSrc.Length(); i++)
		{
		TUint c = aSrc[i];
		if (c < 0x80)
			{
			if (n + 1 >= aMax) break;
			aDst[n++] = (char)c;
			}
		else
			{
			if (n + 2 >= aMax) break;
			aDst[n++] = (char)(0xc0 | (c >> 6));
			aDst[n++] = (char)(0x80 | (c & 0x3f));
			}
		}
	aDst[n] = 0;
	}

// ===========================================================================
// Engine watcher
// ===========================================================================

CPwWatcher::CPwWatcher(CPwView& aView)
	: CActive(EPriorityStandard), iView(aView)
	{
	CActiveScheduler::Add(this);
	}

CPwWatcher::~CPwWatcher()
	{
	Cancel();
	}

void CPwWatcher::Watch(RProcess& aProcess)
	{
	iProcess = &aProcess;
	aProcess.Logon(iStatus);
	SetActive();
	}

void CPwWatcher::RunL()
	{
	iView.EngineEnded();
	}

void CPwWatcher::DoCancel()
	{
	if (iProcess)
		iProcess->LogonCancel(iStatus);
	}

// ===========================================================================
// View
// ===========================================================================

CPwView::~CPwView()
	{
	StartBusyCancel();
	StopEngine();
	if (iAsleep)
		{
		// closed while asleep: Sleep's history and cookies are not for the next session
		RFs& fs = iCoeEnv->FsSession();
		fs.Delete(_L("C:\\System\\Apps\\PsiWeb\\Sleep\\history.txt"));
		fs.Delete(_L("C:\\System\\Apps\\PsiWeb\\Sleep\\cookies.txt"));
		}
	delete iSBFrame;
	delete iAsker;
	delete iStarter;
	delete iTimer;
	delete iBellWaiter;                  // (before the chunk it points into goes)
	iEngRinger.Close();
	delete iWatcher;
	delete iBitmap;
	if (iChunkOpen)
		iChunk.Close();
	}

// (0.93) psiweb.exe processes but this one's own (it has none yet): the
// last PsiWeb's engine, killed by name. The kernel takes back its handles
// (the serial port, the chunk); NIFMAN's timers are given back from here.
static void KillStrayEngines()
	{
	TFindProcess find(_L("psiweb.exe*"));     // (the match ignores case: "psiweb*" would be this app too)
	TFullName name;
	TProcessId me = RProcess().Id();
	TInt killed = 0;
	while (find.Next(name) == KErrNone)
		{
		RProcess p;
		if (p.Open(find) != KErrNone)
			continue;
		if (p.Id() != me && p.ExitType() == EExitPending)
			{
			p.Kill(0);
			killed++;
			}
		p.Close();
		}
	if (killed)
		PsiLinkTimersBack();
	}

// (0.93) The chunk was there already: the last PsiWeb's engine is still on
// it (that PsiWeb crashed or was closed from the task list, under a minute
// ago: the engine stops by itself only after a minute without the app's
// heartbeat). Clearing the chunk under it, as before, let it carry on with
// this app's fresh heartbeat beside the engine started here: two engines
// on one event queue and one screen, the old one holding the serial port
// and its connection in an unknown state. So it is not taken over: it is
// asked to stop (as Close does, StopEngine), given up to 4 s to hang up,
// then killed, and this app starts with a new chunk and a new engine.
static void EndLastEngine(RChunk& aChunk)
	{
	PwShared* s = (PwShared*)aChunk.Base();
	if (aChunk.Size() >= PW_SHARED_SIZE(0, 0) && s->magic == PW_MAGIC && s->state != PW_STATE_EXITED)
		{
		s->quitting = 1;
		s->net.quit = 1;
		s->cmd = PW_CMD_QUIT;
		s->net.resized = 1;
		if (s->net.bell_magic == PSI_BELL_MAGIC)
			{
			TPsiBellRinger ringer;           // (it may be waiting on its doorbell)
			ringer.Ring(&s->net.eng_bell);
			ringer.Close();
			}
		for (TInt i = 0; i < 40 && s->state != PW_STATE_EXITED; i++)
			User::After(100000);
		}
	KillStrayEngines();                      // (gone, stopping, or stuck: none is left)
	}

void CPwView::ConstructL(const TRect& aRect, const TPwSettings& aSettings)
	{
	iSettings = aSettings;
	// the screen: 16 greys (the 5mx) or 256 colours (the netBook, and any
	// colour machine), up to 640x480; the engine draws for it (psiweb.h)
	TDisplayMode mode = iCoeEnv->ScreenDevice()->DisplayMode();
	iColour = mode == EColor256 || mode == EColor4K || mode == EColor64K || mode == EColor16M;
	iMaxH = iCoeEnv->ScreenDevice()->SizeInPixels().iHeight;
	if (iMaxH > PW_MAX_H)
		iMaxH = PW_MAX_H;
	TDisplayMode fbMode = iColour ? EColor256 : EGray16;
	TInt stride = iColour ? PW_STRIDE8 : PW_STRIDE;
	CreateBackedUpWindowL(iCoeEnv->RootWin(), fbMode);
	SetRectL(aRect);
	EnableDragEvents();
	iPics = -1;
	iLastBusy = EFalse;
	// the page's scroll bar, beside the page. (0.89) Shown only while the
	// page does not fit; its room stays the page's margin, white, so a page
	// lays out the same whether or not the bar shows (UpdateScrollBarL)
	iSBFrame = new(ELeave) CEikScrollBarFrame(this, this);
	iSBFrame->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EOn);
	LayoutL();
	iSbShown = ETrue;
	ShowScrollBar(EFalse);               // (nothing to scroll until a page says so)

	TSize size = iPageArea.Size();
	if (size.iWidth > PW_MAX_W) size.iWidth = PW_MAX_W;
	size.iWidth &= ~1;	/* the engine packs two pixels a word: width must be even */
	if (size.iHeight > iMaxH) size.iHeight = iMaxH;
	// (1.00, MEM-76) a strip of the page, not the whole screen: the frame
	// is in the shared chunk already, and the backed-up window keeps what
	// is drawn, so a whole-screen copy (307 KB on the netBook, in the
	// bitmap server) held the page a third time for nothing (BlitRows)
	iBitmap = new(ELeave) CFbsBitmap;
	User::LeaveIfError(iBitmap->Create(TSize(PW_MAX_W, KPwStripRows), fbMode));

	// (only as big as this screen needs: fb is last in the chunk)
	iChunkSize = PW_SHARED_SIZE(iMaxH, stride);
	TInt r = iChunk.CreateGlobal(_L(PSI_SHARED_NAME), iChunkSize, iChunkSize);
	if (r == KErrAlreadyExists && iChunk.OpenGlobal(_L(PSI_SHARED_NAME), EFalse) == KErrNone)
		{
		EndLastEngine(iChunk);               // (0.93) not taken over
		iChunk.Close();
		r = iChunk.CreateGlobal(_L(PSI_SHARED_NAME), iChunkSize, iChunkSize);
		}
	if (r == KErrAlreadyExists)
		{
		// still held (a process not yet cleared away): use it if it is big
		// enough for this screen (an older PsiWeb's may not be)
		r = iChunk.OpenGlobal(_L(PSI_SHARED_NAME), EFalse);
		if (r == KErrNone && iChunk.Size() < iChunkSize)
			{
			iChunk.Close();
			r = KErrInUse;
			}
		}
	User::LeaveIfError(r);
	iChunkOpen = ETrue;
	iShared = (PwShared*)iChunk.Base();
	Mem::FillZ(iShared, iChunkSize);
	iShared->width = size.iWidth;
	iShared->height = size.iHeight;
	iShared->depth = iColour ? PW_DEPTH_COLOR256 : PW_DEPTH_GREY16;
	iShared->stride = stride;

	iTimer = CPeriodic::NewL(CActive::EPriorityStandard);
	iTimer->Start(KTick, KTick, TCallBack(TickCallback, this));
	ActivateL();
	// started once the app is up, so a URL on the command line (from
	// PsiMail) can be the first page: see CPwAppUi::ProcessCommandParametersL
	iStarter = CIdle::NewL(CActive::EPriorityStandard);
	iStarter->Start(TCallBack(StartCallback, this));
	iAsker = CIdle::NewL(CActive::EPriorityStandard);
	}

TInt CPwView::StartCallback(TAny* aSelf)
	{
	CPwView* v = (CPwView*)aSelf;
	TRAPD(r, v->StartEngineL());
	(void)r;
	return 0;
	}

// a page to open: now if the engine is running, else as its first page
void CPwView::OpenUrlL(const TDesC& aUrl)
	{
	if (iRunning)
		Command(PW_CMD_OPEN, aUrl);
	else
		{
		iStartUrl = Clip(aUrl, iStartUrl.MaxLength());
		if (iAsleep)
			{
			iWaking = ETrue;                 // (0.92) asleep: the page of before becomes a step Back
			iWakeY = -1;
			}
		if (iStarter && !iStarter->IsActive())
			StartEngineL();
		}
	}

void CPwView::StartEngineL()
	{
	if (iRunning)
		return;
	PwShared* s = iShared;
	TInt w = s->width, h = s->height, depth = s->depth, stride = s->stride;
	if (iBellWaiter)
		iBellWaiter->Cancel();               // (before the bell it waits on is cleared)
	iEngRinger.Close();                      // (a new engine: a new thread)
	TickFast();                              // (and the tick follows it from the start)
	if (iBehindAt)
		{
		// (0.92) started behind another program (a link from PsiMail): its half minute and ten begin now
		iBehindAt = User::TickCount();
		if (!iBehindAt)
			iBehindAt = 1;
		iTrimmed = EFalse;
		}
	Mem::FillZ(s, PW_SHARED_SIZE(0, 0));
	s->width = w;
	s->height = h;
	s->depth = depth;
	s->stride = stride;
	s->progress = -1;
	s->magic = PW_MAGIC;
	{
	// (0.87) the start page, made again into the cleared chunk
	CPwAppUi* ui = (CPwAppUi*)iEikonEnv->EikAppUi();
	if (ui)
		{
		TRAPD(err, ui->UpdateStartPageL());
		(void)err;
		}
	}
	s->net.magic = PSI_SHARED_MAGIC;
	s->net.bell_magic = PSI_BELL_MAGIC;      // (0.81) we ring eng_bell with every key, tap, command and quit
	UseSharedLink(iCoeEnv->FsSession(), iSettings);   // may have changed in another app
	s->net.baud_index = iSettings.iBaudIndex;
	s->net.rtscts = iSettings.iRtsCts;
	s->net.net_mode = iSettings.iNetMode;
	s->net.modem_tls = SharedModemTls(iCoeEnv->FsSession(), iSettings);   // (0.90) engine/pwatom.h
	CopyToC(s->net.ppp_start, sizeof(s->net.ppp_start), iSettings.iPppStart);
	s->net.port = 80;
	CopyToC(s->net.dial_prefix, sizeof(s->net.dial_prefix), _L("ATDT"));
	CopyToC(s->net.home, sizeof(s->net.home), _L("C:\\System\\Apps\\PsiWeb"));
	// the proxy is the shared one (PsiLink.ini, just read above)
	s->use_proxy = iSettings.iUseProxy;
	CopyToC(s->proxy_host, sizeof(s->proxy_host), iSettings.iProxyHost);
	s->proxy_port = iSettings.iProxyPort;
	s->load_images = iSettings.iImages;
	s->style = StyleNow(iSettings);          // (0.86) View > Page style
	s->keep_cookies = (iSettings.iFlags & KPwFlagKeepCookies) != 0;   // (0.87)
	s->zoom = iSettings.iZoom;
	s->display = (iSettings.iDisplay & KPwDisplayScaledText) ? PW_DISPLAY_SCALED_TEXT : 0;
	s->net.log_on = (iSettings.iDisplay & KPwKeepLog) || PwDiagOn() ? 1 : 0;   // (0.77) Tools > Keep a log; the engine reads it at each line; or Coda's debug mode
	CopyToC(s->net.version, sizeof(s->net.version), KVersion);
	// updates are saved to the CF card if there is one (D:), else C:
	TVolumeInfo vol;
	iUpdateFile.Copy(iCoeEnv->FsSession().Volume(vol, EDriveD) == KErrNone
		? _L("D:\\PsiWeb-update.sis") : _L("C:\\PsiWeb-update.sis"));
	CopyToC(s->net.save_as, sizeof(s->net.save_as), iUpdateFile);
	CopyToC(s->home_url, sizeof(s->home_url), iSettings.iHome);
	CopyToC(s->start_url, sizeof(s->start_url), iStartUrl);
	s->waking = iWaking;                     // (0.92) Sleep's history and cookies: taken up, or deleted
	iWaking = EFalse;
	iAsleep = EFalse;
	iStartedWith = iSettings;                // (1.03) what this engine was given (its web helper is fixed at its start)

	// the engine and its resources (Messages, CSS) live next to the app
	TParse parse;
	parse.Set(CEikonEnv::Static()->EikAppUi()->Application()->AppFullName(), NULL, NULL);
	TFileName dir(parse.DriveAndPath());
	CopyToC(s->res_dir, sizeof(s->res_dir), dir);
	TFileName exe(dir);
	exe.Append(KEngineExe);

	iLastFrame = 0;
	iMsg1.Zero();                        // a blank page until the engine draws
	iMsg2.Zero();
	iShowMsg = ETrue;
	DrawNow();
	// Loading the engine (1.9 MB) and its start-up take some seconds, during
	// which this thread is held in Create(): say so, bottom left, and flush
	// the window server's buffer so the message and the blank page show now
	// rather than when Create() returns. Tick takes it down when the first
	// page starts loading.
	if (!iStartBusy)
		StartBusy(_L("Starting the browser engine\x85"));
	iCoeEnv->WsSession().Flush();

	TInt r = iProcess.Create(exe, KNullDesC);
	if (r != KErrNone)
		{
		StartBusyCancel();
		// (0.94) why, as far as the error says: reinstalling helps only
		// when the file is missing or damaged
		TBuf<120> e;
		if (r == KErrNoMemory)
			e = _L("Not enough memory - close other programs, then Tools > Troubleshooting > Restart browser engine");
		else if (r == KErrNotReady || r == KErrDisMounted)
			{
			// (0.96, review R103) Coda's one wording for the card
			PsiOnCardMsg(e, _L("PsiWeb"));
			e.Append(_L(", then Tools > Troubleshooting > Restart browser engine"));
			}
		else if (r == KErrInUse || r == KErrLocked)
			e = _L("The last browser engine is still closing - Tools > Troubleshooting > Restart browser engine in a moment");
		else if (exe.Length() && exe[0] != 'C' && exe[0] != 'c' && exe[0] != 'Z' && exe[0] != 'z'
			&& !DiskThere(iCoeEnv->FsSession(), exe[0]))
			{
			// (0.96, review R103) PsiWeb is on the card and the card is out:
			// Coda's one wording for it, not "reinstall"
			PsiOnCardMsg(e, _L("PsiWeb"));
			}
		else if (r == KErrNotFound || r == KErrPathNotFound)
			e = _L("Part of PsiWeb is missing - install PsiWeb again");   // (0.99: no file names, style sheet 2.13)
		else if (r == KErrCorrupt || r == KErrNotSupported)
			e = _L("Part of PsiWeb is damaged - install PsiWeb again");
		else
			PsiErrMsg(e, _L("Cannot start the browser engine"), r);   // (0.96: in words, review R105)
		{
		TBuf<60> l;
		l.Format(_L("psiweb.exe did not start (error %d)"), r);
		AppLog(iShared, iCoeEnv->FsSession(), l);
		}
		ShowMessage(_L("The browser engine did not start"), e);
		return;
		}
	iRunning = ETrue;
	iStartUrl.Zero();                  // (kept if the engine could not start)
	if (!iWatcher)
		iWatcher = new(ELeave) CPwWatcher(*this);
	iWatcher->Watch(iProcess);
	iProcess.Resume();
	}

void CPwView::StartBusy(const TDesC& aText)
	{
	if (iStartBusy)
		iEikonEnv->BusyMsgCancel();
	iStartBusy = EFalse;
	TRAPD(err, iEikonEnv->BusyMsgL(aText, EHLeftVBottom, TTimeIntervalMicroSeconds32(0)));
	iStartBusy = err == KErrNone;
	iStartBusyAt = User::TickCount();
	}

void CPwView::StartBusyCancel()
	{
	if (iStartBusy)
		iEikonEnv->BusyMsgCancel();
	iStartBusy = EFalse;
	}

// (0.89) The route in a busy message: nothing for a page fetched direct,
// "through the web helper" (0.99: one word for the helper, not the products'
// names), with the space before it
static void RouteBusyWords(const TPwSettings& aS, TDes& aOut)
	{
	if (!aS.iUseProxy || aS.iProxyHost.Length() == 0)
		aOut.Zero();
	else
		aOut.Copy(_L(" through the web helper"));
	}

// (0.89) While a page loads, the busy message (bottom left) says which page,
// by which route and how much has come: "Opening bbc.co.uk through PsiProxy
// - 64 KB...", then "Fetching pictures through PsiProxy - 12 of 30...".
// A note from the link that matters more (dialling, looking up a name, the
// modem) stays up until data comes; the figures change at most twice a second.
void CPwView::LoadBusyL()
	{
	PwShared* s = iShared;
	TBuf<100> t;
	if (s && iRunning && s->busy && !iLinkBusy)
		{
		TBuf<32> route;
		RouteBusyWords(iStartedWith, route);
		if (s->direct_now)
			route.Copy(_L(" directly"));     // (1.03) "Open directly this time"

		if (s->load_host[0])
			{
			TBuf<64> host;
			FromUtf8(host, s->load_host);
			TPtrC h = Clip(host, 36);
			t.Format(_L("Opening %S%S"), &h, &route);
			if (s->load_kb > 0)
				t.AppendFormat(_L(" - %d KB"), s->load_kb);
			t.Append(TChar(0x85));
			}
		else if (s->pics_all > 0 && s->pics_done < s->pics_all)
			t.Format(_L("Fetching pictures%S - %d of %d\x85"), &route, s->pics_done, s->pics_all);
		}
	if (s && !s->busy)
		iSaving = EFalse;                // (0.97) no transfer: no download either
	if (t.Length() == 0)
		{
		if (iLoadBusy)
			iEikonEnv->BusyMsgCancel();
		iLoadBusy = EFalse;
		iLoadShown.Zero();
		return;
		}
	if (t == iLoadShown)
		return;
	TUint now = User::TickCount();
	if (iLoadBusy && iLoadShown.Length() && now - iLoadAt < 32)
		return;                          // (the figure changes again soon)
	iEikonEnv->BusyMsgL(t, EHLeftVBottom, TTimeIntervalMicroSeconds32(0));
	iLoadBusy = ETrue;
	iLoadShown = t;
	iLoadAt = now;
	}

void CPwView::StopEngine(TBool aSleep)
	{
	if (!iRunning)
		return;
	iShared->sleeping = aSleep;          // (0.92) the engine writes its history and cookies for WakeL
	iShared->quitting = 1;
	iShared->net.quit = 1;
	iShared->cmd = PW_CMD_QUIT;
	RingEngine();
	// give the engine a few seconds to hang up and close
	for (TInt i = 0; i < 40 && iShared->state != PW_STATE_EXITED; i++)
		User::After(100000);
	if (iWatcher)
		iWatcher->Cancel();
	if (iShared->state != PW_STATE_EXITED)
		{
		iProcess.Kill(0);
		PsiLinkTimersBack();             // it never got to give NIFMAN its timers back
		}
	iProcess.Close();
	iRunning = EFalse;
	}

// (0.92) Memory. PsiWeb.app and psiweb.exe together hold some 5 MB while a
// page is open, and held it for as long as PsiWeb was open, in front or
// not. Behind another program:
//  - after half a minute the engine drops its caches and the pictures away
//    from the view, and gives the free end of its heap back (PW_CMD_TRIM);
//  - after ten minutes, if the engine says nothing would be lost (no
//    transfer or dialog, no form filled in, no page that came from a form:
//    sleep_ok), the engine is ended: its code and heap go back to the Psion.
//    It starts again when PsiWeb comes forward, or is sent an address, on
//    the page it showed, at the same place, with its Back and Forward lists
//    and its cookies (the engine's side: links/psi_drv.c).
//    (0.94) Only while the Psion is short of memory (under KSleepUnderKb
//    free): with plenty free, waking cost 2.2 MB read from the card and the
//    page fetched again for nothing.
const TUint KTrimAfterSecs = 30;
const TUint KSleepAfterSecs = 600;
const TInt KSleepUnderKb = 3072;

void CPwView::SetForeground(TBool aFront)
	{
	if (!aFront)
		{
		if (!iBehindAt)
			{
			iBehindAt = User::TickCount();
			if (!iBehindAt)
				iBehindAt = 1;
			iTrimmed = EFalse;
			}
		return;
		}
	iBehindAt = 0;
	if (iAsleep)
		{
		TRAPD(err, WakeL());
		(void)err;
		}
	else if (iRunning)
		LinkToShared();                      // (0.96, review R60) a route changed in another program
	}

// (0.96, review R60) the shared connection settings (PsiLink.ini) into the
// running engine's chunk, read again on coming to the front: the engine
// takes the route up at its next dial with no link open (psiglue's
// pg_dial), so a route changed in PsiMail, PsiTerm or Connection settings
// is used without restarting the engine. The proxy goes in the same way.
void CPwView::LinkToShared()
	{
	PwShared* s = iShared;
	if (!s)
		return;
	UseSharedLink(iCoeEnv->FsSession(), iSettings);
	s->net.baud_index = iSettings.iBaudIndex;
	s->net.rtscts = iSettings.iRtsCts;
	s->net.net_mode = iSettings.iNetMode;
	s->net.modem_tls = SharedModemTls(iCoeEnv->FsSession(), iSettings);
	CopyToC(s->net.ppp_start, sizeof(s->net.ppp_start), iSettings.iPppStart);
	s->use_proxy = iSettings.iUseProxy;
	CopyToC(s->proxy_host, sizeof(s->proxy_host), iSettings.iProxyHost);
	s->proxy_port = iSettings.iProxyPort;
	}

void CPwView::MemoryTick()
	{
	PwShared* s = iShared;
	if (!s || !iRunning || !iBehindAt || s->busy || s->state != PW_STATE_READY)
		return;
	TUint behind = (User::TickCount() - iBehindAt) / 64;
#ifdef PW_SLEEP_TEST
	// (a test build: 10 s and 25 s, for the emulator)
	const TUint trimAfter = 10, sleepAfter = 25;
#else
	const TUint trimAfter = KTrimAfterSecs, sleepAfter = KSleepAfterSecs;
#endif
	if (!iTrimmed && behind >= trimAfter)
		{
		if (s->cmd != PW_CMD_NONE)
			return;                          // (a command still waits: next tick)
		iTrimmed = ETrue;
		User::Heap().Compress();             // (1.00, MEM-73) the app's own heap too
		CopyToC(s->cmd_arg, sizeof(s->cmd_arg), KNullDesC);
		s->cmd = PW_CMD_TRIM;
		s->net.resized = 1;                  // wakes the engine's wait (see psi_os.c)
		RingEngine();
		return;
		}
	if (behind < sleepAfter || !s->sleep_ok || s->cmd != PW_CMD_NONE)
		return;
#ifndef PW_SLEEP_TEST
	{
	TMemoryInfoV1Buf mem;
	UserHal::MemoryInfo(mem);
	if (mem().iFreeRamInBytes / 1024 >= KSleepUnderKb)
		return;                          // (looked at again next tick)
	}
#endif
	// nothing of the app's own going on with the engine either
	if (iAsking || iStartBusy || iLinkBusy || iUpdDlg || iTesting || iLogState == PW_LOG_RUNNING
		|| iUpdState == PW_UPD_RUNNING || iUpdState == -1
		|| s->auth_state == PW_ASK_ASKING || s->save_state == PW_ASK_ASKING || s->app_cmd != PW_APP_NONE
		|| iEikonEnv->EikAppUi()->IsDisplayingMenuOrDialog())
		return;
	Sleep();
	}

// (1.00) a page opening, a download, an update, a test or a question: not
// to be put to sleep or closed by the memory watcher (PsiMemBusy)
TBool CPwView::MemBusy() const
	{
	PwShared* s = iShared;
	if (!s || !iRunning)
		return EFalse;
	return s->busy || iLinkBusy || iStartBusy || iUpdDlg || iUpdState == PW_UPD_RUNNING || iUpdState == -1
		|| iTesting || iLogState == PW_LOG_RUNNING || iAsking
		|| s->save_state == PW_ASK_ASKING || s->auth_state == PW_ASK_ASKING;
	}

void CPwView::Sleep()
	{
	TBuf<PW_URL_MAX> url;
	FromUtf8(url, iShared->url);
	if (!url.Length() || url.Compare(_L("about:blank")) == 0)
		return;                              // (no page: nothing to come back to)
	iWakeY = iShared->page_y;
	StopEngine(ETrue);
	if (iRunning)
		return;
	// (the doorbell's handle on the engine's thread kept the ended process's
	// data, 370 KB, until the next start: closed now, as StartEngineL does)
	if (iBellWaiter)
		iBellWaiter->Cancel();
	iEngRinger.Close();
	iStartUrl = url;
	iAsleep = ETrue;
	// (0.96, review R115) asleep, nothing comes from an engine and nothing
	// is drawn: no tick at all until it wakes (StartEngineL's TickFast
	// starts it again; coming to the front is what wakes it)
	if (iTimer)
		iTimer->Cancel();
	iTickQuiet = ETrue;
	}

// (0.97) PsiHub's Free memory dialog, "Put PsiWeb to sleep" ticked
// (psimem.h "sleepnow"): asleep now, whatever the time behind and the
// memory free (MemoryTick's ten minutes and 3 MB are for sleeping
// unasked), if nothing would be lost; else why not, in words, for the
// dialog to say ("PsiWeb cannot sleep now - a page is still opening").
// The page, Back and Forward, the place on the page and the cookies are
// kept (Sleep); PsiWeb wakes on them when it next comes to the front.
TBool CPwView::SleepNow(TDes& aWhy)
	{
	aWhy.Zero();
	if (iAsleep)
		return ETrue;                        // (asleep already)
	PwShared* s = iShared;
	if (!s || !iRunning)
		{
		aWhy.Copy(_L("its browser engine has stopped"));
		return EFalse;
		}
	TBuf<PW_URL_MAX> url;
	FromUtf8(url, s->url);
	if (!url.Length() || url.Compare(_L("about:blank")) == 0)
		aWhy.Copy(_L("no page is open"));
	else if (s->busy && iSaving)
		aWhy.Copy(_L("a download is in progress"));
	else if (s->busy)
		aWhy.Copy(_L("a page is still opening"));
	else if (iLinkBusy || iStartBusy)
		aWhy.Copy(_L("it is connecting"));
	else if (iUpdDlg || iUpdState == PW_UPD_RUNNING || iUpdState == -1)
		aWhy.Copy(_L("an update is in progress"));
	else if (iTesting || iLogState == PW_LOG_RUNNING)
		aWhy.Copy(_L("a test of the connection is running"));
	else if (s->save_state == PW_ASK_ASKING || s->auth_state == PW_ASK_ASKING || iAsking
		|| iEikonEnv->EikAppUi()->IsDisplayingMenuOrDialog())
		aWhy.Copy(_L("a dialog is open in it"));
	else if (s->app_cmd != PW_APP_NONE || s->cmd != PW_CMD_NONE || s->state != PW_STATE_READY)
		aWhy.Copy(_L("it is busy - try again in a moment"));
	else if (!s->sleep_ok)
		aWhy.Copy(_L("a form on the page would be lost"));
	if (aWhy.Length())
		{
		TBuf<100> l;
		l.Format(_L("memory: PsiHub asked for sleep - not now, %S"), &aWhy);
		AppLog(s, iCoeEnv->FsSession(), l);
		return EFalse;
		}
	AppLog(s, iCoeEnv->FsSession(), _L("memory: PsiHub asked for sleep - the engine is stopped"));
	Sleep();
	if (!iAsleep)
		{
		aWhy.Copy(_L("its browser engine did not stop"));
		return EFalse;
		}
	return ETrue;
	}

// (0.97) psimem.h "trim" (PsiHub, under 3 MB free; the dialog's "Clear
// PsiWeb's stored pages"): the engine drops its caches and the pictures
// away from the view and gives back its heap's free end, as after half a
// minute behind; the app gives back its own
void CPwView::TrimNow()
	{
	PwShared* s = iShared;
	User::Heap().Compress();
	if (!s || !iRunning || s->cmd != PW_CMD_NONE)
		return;
	CopyToC(s->cmd_arg, sizeof(s->cmd_arg), KNullDesC);
	s->cmd = PW_CMD_TRIM;
	s->net.resized = 1;                  // wakes the engine's wait (see psi_os.c)
	RingEngine();
	}

// The engine again, on the page it slept on (iStartUrl; OpenUrlL may have
// put another address there: then the page of before is one step Back)
void CPwView::WakeL()
	{
	if (!iAsleep || iRunning)
		return;
	iWaking = ETrue;
	StartBusy(_L("Opening the page again\x85"));
	StartEngineL();
	}

// (1.03) The web helper changed (Set up PsiWeb, Connection settings): the
// running engine takes it for its next request (PW_CMD_ROUTE), as it would
// have at its start; nothing is restarted or hung up
void CPwView::HelperChangedL()
	{
	LinkToShared();
	iStartedWith.iUseProxy = iSettings.iUseProxy;
	iStartedWith.iProxyHost = iSettings.iProxyHost;
	iStartedWith.iProxyPort = iSettings.iProxyPort;
	if (iRunning)
		Command(PW_CMD_ROUTE, KNullDesC);
	}

// Stops the engine and starts it again, on the page it was showing if asked
// (settings that the engine reads at start, and the toolbar's room)
void CPwView::RestartL(TBool aSamePage)
	{
	TBuf<PW_URL_MAX> url;
	if (aSamePage && iRunning)
		{
		FromUtf8(url, iShared->url);
		// (1.03) a "Page not opened" page is about the address that did
		// not open: that is the page to try again (about:problem itself is
		// only the engine's, and a new engine has nothing to show for it:
		// it was a blank page, an empty address and nothing to reload)
		if (url.Compare(_L("about:problem")) == 0)
			FromUtf8(url, iShared->problem_url);
		if (url.Compare(_L("about:blank")) == 0 || Clip(url, 10).Compare(_L("about:cmd-")) == 0)
			url.Zero();
		}
	iRestarting = ETrue;
	StartBusy(_L("Restarting the browser engine\x85"));
	StopEngine();
	if (url.Length())
		iStartUrl = url;
	TRAPD(err, StartEngineL());
	iRestarting = EFalse;
	User::LeaveIfError(err);
	}

// The page area changed (the toolbar or toolband was shown or hidden).
// (0.87) The engine lays the page out again at the new size, on the same
// page and place (PW_CMD_RESIZE): no restart, no blank page. It was given
// room for the whole screen when it started.
void CPwView::SetPageRectL(const TRect& aRect)
	{
	SetRectL(aRect);
	LayoutL();
	TSize size = iPageArea.Size();
	if (size.iWidth > PW_MAX_W) size.iWidth = PW_MAX_W;
	size.iWidth &= ~1;	/* the engine packs two pixels a word: width must be even */
	if (size.iHeight > iMaxH) size.iHeight = iMaxH;
	iShared->width = size.iWidth;
	iShared->height = size.iHeight;
	if (iRunning)
		{
		TBuf<24> arg;
		arg.Format(_L("%dx%d"), size.iWidth, size.iHeight);
		Command(PW_CMD_RESIZE, arg);
		}
	DrawNow();
	}

// ----- the page's scroll bar --------------------------------------------------------

// The bar takes its breadth from the right of Rect(); the engine draws in
// what is left (iPageArea, at the window's origin)
void CPwView::LayoutL()
	{
	TEikScrollBarModel hModel(0, 0, 0);
	TEikScrollBarModel vModel(iSbH, iSbH > 0 ? iSbVh : 0, iSbH > 0 ? iSbY : 0);
	TRect inclusive(Rect());
	TRect client(Rect());
	TEikScrollBarFrameLayout layout;
	layout.iTilingMode = TEikScrollBarFrameLayout::EInclusiveRectConstant;
	iSBFrame->TileL(&hModel, &vModel, client, inclusive, layout);
	iPageArea = client;
	CEikScrollBar* sb = iSBFrame->GetScrollBarHandle(CEikScrollBar::EVertical);
	if (sb && !iSbShown)
		sb->MakeVisible(EFalse);         // (0.89) laid out again, still not shown
	}

TInt CPwView::CountComponentControls() const
	{
	return iSBFrame ? iSBFrame->CountComponentControls() : 0;
	}

CCoeControl* CPwView::ComponentControl(TInt aIndex) const
	{
	return iSBFrame->ComponentControl(aIndex);
	}

// The engine has said where the page is (psiweb.h page_*): the thumb goes
// there, unless the pen is dragging it. Only a change from the engine moves
// it, so a tap on the bar is not undone while the engine catches up.
void CPwView::UpdateScrollBarL()
	{
	PwShared* s = iShared;
	TInt h = 0, y = 0, vh = 0;
	if (iAsleep)
		return;                              // (0.92) asleep: the page and its bar stay as they were
	if (s && iRunning && !iShowMsg && s->page_h > 0 && s->page_vh > 0)
		{
		h = s->page_h;
		y = s->page_y;
		vh = s->page_vh;
		}
	if (iSbDragging || (h == iEngH && y == iEngY && vh == iEngVh))
		return;
	iEngH = h;
	iEngY = y;
	iEngVh = vh;
	if (h == iSbH && y == iSbY && vh == iSbVh)
		return;
	iSbH = h;
	iSbY = y;
	iSbVh = vh;
	CEikScrollBar* sb = iSBFrame->GetScrollBarHandle(CEikScrollBar::EVertical);
	if (!sb)
		return;
	TEikScrollBarModel model(h, h > 0 ? vh : 0, h > 0 ? y : 0);
	sb->SetModelL(&model);
	if (h > vh && vh > 0)
		{
		ShowScrollBar(ETrue);
		sb->DrawNow();
		}
	else
		ShowScrollBar(EFalse);           // (0.89) the page fits, or there is none
	}

// (0.89) The bar only while there is something to scroll; without it, its
// room is drawn white (Draw)
void CPwView::ShowScrollBar(TBool aShow)
	{
	if (!iSBFrame || aShow == iSbShown)
		return;
	CEikScrollBar* sb = iSBFrame->GetScrollBarHandle(CEikScrollBar::EVertical);
	if (!sb)
		return;
	iSbShown = aShow;
	sb->MakeVisible(aShow);
	if (!aShow && IsReadyToDraw())
		{
		TRect strip(Rect());
		strip.iTl.iX = iPageArea.iBr.iX;
		if (!strip.IsEmpty())
			{
			// (0.92) drawn here: the view's window is backed up, which has
			// no redraw to ask for. Invalidate() is a request such a window
			// does not know, and the window server closed PsiWeb for it
			// (WSERV 1) when the bar went
			ActivateGc();
			Draw(strip);
			DeactivateGc();
			}
		}
	}

// The pen on the bar: the arrows move a few lines, the shaft a screen (as
// Fn+Up and Fn+Down), the thumb where it is put
void CPwView::HandleScrollEventL(CEikScrollBar* aScrollBar, TEikScrollEvent aEventType)
	{
	if (!iRunning || iSbH <= 0 || iUpdState == PW_UPD_RUNNING)
		return;
	TInt line = iSbVh / 8 < 16 ? 16 : iSbVh / 8;
	TInt y = iSbY;
	switch (aEventType)
		{
	case EEikScrollUp:
		y -= line;
		break;
	case EEikScrollDown:
		y += line;
		break;
	case EEikScrollPageUp:
		y -= iSbVh - line;
		break;
	case EEikScrollPageDown:
		y += iSbVh - line;
		break;
	case EEikScrollTop:
	case EEikScrollHome:
		y = 0;
		break;
	case EEikScrollBottom:
	case EEikScrollEnd:
		y = iSbH;
		break;
	case EEikScrollThumbDragVert:
		iSbDragging = ETrue;
		y = aScrollBar->ThumbPosition();
		break;
	case EEikScrollThumbReleaseVert:
		iSbDragging = EFalse;
		y = aScrollBar->ThumbPosition();
		break;
	default:
		return;
		}
	ScrollTo(y);
	}

void CPwView::ScrollTo(TInt aY)
	{
	if (aY > iSbH - iSbVh) aY = iSbH - iSbVh;
	if (aY < 0) aY = 0;
	if (aY == iSbY && !iSbDragging)
		return;
	iSbY = aY;
	CEikScrollBar* sb = iSBFrame->GetScrollBarHandle(CEikScrollBar::EVertical);
	if (sb && !iSbDragging)
		{
		sb->SetModelThumbPosition(aY);
		sb->DrawNow();
		}
	TBuf<12> arg;
	arg.Num(aY);
	Command(PW_CMD_SCROLL, arg);
	}

// the page's pictures are showing: on every page (Preferences), or asked
// for this one (View > Show pictures, the Pictures button)
TBool CPwView::PicturesShown() const
	{
	return iSettings.iImages || (iRunning && iShared && iShared->page_pics);
	}

void CPwView::EngineEnded()
	{
	TExitType type = iProcess.ExitType();
	TInt reason = iProcess.ExitReason();
	TExitCategoryName cat = iProcess.ExitCategory();
	iProcess.Close();
	iRunning = EFalse;
	StartBusyCancel();
	if (iLogState == PW_LOG_RUNNING)
		{
		// the engine went while sending the log
		iLogState = PW_LOG_IDLE;
		iEikonEnv->BusyMsgCancel();
		iEikonEnv->InfoMsg(_L("Cannot send the log - the browser engine stopped"));
		}
	if (iUpdState == PW_UPD_RUNNING || iUpdState == -1)
		{
		// the engine went while updating: the progress window must not wait for it
		iUpdState = PW_UPD_FAILED;
		iUpdMsg = _L("The browser engine stopped");
		UpdateLine(iUpdMsg);
		if (iUpdDlg)
			{
			TRAPD(fe, iUpdDlg->FinishL(iUpdLog));
			(void)fe;
			}
		}
	if (iShared->quitting)
		return;
	TBuf<120> why;
	if (type == EExitPanic)
		PsiLinkTimersBack();             // (0.54) the crash skipped the engine's own clean-up
	// (0.96, review R105) in words; the panic or the number to the log
	{
	TBuf<60> l;
	if (type == EExitPanic)
		l.Format(_L("the engine stopped: %S %d"), &cat, reason);
	else
		l.Format(_L("the engine closed (%d)"), reason);
	AppLog(iShared, iCoeEnv->FsSession(), l);
	}
	if (type == EExitPanic)
		why = _L("It stopped unexpectedly - Tools > Troubleshooting > Restart browser engine starts it again");
	else if (iShared->exit_msg[0])
		{
		FromUtf8(why, iShared->exit_msg);
		}
	else
		why = _L("It closed - Tools > Troubleshooting > Restart browser engine starts it again");
	ShowMessage(_L("The browser engine has stopped"), why);
	}

void CPwView::ShowMessage(const TDesC& aLine1, const TDesC& aLine2)
	{
	iMsg1 = aLine1.Left(aLine1.Length() < 80 ? aLine1.Length() : 80);
	iMsg2 = aLine2.Left(aLine2.Length() < 120 ? aLine2.Length() : 120);
	iShowMsg = ETrue;
	DrawNow();
	}

void CPwView::Command(TInt aCmd, const TDesC& aArg)
	{
	if (!iRunning && iAsleep)
		{
		// (0.92) asleep: the engine first, on its page (the command is not kept)
		TRAPD(err, WakeL());
		(void)err;
		return;
		}
	if (!iRunning)
		{
		iEikonEnv->InfoMsg(_L("Not available - the browser engine has stopped"));
		return;
		}
	CopyToC(iShared->cmd_arg, sizeof(iShared->cmd_arg), aArg);
	if (aCmd == PW_CMD_STOP)
		iShared->net.quit = 1;          // also interrupts a dial or download
	iShared->cmd = aCmd;
	iShared->net.resized = 1;            // wakes the engine's wait (see psi_os.c)
	RingEngine();
	TickFast();
	}

// (0.81) The engine waits on a doorbell when nothing is happening
// (psibell.h): every key, tap, command, answer and quit rings it.
void CPwView::RingEngine()
	{
	if (iRunning && iShared)
		iEngRinger.Ring(&iShared->net.eng_bell);
	}

// Is anything going on that the tick must follow closely? A page loading,
// frames coming (within the last second), input not yet taken, a message
// about the link, a question, an update, the start-up message.
TBool CPwView::TickWanted() const
	{
	PwShared* s = iShared;
	if (!s || iStartBusy || iLinkBusy || iAsking || iUpdState == PW_UPD_RUNNING || iLogState == PW_LOG_RUNNING)
		return ETrue;
	if (!iRunning)
		return EFalse;                       // (the engine has stopped: nothing to follow)
	if (s->busy || s->state != PW_STATE_READY || s->ev_tail != s->ev_head || s->cmd != PW_CMD_NONE)
		return ETrue;
	if (s->frame_seq != iLastFrame || s->net.link_seq != iLinkSeq || s->update_state != iUpdState || s->log_state != iLogState)
		return ETrue;
	if (s->auth_state == PW_ASK_ASKING || s->save_state == PW_ASK_ASKING)
		return ETrue;
	if (User::TickCount() - iFrameAt < 64)
		return ETrue;
	return EFalse;
	}

void CPwView::TickFast()
	{
	iCalmTicks = 0;
	if (!iTickQuiet || !iTimer)
		return;
	iTickQuiet = EFalse;
	iTimer->Cancel();
	iTimer->Start(KTick, KTick, TCallBack(TickCallback, this));
	}

void CPwView::TickQuiet()
	{
	iCalmTicks = 0;
	if (iTickQuiet || !iTimer || !iShared)
		return;
	if (!iBellWaiter)
		iBellWaiter = new CPsiBellWaiter(TCallBack(BellCallback, this));   // (no memory: stays quick)
	if (!iBellWaiter || iBellWaiter->Arm(&iShared->net.app_bell) || !iBellWaiter->IsActive())
		return;                              // (rung just now, or no doorbell: stays quick)
	iTickQuiet = ETrue;
	iTimer->Cancel();
	iTimer->Start(KTickQuiet, KTickQuiet, TCallBack(TickCallback, this));
	}

TInt CPwView::BellCallback(TAny* aSelf)
	{
	CPwView* self = (CPwView*)aSelf;
	self->TickFast();
	self->Tick();
	return 0;
	}

TInt CPwView::TickCallback(TAny* aSelf)
	{
	((CPwView*)aSelf)->Tick();
	return 1;
	}

// copies the rows the engine has redrawn into our bitmap
void CPwView::Tick()
	{
	PwShared* s = iShared;
	if (s)
		s->app_beat++;                   // "still here": see pwepoc.cpp
	MemoryTick();                        // (0.92) behind another program: trim, then sleep
	((CPwAppUi*)iEikonEnv->EikAppUi())->SetMemBusy(MemBusy());   // (1.00) for the memory watcher
	// (0.81) the pace: quick while anything is going on, else slow with the
	// doorbell armed (the heartbeat above still goes every 2 s)
	if (TickWanted())
		TickFast();
	else if (!iTickQuiet && ++iCalmTicks >= KCalmTicks)
		TickQuiet();
	// "Starting the browser engine...": down once the engine starts loading
	// its first page (its window is up by then; its first frame is only the
	// blank background, seconds before that), or after a minute regardless
	if (s && iStartBusy && iRunning && (s->busy || User::TickCount() - iStartBusyAt > 64 * 60))
		StartBusyCancel();
	if (s && s->net.link_seq != iLinkSeq)
		{
		// what the connection is doing (dialling, looking up, connecting):
		// the engine is busy then and can't draw its own status line. A
		// note that ends in "..." is a busy message, bottom left, until the
		// page has loaded; anything else is an infoprint, top right
		iLinkSeq = s->net.link_seq;
		TBuf<80> m;
		FromUtf8(m, s->net.link_msg);
		m.Trim();
		// (Right(3) of a shorter text panics USER 22: a one- or two-letter
		// note, e.g. after NO CARRIER, closed PsiWeb up to 0.56)
		TBool dots = m.Length() >= 3 && m.Right(3).Compare(_L("...")) == 0;
		if (dots)
			m.Replace(m.Length() - 3, 3, _L("\x85"));   // (0.99) the ellipsis is one character
		else if (m.Length() && m[m.Length() - 1] == 0x85)
			dots = ETrue;
		if (m.Length() && m[m.Length() - 1] == '.' && !dots)
			m.SetLength(m.Length() - 1);     // no full stop at the end of a message
		// (0.89) the link's routine notes for each connection ("Connecting
		// to 10.0.0.5:8080...", "Request sent...") are left out while a page
		// loads, or the connection is tested: the busy message says which page
		// and by which route instead (LoadBusyL). Dialling, looking up and
		// the modem still say what they do.
		TBool routine = m.Find(_L("Connecting to ")) == 0 || m.Find(_L("TCP connection open")) == 0
			|| m.Find(_L("Request sent")) == 0 || m.Find(_L("Receiving the reply")) == 0
			|| m.Find(_L("Using the address looked up")) == 0;
		if (routine && !iUpdDlg && (iTesting || (s->busy && s->load_host[0])))
			{
			if (iLinkBusy && !iTesting)
				iLoadShown.Zero();           // the route's message goes up again over the old note
			iLinkBusy = EFalse;
			}
		else if (iUpdDlg)
			{
			// updating: the progress window shows each step of getting online
			if (m.Length())
				{
				UpdateLine(m);
				TRAPD(ue, iUpdDlg->ShowL(iUpdLog));
				(void)ue;
				}
			}
		else if (dots)
			{
			TRAPD(err, iEikonEnv->BusyMsgL(m, EHLeftVBottom, TTimeIntervalMicroSeconds32(0)));
			iLinkBusy = err == KErrNone;
			iLoadBusy = EFalse;              // (its busy message has gone under this one)
			iLoadShown.Zero();
			}
		else if (m.Length())
			{
			// a plain note after a busy one ("Checking the modem..." then
			// "CONNECT 115200"): that step is over
			if (iLinkBusy)
				{
				iEikonEnv->BusyMsgCancel();
				iLinkBusy = EFalse;
				}
			iEikonEnv->InfoMsg(m);
			}
		}
	if (s && (iLogState == PW_LOG_RUNNING || s->log_state != iLogState))
		LogTick();
	if (s && iLinkBusy && !s->busy && iLogState != PW_LOG_RUNNING)
		{
		iEikonEnv->BusyMsgCancel();
		iLinkBusy = EFalse;
		}
	if (s && !iUpdDlg && !iTesting && iLogState != PW_LOG_RUNNING)
		{
		TRAPD(lerr, LoadBusyL());
		(void)lerr;
		}
	// the engine asks something (a password, where to save a file): the
	// dialog is shown from an idle callback, not inside this tick
	if (s && !iAsking && iAsker && !iAsker->IsActive()
		&& (s->auth_state == PW_ASK_ASKING || s->save_state == PW_ASK_ASKING || s->app_cmd != PW_APP_NONE))
		iAsker->Start(TCallBack(AskCallback, this));
	if (s && (iUpdState == PW_UPD_RUNNING || s->update_state != iUpdState))
		{
		TRAPD(err, UpdateTickL());
		(void)err;
		}
	// (0.69) a page has loaded when the engine stops being busy: History
	if (s && iRunning)
		{
		TBool busy = s->busy != 0;
		CPwAppUi* ui = (CPwAppUi*)iEikonEnv->EikAppUi();
		if (iLastBusy && !busy)
			{
			ui->PageLoaded();
			TRAPD(oe, ui->StyleOfferL());    // (1.03) once: Text only, for big pages without PsiProxy
			(void)oe;
			if (iWakeY > 0)
				{
				// (0.92) woken: back to the place it slept at
				TBuf<12> y;
				y.Num(iWakeY);
				iWakeY = -1;
				Command(PW_CMD_SCROLL, y);
				}
			}
		else if (!busy)
			ui->PageTick();
		iLastBusy = busy;
		}
	((CPwAppUi*)iEikonEnv->EikAppUi())->ChromeTick();   // (0.78) the toolband's address and progress
	if (s && iRunning && s->page_pics != iPics)
		{
		iPics = s->page_pics;
		((CPwAppUi*)iEikonEnv->EikAppUi())->ShowPicturesState(PicturesShown());
		}
	if (!s || s->frame_seq == iLastFrame || iUpdState == PW_UPD_RUNNING)
		{
		TRAPD(se, UpdateScrollBarL());
		(void)se;
		return;
		}
	iLastFrame = s->frame_seq;
	iFrameAt = User::TickCount();
	TInt y0 = s->dirty_y0, y1 = s->dirty_y1;
	s->dirty_y0 = s->height;
	s->dirty_y1 = 0;
	if (y0 < 0) y0 = 0;
	if (y1 > s->height) y1 = s->height;
	if (y0 >= y1)
		{
		// the engine widened the band just as we reset it: copy it all
		y0 = 0;
		y1 = s->height;
		}
	iShowMsg = EFalse;
	// a backed-up window: draw straight in, no redraw cycle
	ActivateGc();
	BlitRows(SystemGc(), TRect(0, y0, s->width, y1));
	DeactivateGc();
	TRAPD(se, UpdateScrollBarL());
	(void)se;
	}

// ----- the engine's questions (Links phase 5) --------------------------------------

TInt CPwView::AskCallback(TAny* aSelf)
	{
	CPwView* v = (CPwView*)aSelf;
	PwShared* s = v->iShared;
	if (!s || v->iAsking)
		return 0;
	v->iAsking = ETrue;
	if (v->iLinkBusy)
		{
		CEikonEnv::Static()->BusyMsgCancel();
		v->iLinkBusy = EFalse;
		}
	if (s->auth_state == PW_ASK_ASKING)
		{
		TRAPD(err, v->AskAuthL());
		if (err != KErrNone && s->auth_state == PW_ASK_ASKING)
			s->auth_state = PW_ASK_CANCEL;
		}
	if (s->save_state == PW_ASK_ASKING)
		{
		TRAPD(err, v->AskSaveL());
		if (err != KErrNone && s->save_state == PW_ASK_ASKING)
			s->save_state = PW_ASK_CANCEL;
		}
	// (0.85) a "Page not opened" page's Test the connection or Set up
	// PsiWeb link, (0.98) and the start page's What works and Add...
	// (psiweb_cmds.h PW_APP_*)
	TInt cmd = s->app_cmd;
	s->app_cmd = PW_APP_NONE;
	if (cmd == PW_APP_TEST || cmd == PW_APP_SETUP || cmd == PW_APP_HELP || cmd == PW_APP_FAVS)
		{
		CPwAppUi* ui = (CPwAppUi*)CEikonEnv::Static()->EikAppUi();
		TRAPD(err, ui->EngineCommandL(cmd));
		(void)err;
		}
	s->net.resized = 1;                  // wakes the engine's wait (psi_os.c)
	v->RingEngine();
	v->iAsking = EFalse;
	return 0;
	}

// User name and password: who asks, and the server's name for it
void CPwView::AskAuthL()
	{
	PwShared* s = iShared;
	TBuf<64> host;
	TBuf<96> realm;
	FromUtf8(host, s->auth_host);
	FromUtf8(realm, s->auth_realm);
	TBuf<100> who;
	TPtrC h = Clip(host, 60);
	if (s->auth_proxy)
		who.Format(_L("The web helper %S needs a password"), &h);
	else
		who.Format(_L("%S needs a password"), &h);
	TBuf<100> what;
	if (realm.Length() && realm.Compare(host))
		{
		TPtrC r = Clip(realm, 90);
		what.Format(_L("\"%S\""), &r);
		}
	TBuf<60> user;
	TBuf<CEikSecretEditor::EMaxSecEdLength> pass;
	CPwAuthDialog* dlg = new(ELeave) CPwAuthDialog(who, what, user, pass);
	if (dlg->ExecuteLD(R_PW_AUTH_DIALOG))
		{
		ToUtf8(s->auth_user, sizeof(s->auth_user), user);
		ToUtf8(s->auth_pass, sizeof(s->auth_pass), pass);
		s->auth_state = PW_ASK_OK;
		}
	else
		s->auth_state = PW_ASK_CANCEL;
	pass.FillZ();
	}

void CPwAuthDialog::PreLayoutDynInitL()
	{
	((CEikLabel*)Control(EPwDlgAuthWho))->SetTextL(Clip(iWho, 60));
	if (iRealm.Length())
		((CEikLabel*)Control(EPwDlgAuthRealm))->SetTextL(Clip(iRealm, 70));
	else
		MakeLineVisible(EPwDlgAuthRealm, EFalse);
	}

TBool CPwAuthDialog::OkToExitL(TInt /*aButtonId*/)
	{
	((CEikEdwin*)Control(EPwDlgAuthUser))->GetText(iUser);
	iUser.Trim();
	if (iUser.Length() == 0)
		{
		TryChangeFocusToL(EPwDlgAuthUser);
		iEikonEnv->InfoMsg(_L("No user name entered"));
		return EFalse;
		}
	((CEikSecretEditor*)Control(EPwDlgAuthPass))->GetText(iPass);
	return ETrue;
	}

// A file PsiWeb cannot show: say what it is, then the standard Save as
// dialog (on the Memory disk if there is one). The engine saves it, with
// its progress as the busy message and "Saved" at the end.
void CPwView::AskSaveL()
	{
	PwShared* s = iShared;
	TBuf<64> name, type;
	FromUtf8(name, s->save_name);
	FromUtf8(type, s->save_type);
	for (TInt i = 0; i < name.Length(); i++)
		{
		TChar c = name[i];
		if (c < ' ' || c == '\\' || c == '/' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
			name[i] = '_';
		}
	name.Trim();
	if (name.Length() == 0)
		name = _L("Download");
	TBuf<120> what;
	TPtrC n = Clip(name, 36), t = Clip(type, 30);
	if (s->save_size >= 0)
		what.Format(_L("PsiWeb cannot show \"%S\" (%S, %d KB)"), &n, &t, (s->save_size + 1023) / 1024);
	else
		what.Format(_L("PsiWeb cannot show \"%S\" (%S)"), &n, &t);
	if (!iEikonEnv->QueryWinL(what, _L("Save it to a file?")))
		{
		s->save_state = PW_ASK_CANCEL;
		return;
		}
	RFs& fs = iCoeEnv->FsSession();
	TDriveInfo di;
	TBool memDisk = fs.Drive(di, EDriveD) == KErrNone && di.iType != EMediaNotPresent;
	TFileName path;
	path = memDisk ? _L("D:\\Documents\\") : _L("C:\\Documents\\");
	fs.MkDirAll(path);
	path.Append(name);
	TBuf<40> title(_L("Save to file"));
	CEikFileSaveAsDialog* dlg = new(ELeave) CEikFileSaveAsDialog(&path, &title, NULL, EFalse);
	if (!dlg->ExecuteLD(R_EIK_DIALOG_FILE_SAVEAS))
		{
		s->save_state = PW_ASK_CANCEL;
		return;
		}
	// room for it? (if the size is not known, the engine finds out)
	TInt drive;
	TVolumeInfo vol;
	if (s->save_size >= 0 && RFs::CharToDrive(path[0], drive) == KErrNone && fs.Volume(vol, drive) == KErrNone
		&& vol.iFree < TInt64(s->save_size) + TInt64(16384))
		{
		TBuf<80> m;
		m.Format(_L("Not enough room on the disk - the file is %d KB"), (s->save_size + 1023) / 1024);
		iEikonEnv->InfoMsg(m);
		s->save_state = PW_ASK_CANCEL;
		return;
		}
	fs.MkDirAll(TParsePtrC(path).DriveAndPath());
	ToUtf8(s->save_path, sizeof(s->save_path), path);
	s->save_state = PW_ASK_OK;
	iSaving = ETrue;                     // (0.97) the Free memory dialog says "a download is in progress"
	}

// ----- Send log to helper -------------------------------------------------------
// Tools > Troubleshooting > Send log to helper: the engine POSTs PsiWeb.old and PsiWeb.log to
// the proxy (pwupdate.c pw_putlog_run). The busy message is bottom left, the
// outcome an infoprint.

void CPwView::SendLog()
	{
	if (iLogState == PW_LOG_RUNNING)
		{
		iEikonEnv->InfoMsg(_L("Not available while sending the log"));
		return;
		}
	if (!(iSettings.iDisplay & KPwKeepLog))
		{
		iEikonEnv->InfoMsg(_L("No log kept - tick Tools > Troubleshooting > Keep a log"));
		return;
		}
	if (!iRunning)
		{
		iEikonEnv->InfoMsg(_L("Not available - the browser engine has stopped"));
		return;
		}
	UseSharedLink(iCoeEnv->FsSession(), iSettings);   // the proxy may have been set in another app
	if (!iSettings.iUseProxy || iSettings.iProxyHost.Length() == 0)
		{
		iEikonEnv->InfoMsg(_L("Not available - set a web helper in Connection settings"));
		return;
		}
	iShared->use_proxy = iSettings.iUseProxy;
	CopyToC(iShared->proxy_host, sizeof(iShared->proxy_host), iSettings.iProxyHost);
	iShared->proxy_port = iSettings.iProxyPort;
	iShared->log_state = PW_LOG_RUNNING;
	iShared->log_msg[0] = 0;
	iLogState = PW_LOG_RUNNING;
	TRAPD(err, iEikonEnv->BusyMsgL(_L("Sending the log\x85"), EHLeftVBottom, TTimeIntervalMicroSeconds32(0)));
	(void)err;
	Command(PW_CMD_PUTLOG, KNullDesC);
	}

void CPwView::LogTick()
	{
	PwShared* s = iShared;
	TInt st = s->log_state;
	if (st == PW_LOG_RUNNING)
		{
		iLogState = st;
		return;
		}
	if (iLogState != PW_LOG_RUNNING)
		{
		iLogState = st;                      // (not ours: a stale value)
		return;
		}
	iLogState = st;
	iEikonEnv->BusyMsgCancel();
	iLinkBusy = EFalse;
	if (st == PW_LOG_SENT)
		iEikonEnv->InfoMsg(_L("Log sent"));
	else
		{
		TBuf<96> why;
		TBuf<128> msg;
		FromUtf8(why, s->log_msg);
		if ((why.Length() >= 2 && why.Left(2) == _L("No")) || (why.Length() >= 5 && why.Left(5) == _L("Needs")))
			msg = why;                       // "Not available ...", "No log to send", "Needs PsiProxy on a computer"
		else
			{
			msg = _L("Cannot send the log - ");
			msg.Append(why);
			}
		iEikonEnv->InfoMsg(msg);
		}
	}

// ----- Update PsiWeb ------------------------------------------------------------

void CPwView::StartUpdateL()
	{
	if (!iRunning)
		{
		iEikonEnv->InfoMsg(_L("Not available - the browser engine has stopped"));
		return;
		}
	if (iUpdState == PW_UPD_RUNNING)
		{
		iEikonEnv->InfoMsg(_L("Not available while updating"));
		return;
		}
	iShared->update_state = PW_UPD_RUNNING;
	iUpdState = -1;                      // (the first tick shows the engine's first words)
	iUpdLog.Zero();
	UpdateLine(_L("Starting the update\x85"));
	Command(PW_CMD_UPDATE, KNullDesC);
	UpdateDialogL();
	}

// A line for the progress window. A download count ("12 of 2700 KB")
// replaces the last line if that was one too, so the window shows steps and
// not a hundred counts.
void CPwView::UpdateLine(const TDesC& aLine)
	{
	TBool count = aLine.Find(_L(" KB")) >= 0;
	TInt last = iUpdLog.LocateReverse(CEditableText::EParagraphDelimiter);
	if (count && iUpdLog.Length() > 0 && iUpdLog.Mid(last + 1).Find(_L(" KB")) >= 0)
		iUpdLog.SetLength(last >= 0 ? last : 0);
	if (iUpdLog.Length() > 0)
		iUpdLog.Append(CEditableText::EParagraphDelimiter);
	while (iUpdLog.Length() + aLine.Length() + 1 > iUpdLog.MaxLength())
		{
		TInt p = iUpdLog.Locate(CEditableText::EParagraphDelimiter);
		if (p < 0)
			{
			iUpdLog.Zero();
			break;
			}
		iUpdLog.Delete(0, p + 1);
		}
	iUpdLog.Append(aLine.Left(aLine.Length() < 120 ? aLine.Length() : 120));
	}

// The progress window stays up while the engine works (Stop asks it to give
// up); the tick adds its words and, at the end, turns Stop into Close. What
// the update came to is then said: install it, saved, current, failed.
void CPwView::UpdateDialogL()
	{
	CPwUpdateProgress* dlg = new(ELeave) CPwUpdateProgress(*this);
	iUpdDlg = dlg;
	TRAPD(err, dlg->ExecuteLD(R_PW_UPDATE_PROGRESS));
	iUpdDlg = NULL;
	User::LeaveIfError(err);
	if (iUpdState == PW_UPD_READY)
		{
		TBuf<64> q;
		TBuf<16> v;
		FromUtf8(v, iShared->update_version);
		q.Format(_L("Install PsiWeb %S now?"), &v);
		if (iEikonEnv->QueryWinL(_L("PsiWeb will close while it installs"), q))
			StartInstallerL();
		else
			iEikonEnv->InfoWinL(_L("Update saved"), iUpdateFile);
		}
	}

// progress while the engine downloads; the outcome when it has finished
void CPwView::UpdateTickL()
	{
	PwShared* s = iShared;
	TInt st = s->update_state;
	TBuf<128> msg;
	FromUtf8(msg, s->update_msg);
	if (st == PW_UPD_RUNNING)
		{
		if (iUpdState != PW_UPD_RUNNING || msg != iUpdMsg)
			{
			iUpdState = PW_UPD_RUNNING;
			iUpdMsg = msg;
			UpdateLine(msg);
			if (iUpdDlg)
				iUpdDlg->ShowL(iUpdLog);
			}
		return;
		}
	TInt was = iUpdState;
	iUpdState = st;
	if (was != PW_UPD_RUNNING && was != -1)
		return;
	// it has ended: the last words, and Stop becomes Close. What it came
	// to is said when the window closes (UpdateDialogL).
	iUpdMsg = msg;
	UpdateLine(msg);
	if (iUpdDlg)
		iUpdDlg->FinishL(iUpdLog);
	}

void CPwUpdateProgress::SetSizeAndPositionL(const TSize& aSize)
	{
	TSize screen = iEikonEnv->ScreenDevice()->SizeInPixels();
	TSize size(aSize.iWidth < screen.iWidth - 8 ? aSize.iWidth : screen.iWidth - 8,
		aSize.iHeight < screen.iHeight - 8 ? aSize.iHeight : screen.iHeight - 8);
	SetCornerAndSizeL(EHCenterVCenter, size);   // what CEikDialog does, clamped
	}

void CPwUpdateProgress::PreLayoutDynInitL()
	{
	ShowL(iView.UpdateLog());
	}

// The last few lines showing (the text scrolls with the arrow keys)
void CPwUpdateProgress::ShowL(const TDesC& aText)
	{
	CEikEdwin* ed = (CEikEdwin*)Control(EPwDlgUpdText);
	ed->SetTextL(&aText);
	TInt pos = aText.Length(), paras = 0;
	while (pos > 0)
		{
		if (aText[pos - 1] == CEditableText::EParagraphDelimiter && ++paras >= 6)
			break;
		pos--;
		}
	ed->SetCursorPosL(aText.Length(), EFalse);
	ed->SetCursorPosL(pos, EFalse);
	ed->DrawNow();
	}

// One button: "Stop" while the job runs, "Close" afterwards
void CPwUpdateProgress::SetButtonTextL(const TDesC& aText)
	{
	CEikCommandButtonBase* b = ButtonPanel()->ButtonById(EEikBidOk);
	if (b)
		{
		((CEikCommandButton*)b)->SetTextL(aText);
		b->DrawNow();
		}
	}

void CPwUpdateProgress::FinishL(const TDesC& aText)
	{
	if (iFinished)
		return;
	iFinished = ETrue;
	ShowL(aText);
	SetButtonTextL(_L("Close"));
	}

TBool CPwUpdateProgress::OkToExitL(TInt /*aButtonId*/)
	{
	if (iFinished)
		return ETrue;
	iView.StopUpdate();                  // Stop (or Esc): ask the updater to give up, stay open
	return EFalse;
	}

void CPwView::StartInstallerL()
	{
	StartBusyCancel();
	StopEngine();                        // psiweb.exe is one of the files replaced
	TInt err;
	RApaLsSession ls;
	err = ls.Connect();
	if (err == KErrNone)
		{
		TThreadId tid;
		err = ls.StartDocument(iUpdateFile, TUid::Uid(0x10000419), tid);
		ls.Close();
		}
	if (err == KErrNone)
		{
		iEikonEnv->EikAppUi()->HandleCommandL(EEikCmdExit);
		return;
		}
	TBuf<160> m;
	PsiErrMsg(m, _L("Cannot start the installer"), err);   // (0.96: in words, review R105)
	m.Append(_L(" - open "));
	m.Append(iUpdateFile.Left(Min(iUpdateFile.Length(), m.MaxLength() - m.Length() - 23)));
	m.Append(_L(" from the System screen"));
	iEikonEnv->InfoWinL(_L("Update downloaded"), m);
	}

void CPwView::Draw(const TRect& aRect) const
	{
	CWindowGc& gc = SystemGc();
	// (0.89) right of the engine's page (its width is even, the room may
	// be a pixel wider) and, with no scroll bar, the bar's room: the page's
	// white margin, never an old frame's columns
	TInt engW = iShared && iShared->width > 0 && iShared->width < iPageArea.Width() ? iShared->width : iPageArea.Width();
		{
		TRect strip(Rect());
		strip.iTl.iX = iPageArea.iTl.iX + engW;
		if (iSbShown)
			strip.iBr.iX = iPageArea.iBr.iX;
		strip.Intersection(aRect);
		if (!strip.IsEmpty())
			{
			gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
			gc.SetBrushColor(KRgbWhite);
			gc.SetPenStyle(CGraphicsContext::ENullPen);
			gc.DrawRect(strip);
			gc.SetPenStyle(CGraphicsContext::ESolidPen);
			gc.SetBrushStyle(CGraphicsContext::ENullBrush);
			}
		}
	if (iShowMsg)
		{
		gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
		gc.SetBrushColor(KRgbWhite);
		gc.SetPenStyle(CGraphicsContext::ENullPen);
		gc.DrawRect(iPageArea);
		gc.SetPenStyle(CGraphicsContext::ESolidPen);
		gc.SetPenColor(KRgbBlack);
		const CFont* font = iEikonEnv->TitleFont();
		gc.UseFont(font);
		gc.DrawText(iMsg1, TPoint(20, 90));
		gc.DiscardFont();
		font = iEikonEnv->NormalFont();
		gc.UseFont(font);
		// (0.94) a reason too wide for the page: its remedy, after " - ",
		// on a line of its own
		TInt dash = iMsg2.Find(_L(" - "));
		if (dash > 0 && font->TextWidthInPixels(iMsg2) > iPageArea.Width() - 40)
			{
			gc.DrawText(iMsg2.Left(dash), TPoint(20, 120));
			gc.DrawText(iMsg2.Mid(dash + 3), TPoint(20, 120 + font->HeightInPixels() + 4));
			}
		else
			gc.DrawText(iMsg2, TPoint(20, 120));
		gc.DiscardFont();
		return;
		}
	TRect r(aRect);
	TRect page(iPageArea);
	page.iBr.iX = page.iTl.iX + engW;
	r.Intersection(page);                // (the scroll bar draws itself)
	if (!r.IsEmpty())
		BlitRows(gc, r);
	}

// (1.00, MEM-76) the engine's frame (the shared chunk: exactly an EGray16 or
// EColor256 scan line's layout) drawn at aRect, a strip of KPwStripRows
// lines at a time through iBitmap, each blitted before the next is filled;
// the frame's row y is the window's y
void CPwView::BlitRows(CWindowGc& aGc, const TRect& aRect) const
	{
	PwShared* s = iShared;
	if (!s || !iBitmap)
		return;
	TInt y1 = aRect.iBr.iY < s->height ? aRect.iBr.iY : s->height;
	for (TInt y = aRect.iTl.iY < 0 ? 0 : aRect.iTl.iY; y < y1; y += KPwStripRows)
		{
		TInt n = y1 - y < KPwStripRows ? y1 - y : KPwStripRows;
		for (TInt k = 0; k < n; k++)
			{
			TPtr8 row(s->fb + (y + k) * s->stride, s->stride, s->stride);
			iBitmap->SetScanLine(row, k);
			}
		aGc.BitBlt(TPoint(aRect.iTl.iX, y), iBitmap, TRect(aRect.iTl.iX, 0, aRect.iBr.iX, n));
		// the window server reads the strip when it carries the BitBlt
		// out, not when it is asked: it must have done so before the strip
		// is filled again (else every strip shows the last one's lines)
		iCoeEnv->WsSession().Flush();         // (carried out before the request returns)
		}
	}

void CPwView::PushEvent(TInt aType, TInt aCode, TInt aX, TInt aY)
	{
	PwShared* s = iShared;
	if (!iRunning || s->ev_head - s->ev_tail >= PW_EVQ)
		return;
	PwEvent& e = s->ev[s->ev_head % PW_EVQ];
	e.type = aType;
	e.code = aCode;
	e.x = aX;
	e.y = aY;
	s->ev_head++;
	s->net.resized = 1;                  // wakes the engine's wait (see psi_os.c)
	RingEngine();                        // (0.81) it may be waiting on its doorbell
	TickFast();                          // ...and the frame it draws is wanted soon
	}

// Key and pen timings: the randomness behind TLS keys (see psiglue pg_entropy)
void CPwView::AddEntropy(TUint aValue)
	{
	TUint32 t = User::TickCount() ^ (aValue << 16);
	TTime now;
	now.UniversalTime();
	t ^= now.Int64().Low();
	iShared->net.entropy[iEntropyPos % PSI_ENTROPY_SIZE] ^= (TUint8)t;
	iShared->net.entropy[(iEntropyPos + 1) % PSI_ENTROPY_SIZE] ^= (TUint8)(t >> 8);
	iEntropyPos += 2;
	if (iShared->net.entropy_len < PSI_ENTROPY_SIZE)
		iShared->net.entropy_len = iEntropyPos < PSI_ENTROPY_SIZE ? iEntropyPos : PSI_ENTROPY_SIZE;
	}

void CPwView::Key(TUint aCode, TUint aMods)
	{
	PushEvent(PW_EV_KEYDOWN, aCode, aMods, 0);
	}

TKeyResponse CPwView::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
	{
	CPwAppUi* ui = (CPwAppUi*)iEikonEnv->EikAppUi();
	if (aType != EEventKey)
		return EKeyWasNotConsumed;
	TUint code = aKeyEvent.iCode;
	TUint mods = aKeyEvent.iModifiers;
	if (code == EKeyMenu)
		return EKeyWasNotConsumed;
	// (0.78) the toolband's address is being typed in: it has the keys,
	// but for the menu's shortcuts
	if (ui->Editing() && !(mods & EModifierCtrl) && ui->AddressKeyL(aKeyEvent, aType))
		return EKeyWasConsumed;
	// (0.94) Ctrl+A in the address: all of it chosen
	if (ui->Editing() && (mods & EModifierCtrl) && !(mods & EModifierShift) && (code == 1 || (code | 0x20) == 'a'))
		{
		ui->SelectAllAddressL();
		return EKeyWasConsumed;
		}
	// the Zoom in and Zoom out icons beside the screen: as View > Zoom in
	// and Zoom out (Ctrl+M, Shift+Ctrl+M)
	if (code == (TUint)EEikSidebarZoomInKey || code == (TUint)EEikSidebarZoomOutKey)
		{
		iEikonEnv->EikAppUi()->HandleCommandL(code == (TUint)EEikSidebarZoomInKey ? EPwCmdZoomIn : EPwCmdZoomOut);
		return EKeyWasConsumed;
		}
	AddEntropy(code);
	if (mods & EModifierCtrl)
		{
		// leave the menu's shortcuts to the menu (see psiweb.rss);
		// Ctrl+letter arrives as 1..26
		TUint letter = (code >= 1 && code <= 26) ? 'a' + code - 1 : (code | 0x20);
		if (mods & EModifierShift)
			{
			// Shift+Ctrl+M zoom out, Q page information, H help, A about,
			// R Reading mode
			switch (letter)
				{
				case 'm': case 'q': case 'h': case 'a': case 'r': case 't':
					return EKeyWasNotConsumed;
				}
			}
		else
			{
			switch (letter)
				{
				case 'e': case 'l': case 'o': case 'h': case 'r': case 'b':
				case 'f': case 'm': case 'i': case 't': case 'k': case 'u':
				case 'x': case 'c': case 'v':    // (0.94) Edit's Cut, Copy, Paste
					return EKeyWasNotConsumed;
				}
			}
		}
	if (code == EKeyEscape && iUpdState == PW_UPD_RUNNING)
		{
		StopUpdate();                    // (the progress window has it too)
		return EKeyWasConsumed;
		}
	if (iUpdState == PW_UPD_RUNNING)
		return EKeyWasConsumed;
	if (code == EKeyEscape && iShared->busy)
		{
		Command(PW_CMD_STOP, KNullDesC);
		return EKeyWasConsumed;
		}
	// (0.87) the start page: 1 to 8 open its tiles, unless a text field
	// (its search box) has the cursor. (They pick from the numbered list on
	// the page, as Screen greys' 1 to 4 do; they are not shortcuts.)
	if (code >= '1' && code <= '8' && !(mods & (EModifierCtrl | EModifierShift | EModifierFunc))
		&& iRunning && !iShared->field_active && !iShared->busy
		&& TPtrC8((const TUint8*)iShared->url).Compare(_L8("about:start")) == 0)
		{
		ui->OpenTileL(code - '1');
		return EKeyWasConsumed;
		}
	Key(code, mods);
	return EKeyWasConsumed;
	}

void CPwView::HandlePointerEventL(const TPointerEvent& aEvent)
	{
	TPoint p = aEvent.iPosition;
	AddEntropy(p.iX * 1000 + p.iY);
	// the pen went down on the scroll bar: it has the pen until it comes up
	if (aEvent.iType == TPointerEvent::EButton1Down)
		iSbPen = !iPageArea.Contains(p);
	if (iSbPen)
		{
		CCoeControl::HandlePointerEventL(aEvent);
		if (aEvent.iType == TPointerEvent::EButton1Up)
			iSbPen = EFalse;
		return;
		}
	switch (aEvent.iType)
		{
	case TPointerEvent::EButton1Down:
		PushEvent(PW_EV_PENDOWN, 0, p.iX, p.iY);
		break;
	case TPointerEvent::EDrag:
		PushEvent(PW_EV_PENMOVE, 0, p.iX, p.iY);
		break;
	case TPointerEvent::EButton1Up:
		PushEvent(PW_EV_PENUP, 0, p.iX, p.iY);
		break;
	default:
		break;
		}
	}

// ===========================================================================
// Dialogs
// ===========================================================================

void CPwInfoDialog::PreLayoutDynInitL()
	{
	for (TInt i = 0; i < 6; i++)
		{
		if (i < iCount)
			SetLabelL(EPwDlgInfo1 + i, iLines[i]);
		else
			MakeLineVisible(EPwDlgInfo1 + i, EFalse);
		}
	}

void CPwConnDialog::PreLayoutDynInitL()
	{
	((CEikChoiceList*)Control(EPwDlgLink))->SetCurrentItem(iSettings.iNetMode ? 1 : 0);
	((CEikChoiceList*)Control(EPwDlgBaud))->SetCurrentItem(iSettings.iBaudIndex);
	((CEikChoiceList*)Control(EPwDlgFlow))->SetCurrentItem(iSettings.iRtsCts ? 1 : 0);
	SetEdwinTextL(EPwDlgPppStart, &iSettings.iPppStart);
	// the proxy, shared by all three apps (psilink.h)
	TPsiLink link;
	link.SetDefaults();
	link.Load(iCoeEnv->FsSession());
	SetChoiceListCurrentItem(EPwDlgProxy, link.iUseProxy ? 1 : 0);
	SetEdwinTextL(EPwDlgProxyHost, &link.iProxyHost);
	SetNumberEditorValue(EPwDlgProxyPort, link.iProxyPort);
	SetChoiceListCurrentItem(EPwDlgKeepUp, link.iKeepUp ? 0 : 1);
	DimProxyLines();
	DimKeepUpLine();
	}

// (0.73) staying connected after use is the Psion Internet route's only:
// the modem route always hangs up (it holds the serial port)
void CPwConnDialog::DimKeepUpLine()
	{
	SetLineDimmedNow(EPwDlgKeepUp, ChoiceListCurrentItem(EPwDlgLink) != 1);
	}

// the host and port wait for "Use proxy" to be Yes: dimmed, not hidden
void CPwConnDialog::DimProxyLines()
	{
	TBool off = ChoiceListCurrentItem(EPwDlgProxy) != 1;
	SetLineDimmedNow(EPwDlgProxyHost, off);
	SetLineDimmedNow(EPwDlgProxyPort, off);
	}

void CPwConnDialog::HandleControlStateChangeL(TInt aControlId)
	{
	if (aControlId == EPwDlgProxy)
		DimProxyLines();
	else if (aControlId == EPwDlgLink)
		DimKeepUpLine();
	}

// Test: tries the values shown (not yet saved). If the browser engine has
// the port (mid-connection), the test says the port is in use.
void CPwConnDialog::TestL()
	{
	TBuf<40> ppp;
	GetEdwinText(ppp, EPwDlgPppStart);
	ppp.TrimAll();
	PgLinkTestL(((CEikChoiceList*)Control(EPwDlgBaud))->CurrentItem(),
		((CEikChoiceList*)Control(EPwDlgFlow))->CurrentItem() == 1,
		((CEikChoiceList*)Control(EPwDlgLink))->CurrentItem() == 1,
		ppp, R_PW_TEST_DIALOG, EPwDlgTest1);
	}

TBool CPwConnDialog::OkToExitL(TInt aButtonId)
	{
	if (aButtonId == EPwBidTest)
		{
		TestL();
		return EFalse;
		}
	TInt proxy = ChoiceListCurrentItem(EPwDlgProxy) == 1;
	TBuf<60> host;
	GetEdwinText(host, EPwDlgProxyHost);
	host.Trim();
	TInt port = NumberEditorValue(EPwDlgProxyPort);
	if (proxy && host.Length() == 0)
		{
		iEikonEnv->InfoMsg(_L("No helper's address entered"));
		TryChangeFocusToL(EPwDlgProxyHost);
		return EFalse;
		}
	iSettings.iNetMode = ((CEikChoiceList*)Control(EPwDlgLink))->CurrentItem() == 1;
	iSettings.iBaudIndex = ((CEikChoiceList*)Control(EPwDlgBaud))->CurrentItem();
	iSettings.iRtsCts = ((CEikChoiceList*)Control(EPwDlgFlow))->CurrentItem() == 1;
	TBuf<40> ppp;
	GetEdwinText(ppp, EPwDlgPppStart);
	ppp.TrimAll();
	iSettings.iPppStart = ppp;
	// the proxy goes straight to the shared file; the app's own link lines
	// are saved by the caller (SaveSharedLink keeps the proxy as it finds it)
	RFs& fs = iCoeEnv->FsSession();
	TPsiLink link;
	link.SetDefaults();
	link.Load(fs);
	link.iUseProxy = proxy;
	if (host.Length())
		link.iProxyHost = host;
	if (port > 0 && port < 65536)
		link.iProxyPort = port;
	link.iKeepUp = ChoiceListCurrentItem(EPwDlgKeepUp) == 0;
	link.Save(fs);
	// a link already up and now to stay up: hold it (psihold.exe ends by
	// itself when the setting goes the other way)
	if (link.iKeepUp && iSettings.iNetMode)
		{
		TFileName near(iEikonEnv->EikAppUi()->Application()->AppFullName());
		PsiHoldStart(fs, near, (iSettings.iDisplay & KPwKeepLog) != 0);
		}
	return ETrue;
	}

// (0.99) the family's About (Coda's style sheet 2.7): "PsiWeb 0.99" in
// bold, what it is, the copyright (with Coda's line on Coda's netBook only),
// Buy me a coffee
void CPwAboutDialog::PreLayoutDynInitL()
	{
	TBuf<32> title(_L("PsiWeb "));
	title.Append(KVersion);
	((CEikLabel*)Control(EPwDlgAbout1))->SetFont(iEikonEnv->TitleFont());   // (the dialogs' bold)
	SetLabelL(EPwDlgAbout1, title);
	// "part of Coda" only on Coda's netBook (its ROM has PsiBattHook.bin,
	// as the toolbar's test), as PsiTerm's and PsiMail's About: never on
	// Psion's own image, the Series 7 or the 5mx
	TEntry e;
	TBool coda = iCoeEnv->FsSession().Entry(_L("Z:\\System\\Libs\\PsiBattHook.bin"), e) == KErrNone;
	SetLabelL(EPwDlgAbout4, coda ? _L("\xA9 Dan Edge 2026 - part of Coda for the Psion netBook")
		: _L("\xA9 Dan Edge 2026"));
	}

TBool CPwAboutDialog::OkToExitL(TInt aButtonId)
	{
	if (aButtonId == EPwBidCredits)
		{
		CPwCreditsDialog* dlg = new(ELeave) CPwCreditsDialog;
		dlg->ExecuteLD(R_PW_CREDITS_DIALOG);
		return EFalse;                       // (About stays)
		}
	return ETrue;
	}

// About > Credits: the lines with a copyright sign are set here (the
// resource compiler takes plain text)
void CPwCreditsDialog::PreLayoutDynInitL()
	{
	SetLabelL(EPwDlgCredit1, _L("Links 2 browser engine \xA9 Mikulas Patocka & others - GPL v2"));
	SetLabelL(EPwDlgCredit6, _L("\xA9 Symbian Ltd 1998-9"));
	}

void CPwUpdateDialog::PreLayoutDynInitL()
	{
	((CEikChoiceList*)Control(EPwDlgUpdSource))->SetCurrentItem(iSource >= 0 && iSource <= 2 ? iSource : 0);
	SetEdwinTextL(EPwDlgUpdHost, &iHost);
	SetNumberEditorValue(EPwDlgUpdPort, iPort > 0 ? iPort : 8686);
	DimComputerLines();
	}

// (0.99) the computer's address and port matter only for "A computer on my
// network": dimmed for the other two, as the style guide dims what does
// not apply
void CPwUpdateDialog::DimComputerLines()
	{
	TBool off = ((CEikChoiceList*)Control(EPwDlgUpdSource))->CurrentItem() != 2;
	SetLineDimmedNow(EPwDlgUpdHost, off);
	SetLineDimmedNow(EPwDlgUpdPort, off);
	}

void CPwUpdateDialog::HandleControlStateChangeL(TInt aControlId)
	{
	if (aControlId == EPwDlgUpdSource)
		DimComputerLines();
	}

TBool CPwUpdateDialog::OkToExitL(TInt /*aButtonId*/)
	{
	iSource = ((CEikChoiceList*)Control(EPwDlgUpdSource))->CurrentItem();
	GetEdwinText(iHost, EPwDlgUpdHost);
	iHost.Trim();
	iPort = NumberEditorValue(EPwDlgUpdPort);
	if (iSource == 2 && iHost.Length() == 0)
		{
		iEikonEnv->InfoMsg(_L("No computer's address entered"));
		TryChangeFocusToL(EPwDlgUpdHost);
		return EFalse;
		}
	return ETrue;
	}

void CPwPrefsDialog::PreLayoutDynInitL()
	{
	SetEdwinTextL(EPwDlgHome, &iSettings.iHome);
	((CEikChoiceList*)Control(EPwDlgPictures))->SetCurrentItem(iSettings.iImages ? 1 : 0);
	((CEikChoiceList*)Control(EPwDlgText))->SetCurrentItem((iSettings.iDisplay & KPwDisplayScaledText) ? 1 : 0);
	SetChoiceListCurrentItem(EPwDlgSearch, iSettings.iSearch == PW_SEARCH_FROGFIND ? 1 : 0);
	SetChoiceListCurrentItem(EPwDlgStyle, StyleNow(iSettings));
	SetChoiceListCurrentItem(EPwDlgCookies, (iSettings.iFlags & KPwFlagKeepCookies) ? 1 : 0);
	}

TBool CPwPrefsDialog::OkToExitL(TInt aButtonId)
	{
	if (aButtonId == EPwBidGreys)
		{
		// the greys screen shared with PsiTerm and PsiMail, on top of this
		// dialog; the engine reads PsiGrey.ini at each page load
		PsiGrey grey;
		TInt r = PsiGreyScreenL(grey);
		if (r > 0)
			iEikonEnv->InfoMsg(_L("Greys saved - used from the next page"));
		else if (r < 0)
			iEikonEnv->InfoMsg(_L("Not saved - the internal disk is full or in use"));
		return EFalse;                        // (the dialog stays open)
		}
	GetEdwinText(iSettings.iHome, EPwDlgHome);
	iSettings.iHome.Trim();
	iSettings.iImages = ((CEikChoiceList*)Control(EPwDlgPictures))->CurrentItem() == 1;
	iSettings.iSearch = ChoiceListCurrentItem(EPwDlgSearch) == 1 ? PW_SEARCH_FROGFIND : PW_SEARCH_DDG;
	if (ChoiceListCurrentItem(EPwDlgCookies) == 1)
		iSettings.iFlags |= KPwFlagKeepCookies;
	else if (iSettings.iFlags & KPwFlagKeepCookies)
		{
		// (0.87) no longer kept: the cookies kept so far go
		iSettings.iFlags &= ~KPwFlagKeepCookies;
		iCoeEnv->FsSession().Delete(_L("C:\\System\\Apps\\PsiWeb\\cookies.txt"));
		}
	if (ChoiceListCurrentItem(EPwDlgStyle) != StyleNow(iSettings))
		iSettings.iStyle = ChoiceListCurrentItem(EPwDlgStyle);   // (0.86) chosen: no longer the route's standard
	if (((CEikChoiceList*)Control(EPwDlgText))->CurrentItem() == 1)
		iSettings.iDisplay |= KPwDisplayScaledText;
	else
		iSettings.iDisplay &= ~KPwDisplayScaledText;
	return ETrue;
	}

// ===========================================================================
// App UI
// ===========================================================================

void CPwAppUi::ConstructL()
	{
	BaseConstructL();
	// (0.95) Coda's rule for settings on C: (psikeep.h): a new C: (a new
	// image, a cold start, a flat battery) takes the card's copies of the
	// settings and the favourites; cookies and the history are never copied
	RFs& kfs = iCoeEnv->FsSession();
	TBool keptBack = PsiKeepRestore(kfs, KIniFile);
	if (PsiKeepRestore(kfs, KFavsKept))
		keptBack = ETrue;
	PsiKeepRestore(kfs, KUpdIniFile);
	PsiKeepRestore(kfs, KPsiLinkFile);
	// (1.00, MEM-75) an update saved by Tools > Update on C: (2.6 MB of
	// the Psion's memory, with no card) was never deleted: it goes at the
	// next start, when nothing is downloading or installing it. One saved
	// on the card (D:) stays, to be installed later
	kfs.Delete(_L("C:\\PsiWeb-update.sis"));
	TPwSettings settings;
	LoadSettings(settings);
	iShownProgress = -1;
	iLockShown = -1;
	iBackShown = iForwardShown = -1;
	TallToolbarL();                          // (0.99) the netBook's own toolbar
	// (0.99) the toolbar's name is a CEikFileNameLabel, as in Psion's own
	// programs: it shows the "file" name, which is the program's here
	iEikonEnv->Process()->SetMainDocFileName(_L("PsiWeb"));
	CEikFileNameLabel* name = iToolBar ? (CEikFileNameLabel*)iToolBar->ControlById(EPwCtlFileName) : NULL;
	if (name)
		{
		TRAPD(ne, name->UpdateL());
		(void)ne;
		}
	TRAPD(pics, ToolbarPicturesL());
	(void)pics;                              // (no PsiWeb.mbm: words only)
	// the Pictures buttons stay pressed in while the page's pictures show
	CEikButtonBase* picsButton = iToolBar ? (CEikButtonBase*)iToolBar->ControlById(EPwCmdImages) : NULL;
	if (picsButton)
		picsButton->SetBehavior(EEikButtonLatches);
	picsButton = iToolBand ? (CEikButtonBase*)iToolBand->ControlById(EPwCmdImages) : NULL;
	if (picsButton)
		picsButton->SetBehavior(EEikButtonLatches);
	// the toolband's address: a field with a sunken edge; a tap on it (seen
	// in HandleWsEventL and through its observer) or Ctrl+L starts typing
	if (iToolBand)
		{
		iAddress = (CEikEdwin*)iToolBand->ControlById(EPwCtlAddress);
		if (iAddress)
			{
			// (1.03) a plain 1-pixel frame all round, on the band's own top
			// and bottom lines: the address is exactly the buttons' height and
			// lines up with them (the shallow sunken edge had two black lines
			// at its top, one grey at its left and none at its foot or right)
			iAddress->SetBorder(TEikBorder::ESingleBlack);
			// the text 4 pixels in from the frame's sides and, in the
			// band's 25 lines, set down 3 so it sits in the middle above
			// the progress bar (it touched the frame at the left and sat
			// high)
			TMargins8 m;
			m.iLeft = 4;
			m.iRight = 4;
			m.iTop = 3;
			m.iBottom = 0;
			iAddress->SetBorderViewMargins(m);
			iAddress->SetObserver(this);
			}
		// (0.87) how far the page has come: a bar along the address's
		// lower edge while it loads, nothing when idle
		iBar = new(ELeave) CPwBar;
		iBar->ConstructL(*iToolBand);
		}
	if (iToolBar && !settings.iToolbar)
		iToolBar->MakeVisible(EFalse);       // remembered from last time
	if (iToolBand)
		{
		iToolBand->MakeVisible(settings.iToolband);
		iToolBand->ActivateL();              // (EEikToolBarDelayActivation: laid out first)
		}
	ShowPicturesState(settings.iImages);
	iFavs = CPwList::NewL(iCoeEnv->FsSession(), _L("C:\\System\\Apps\\PsiWeb\\Favourites.dat"), KPwMaxFavs);
	iHist = CPwList::NewL(iCoeEnv->FsSession(), _L("C:\\System\\Apps\\PsiWeb\\History.dat"), KPwMaxHist);
	iHist->SetLazy();                        // (0.96, review R115) not rewritten whole for every page
	SeedFavouritesL();                       // (0.87) a new PsiWeb: its first tiles
	iView = new(ELeave) CPwView;
	iView->ConstructL(PageRect(), settings);
	UpdateStartPageL();                      // (0.87) before the engine's first page
	if (iToolBand && iToolBand->IsVisible())
		iToolBand->DrawNow();
	AddToStackL(iView);
	// (1.00) on the memory watcher's list (psimemlvl.h): PsiWeb takes "trim"
	// and "sleep" (ProcessMessageL), and says when it is busy
	PsiMemRegister(iMemChunk, KUidPsiWeb.iUid);
	iCoeEnv->RootWin().EnableOnEvents(EEventControlAlways);   // (0.54) switch-on events even when in the background
	// the user's contrast and backlight back if a program stopped in
	// Reading mode without doing it (docs/display.md)
	TPsiReading::Recover(iCoeEnv->FsSession(), iCoeEnv->WsSession(), KUidPsiWeb);
	iForeground = ETrue;
	UpdateReading();
	// (0.85) a new user (no settings in any program of the suite): the
	// setup assistant over the first page, once the window is up
	if (CPgSetupDialog::FirstRun(iCoeEnv->FsSession(), KIniFile))
		{
		iSetupIdle = CIdle::NewL(CActive::EPriorityIdle);
		iSetupIdle->Start(TCallBack(SetupCallback, this));
		}
	// (0.95) the card's copies up to date (settings from an older PsiWeb)
	PsiKeepFile(kfs, KIniFile);
	PsiKeepFile(kfs, KFavsKept);
	PsiKeepFile(kfs, KUpdIniFile);
	PsiKeepFile(kfs, KPsiLinkFile);
	if (keptBack)
		iEikonEnv->InfoMsg(_L("Settings and favourites put back from the card"));
	}

// ----- Reading mode (docs/display.md) ----------------------------------------------

void CPwAppUi::UpdateReading(TBool aEnterOnly)
	{
	if (!iView)
		return;
	if (iForeground && (iView->Settings().iDisplay & KPwDisplayReading))
		iReading.Enter(iCoeEnv->FsSession(), KUidPsiWeb);
	else if (!aEnterOnly)
		iReading.Leave(iCoeEnv->FsSession(), KUidPsiWeb);
	}

// Reading mode follows PsiWeb in and out of the foreground (CCoeAppUi's
// HandleForegroundEventL is private on ER5: the focus events are seen here);
// after a switch-on turned it off, the next key turns it on again
void CPwAppUi::HandleWsEventL(const TWsEvent& aEvent, CCoeControl* aDestination)
	{
	if (aEvent.Type() == EEventFocusGained || aEvent.Type() == EEventFocusLost)
		{
		iForeground = aEvent.Type() == EEventFocusGained;
		if (iView)
			iView->SetForeground(iForeground);   // (0.92) memory: trim and sleep while behind
		if (!iForeground)
			SaveHistory();                   // (0.96) the pages of this visit, once
		UpdateReading();
		if (iFullBusyAt)                     // (0.88) PsiTerm came in front: "Opening in PsiTerm..." goes
			{
			iEikonEnv->BusyMsgCancel();
			iFullBusyAt = 0;
			}
		}
	else if (aEvent.Type() == EEventKey && aEvent.Key()->iRepeats > 0
		&& (aEvent.Key()->iModifiers & EModifierCtrl) && (aEvent.Key()->iCode == 20 || aEvent.Key()->iCode == 't' || aEvent.Key()->iCode == 'T'))
		return;      // (0.78) Ctrl+T or Shift+Ctrl+T held: once, not again and again (each restarts the engine)
	else if (aEvent.Type() == EEventKey && iForeground && !iReading.Active())
		UpdateReading(ETrue);
	else if (aEvent.Type() == EEventPointer && iAddress)
		{
		// (0.78) the first tap on the toolband's address starts typing
		// there, with the whole address chosen (the tap goes no further,
		// so the edwin does not move the cursor), as a modern browser
		// does; later taps place the cursor. A tap anywhere else while
		// typing leaves the address as it was.
		const TPointerEvent& pe = *aEvent.Pointer();
		if (pe.iType == TPointerEvent::EButton1Down)
			{
			iPenTaken = EFalse;
			if (aDestination == iToolBand && iAddress->Rect().Contains(pe.iPosition) && !iEditing)
				{
				StartAddressL();
				iPenTaken = ETrue;
				}
			else if (iEditing && aDestination != iToolBand)
				EndAddressL(EFalse);
			}
		if (iPenTaken)
			{
			if (pe.iType == TPointerEvent::EButton1Up)
				iPenTaken = EFalse;
			return;
			}
		}
	CEikAppUi::HandleWsEventL(aEvent, aDestination);
	}

// The Psion was switched back on (0.54): the engine re-checks the link
// (PPP up? modem carrier?) rather than wait on a dead connection.
void CPwAppUi::HandleSwitchOnEventL(CCoeControl* aDestination)
	{
	(void)aDestination;                  // (the CONE default does nothing, and is private)
	if (iView && iView->Shared())
		{
		iView->Shared()->net.switch_on++;
		iView->RingEngine();             // (0.81) it may be waiting on its doorbell
		}
	iReading.Leave(iCoeEnv->FsSession(), KUidPsiWeb);   // (back on with the next key)
	}

// a toolbar button's picture, from PsiWeb.mbm (made by web/tools/mkicons.py):
// 24x20, in the middle of its side, the words beside it as the built-in
// programs' buttons have them
void CPwAppUi::ButtonPictureL(CEikToolBar* aBar, TInt aId, TInt aIcon)
	{
	CEikCommandButton* b = aBar ? (CEikCommandButton*)aBar->ControlById(aId) : NULL;
	if (!b)
		return;
	TFileName mbm = Application()->BitmapStoreName();
	CFbsBitmap* bmp = iEikonEnv->CreateBitmapL(mbm, aIcon);
	CleanupStack::PushL(bmp);
	CFbsBitmap* mask = iEikonEnv->CreateBitmapL(mbm, aIcon + 1);
	CleanupStack::PushL(mask);
	b->SetPictureL(bmp, mask);                // (the button owns them now)
	CleanupStack::Pop(2);
	if (b->Picture())
		b->Picture()->SetAlignment(EHCenterVCenter);
	if (b->Label())
		b->Label()->SetAlignment(EHLeftVCenter);
	b->LayoutComponentsL();
	}

// the Pictures button pressed in (the page's pictures show) or not
// (on the toolbar and on the toolband)
void CPwAppUi::ShowPicturesState(TBool aOn)
	{
	CEikToolBar* bars[2] = { iToolBar, iToolBand };
	CEikButtonBase::TState st = aOn ? CEikButtonBase::ESet : CEikButtonBase::EClear;
	for (TInt i = 0; i < 2; i++)
		{
		CEikButtonBase* b = bars[i] ? (CEikButtonBase*)bars[i]->ControlById(EPwCmdImages) : NULL;
		if (!b || b->State() == st)
			continue;
		b->SetState(st);
		if (bars[i]->IsVisible())
			b->DrawNow();
		}
	}

void CPwAppUi::ToolbarPicturesL()
	{
	// (0.99) on a colour screen the colour set (Coda's style sheet 2.3:
	// tools/mkicons.py), on the 5mx the four greys
	TBool c = ColourScreen();
	ButtonPictureL(iToolBar, EPwCmdOpen, c ? EMbmToolOpenC : EMbmToolOpen);
	ButtonPictureL(iToolBar, EPwCmdBack, c ? EMbmToolBackC : EMbmToolBack);
	ButtonPictureL(iToolBar, EPwCmdStart, c ? EMbmToolHomeC : EMbmToolHome);   // (0.87) Start, the start page
	ButtonPictureL(iToolBar, EPwCmdImages, c ? EMbmToolPicturesC : EMbmToolPictures);
	// (the toolband's pictures are named in its resource: pwicons.rh)
	}

// (0.99) A tall screen gets the netBook's toolbar in place of the 5mx's one
// that EIK_APP_INFO made (as PsiTerm makes its file view's toolbar): Coda's
// name box and 50-pixel buttons, and, when the ROM is Coda's (its hook in
// Eikon's control factory is there), the battery, Coda's toolbar extras
// and clock. The toolbar is not on the control stack, so it can simply be
// replaced; the toolband is laid out against it later (PageRect).
void CPwAppUi::TallToolbarL()
	{
	if (!iToolBar || !TallScreen())
		return;
	TEntry e;
	TBool coda = iCoeEnv->FsSession().Entry(_L("Z:\\System\\Libs\\PsiBattHook.bin"), e) == KErrNone;
	CEikToolBar* bar = new(ELeave) CEikToolBar;
	CleanupStack::PushL(bar);
	bar->ConstructL(this, coda ? R_PW_TOOLBAR_CODA : R_PW_TOOLBAR_TALL,
		TRect(TPoint(0, 0), iEikonEnv->ScreenDevice()->SizeInPixels()));
	CleanupStack::Pop();
	delete iToolBar;
	iToolBar = bar;
	if (iToolBand)
		iToolBar->DrawableWindow()->SetOrdinalPosition(0);   // (in front, as BaseConstructL puts it)
	}

// 480 lines (the netBook): room for the toolband as well as the page
TBool CPwAppUi::TallScreen() const
	{
	return iEikonEnv->ScreenDevice()->SizeInPixels().iHeight >= 400;
	}

// (0.99) a colour screen (the netBook, the Series 7)
TBool CPwAppUi::ColourScreen() const
	{
	TDisplayMode m = iEikonEnv->ScreenDevice()->DisplayMode();
	return m == EColor256 || m == EColor4K || m == EColor64K || m == EColor16M;
	}

// The bars laid out as they are shown, and the page's room: the rest.
// (ClientRect() keeps the toolbar's and the toolband's room back even when
// they are hidden.) The toolband runs along the top, as far as the toolbar
// when that is shown, else the whole width.
TRect CPwAppUi::PageRect()
	{
	TRect r(TPoint(0, 0), iEikonEnv->ScreenDevice()->SizeInPixels());
	if (iToolBar && iToolBar->IsVisible())
		iToolBar->ReduceRect(r);
	if (iToolBand)
		{
		// (an edwin's minimum size is its size: made small first, so the
		// band can lay it out shorter as well as longer)
		if (iAddress)
			{
			TRAPD(e2, iAddress->SetSizeL(TSize(40, iAddress->Size().iHeight)));
			(void)e2;
			}
		TRAPD(err, iToolBand->SetBoundingRectL(r));
		(void)err;
		if (iToolBand->IsVisible())
			iToolBand->ReduceRect(r);
		}
	return r;
	}

// View > Show toolbar (Ctrl+T): the page takes its room, or gives it back.
// The engine draws at the width it started with, so it starts again on the
// same page (a few seconds: say so, bottom left).
void CPwAppUi::ShowToolBarL(TBool aShow)
	{
	if (!iToolBar)
		return;
	iToolBar->MakeVisible(aShow);
	TRect page = PageRect();
	if (aShow)
		iToolBar->DrawNow();             // whole, before the engine's stop holds the screen up
	if (iToolBand && iToolBand->IsVisible())
		iToolBand->DrawNow();            // (longer or shorter now)
	iCoeEnv->WsSession().Flush();
	iView->SetPageRectL(page);
	}

// View > Show toolband (Shift+Ctrl+T), as Ctrl+T: the page takes its room
void CPwAppUi::ShowToolBandL(TBool aShow)
	{
	if (!iToolBand)
		return;
	if (!aShow && iEditing)
		EndAddressL(EFalse);
	iToolBand->MakeVisible(aShow);
	TRect page = PageRect();
	if (aShow)
		{
		iAddressStale = ETrue;           // (shown afresh)
		ChromeTick();
		iToolBand->DrawNow();
		iCoeEnv->WsSession().Flush();
		}
	iView->SetPageRectL(page);
	}

// ----- the toolband's address and progress (0.78) -----------------------------------

void CPwAppUi::SetAddressL(const TDesC& aText)
	{
	if (!iAddress)
		return;
	iAddress->SetTextL(&aText);
	iAddress->SetCursorPosL(0, EFalse);  // (the start of the address in view)
	if (iToolBand->IsVisible())
		iAddress->DrawNow();
	}

// the address and the progress bar follow the engine (each tick)
void CPwAppUi::ChromeTick()
	{
	if (!iView || !iAddress || !iToolBand->IsVisible())
		return;
	PwShared* s = iView->Shared();
	if (!s)
		return;
	if (!iEditing)
		{
		TBuf<PW_URL_MAX> url;
		if (iView->EngineRunning() || iView->Asleep())   // (0.92) asleep: the address stays
			FromUtf8(url, s->url);
		if (url.Length() == 0 && iView->EngineRunning() && s->busy)
			FromUtf8(url, s->start_url);     // (1.03) a new engine opening its first page: that page, not nothing
		if (url.Compare(_L("about:problem")) == 0)
			FromUtf8(url, s->problem_url);   // (0.85) the address that did not open, to correct
		else if (Clip(url, 6).Compare(_L("about:")) == 0)
			url.Zero();                      // (0.85) no internal names in the address
		if (iAddressStale || url.Compare(iShownUrl) != 0)
			{
			iAddressStale = EFalse;
			iShownUrl = url;
			TRAPD(err, SetAddressL(url));
			(void)err;
			}
		}
	TRAPD(lightErr, RouteLightL());
	(void)lightErr;
	TInt p = iView->EngineRunning() && s->busy ? s->progress : -1;
	if (p > 100)
		p = 100;
	if (p != iShownProgress && iBar)
		{
		if (p < 0)
			iBar->MakeVisible(EFalse);   // (the address shows again beneath)
		else
			{
			// along the address's lower edge, inside its frame
			TRect a(iAddress->Position(), iAddress->Size());
			a.Shrink(1, 1);
			TRAPD(e3, iBar->SetExtentL(TPoint(a.iTl.iX, a.iBr.iY - 3), TSize(a.Width(), 3)));
			(void)e3;
			iBar->SetValue(p);
			if (iShownProgress < 0)
				iBar->MakeVisible(ETrue);
			iBar->DrawNow();
			}
		iShownProgress = p;
		}
	// (0.87) Reload is Stop while a page loads: its picture says which
	TBool busy = iView->EngineRunning() && s->busy;
	if (busy != iBandBusy)
		{
		iBandBusy = busy;
		TRAPD(e4, BandPictureL(EPwCmdReloadStop, busy ? EMbmBandStop : EMbmBandReload));
		(void)e4;
		}
	TRAPD(e5, ShowNavStateL());
	(void)e5;
	}

// (1.03) a toolband button's picture (and its mask, the next id), redrawn
void CPwAppUi::BandPictureL(TInt aId, TInt aIcon)
	{
	CEikCommandButton* b = iToolBand ? (CEikCommandButton*)iToolBand->ControlById(aId) : NULL;
	if (!b)
		return;
	CFbsBitmap* bmp = iEikonEnv->CreateBitmapL(Application()->BitmapStoreName(), aIcon);
	CleanupStack::PushL(bmp);
	CFbsBitmap* mask = iEikonEnv->CreateBitmapL(Application()->BitmapStoreName(), aIcon + 1);
	CleanupStack::PushL(mask);
	b->SetPictureL(bmp, mask);               // (the button owns them now)
	CleanupStack::Pop(2);
	// in the middle of the button, as the resource's pictures are: a
	// picture set later sat 5 pixels left of the others (Reload's, in 1.02,
	// was the one Dan saw out of line)
	if (b->Picture())
		b->Picture()->SetAlignment(EHCenterVCenter);
	b->LayoutComponentsL();
	if (iToolBand->IsVisible())
		b->DrawNow();
	}

// (1.03) Back and Forward drawn dimmed (grey) when there is no page to go
// to. They still press, and say so, as EIKON's buttons do (DoCommandL).
void CPwAppUi::ShowNavStateL()
	{
	PwShared* s = iView ? iView->Shared() : NULL;
	if (!s || !iToolBand || !iToolBand->IsVisible())
		return;
	TBool on = iView->EngineRunning() || iView->Asleep();
	TInt back = on && s->can_back ? 1 : 0;
	TInt fwd = on && s->can_forward ? 1 : 0;
	if (back != iBackShown)
		{
		iBackShown = back;
		BandPictureL(EPwCmdBack, back ? EMbmBandBack : EMbmBandBackDim);
		}
	if (fwd != iForwardShown)
		{
		iForwardShown = fwd;
		BandPictureL(EPwCmdForward, fwd ? EMbmBandForward : EMbmBandForwardDim);
		}
	}

// ----- (0.87) the progress bar along the address ------------------------------

void CPwBar::ConstructL(const CCoeControl& aParent)
	{
	CreateWindowL(&aParent);
	MakeVisible(EFalse);
	ActivateL();
	}

void CPwBar::Draw(const TRect& /*aRect*/) const
	{
	CWindowGc& gc = SystemGc();
	TRect r(Rect());
	TInt done = r.Width() * iValue / 100;
	gc.SetPenStyle(CGraphicsContext::ENullPen);
	gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	// (1.03) done in the highlight colour (Coda's navy, the theme's; black
	// in 16 greys), the rest a pale track, both from the cube (style sheet
	// 2.4): it was 555555 on white
	gc.SetBrushColor(iEikonEnv->ControlColor(EEikColorControlHighlightBackground, *this));
	gc.DrawRect(TRect(r.iTl, TSize(done, r.Height())));
	gc.SetBrushColor(TRgb(0xcc, 0xcc, 0xcc));
	gc.DrawRect(TRect(TPoint(r.iTl.iX + done, r.iTl.iY), r.iBr));
	}

// Ctrl+L or a tap on the address: type an address (or words to search
// for). The whole address is chosen, so typing replaces it.
void CPwAppUi::StartAddressL()
	{
	if (!iAddress || !iToolBand->IsVisible())
		return;
	if (!iEditing)
		{
		iEditing = ETrue;
		// (not SetFocus: the toolband makes its controls non-focusing.
		// The view hands the keys over through AddressKeyL, and the
		// address shows its cursor and selection as if it had the focus)
		ShowAddressCursorL(ETrue);
		iAddress->SelectAllL();
		iAddress->DrawNow();
		}
	}

void CPwAppUi::ShowAddressCursorL(TBool aShow)
	{
	CTextView* tv = iAddress ? iAddress->TextView() : NULL;
	if (!tv)
		return;
	tv->SetCursorVisibilityL(TCursor::EFCursorInvisible, aShow ? TCursor::EFCursorFlashing : TCursor::EFCursorInvisible);
	tv->SetSelectionVisibilityL(aShow);
	}

// the address shows the page again; aGo: first, go where it says
void CPwAppUi::EndAddressL(TBool aGo)
	{
	if (!iEditing || !iAddress)
		return;
	TBuf<PW_URL_MAX> url;
	iAddress->GetText(url);
	url.Trim();
	iEditing = EFalse;
	iAddress->ClearSelectionL();
	ShowAddressCursorL(EFalse);
	if (aGo && url.Length() > 0)
		{
		iShownUrl = url;                 // (until the engine says where it went)
		GoL(url);
		}
	else
		{
		iAddressStale = ETrue;           // the page's own address back
		ChromeTick();
		}
	}

// (0.94) Edit > Cut, Copy and Paste (Ctrl+X, C, V; the Clipboard icon beside
// the screen): on the address while it is typed in. Paste with no address
// being typed: into the box on the page that has the cursor, else into the
// address (all of it chosen first, so Enter goes there).
void CPwAppUi::ClipboardL(TInt aCommand)
	{
	if (iEditing && iAddress)
		{
		TBool sel = iAddress->SelectionLength() > 0;
		if (aCommand == EEikCmdEditCut || aCommand == EEikCmdEditCopy)
			{
			if (!sel)
				{
				iEikonEnv->InfoMsg(aCommand == EEikCmdEditCut ? _L("Nothing to cut") : _L("Nothing to copy"));
				return;
				}
			iAddress->ClipboardL(aCommand == EEikCmdEditCut ? CEikEdwin::ECut : CEikEdwin::ECopy);
			if (aCommand == EEikCmdEditCopy)
				iEikonEnv->InfoMsg(_L("Copied"));
			}
		else
			iAddress->ClipboardL(CEikEdwin::EPaste);
		iAddress->DrawNow();
		return;
		}
	if (aCommand != EEikCmdEditPaste)
		{
		iEikonEnv->InfoMsg(aCommand == EEikCmdEditCut ? _L("Nothing to cut") : _L("Nothing to copy"));
		return;
		}
	PwShared* s = iView->Shared();
	if (s && iView->EngineRunning() && s->field_active)
		{
		PasteToPageL();
		return;
		}
	if (!iAddress || !iToolBand->IsVisible())
		{
		iEikonEnv->InfoMsg(_L("Nothing to paste into - Shift+Ctrl+T shows the address"));
		return;
		}
	StartAddressL();
	if (iEditing)
		{
		iAddress->ClipboardL(CEikEdwin::EPaste);
		iAddress->DrawNow();
		}
	}

void CPwAppUi::SelectAllAddressL()
	{
	if (!iEditing || !iAddress)
		return;
	iAddress->SelectAllL();
	iAddress->DrawNow();
	}

// the clipboard's text, or KErrNotFound
static TInt ClipboardText(RFs& aFs, TDes& aText)
	{
	aText.Zero();
	CClipboard* cb = NULL;
	TRAPD(r, cb = CClipboard::NewForReadingL(aFs));
	if (r != KErrNone)
		return KErrNotFound;
	CPlainText* t = NULL;
	TRAP(r, t = CPlainText::NewL());
	if (r == KErrNone)
		TRAP(r, t->PasteFromStoreL(cb->Store(), cb->StreamDictionary(), 0));
	if (r == KErrNone)
		t->Extract(aText, 0, Min(t->DocumentLength(), aText.MaxLength()));
	delete t;
	delete cb;
	return r == KErrNone && aText.Length() ? KErrNone : KErrNotFound;
	}

// a box on the page has the cursor: the clipboard's text typed into it, as
// keys (the engine has no clipboard of its own), as much as the key queue
// holds; a line break ends it
void CPwAppUi::PasteToPageL()
	{
	TBuf<PW_EVQ> text;
	if (ClipboardText(iCoeEnv->FsSession(), text) != KErrNone)
		{
		iEikonEnv->InfoMsg(_L("Nothing to paste"));
		return;
		}
	PwShared* s = iView->Shared();
	TInt room = PW_EVQ - (s->ev_head - s->ev_tail) - 4;
	TInt n = 0;
	for (; n < text.Length() && n < room; n++)
		{
		TUint c = text[n];
		if (c == CEditableText::EParagraphDelimiter || c == CEditableText::ELineBreak || c < 0x20)
			break;
		iView->Key(c, 0);
		}
	if (n < text.Length())
		iEikonEnv->InfoMsg(_L("Only the first line pasted"));
	}

// the address of the page showing (none for PsiWeb's own pages), as the
// toolband shows it, whether the toolband is shown or not
static void PageAddress(CPwView* aView, TDes& aUrl)
	{
	aUrl.Zero();
	PwShared* s = aView ? aView->Shared() : NULL;
	if (!s || !(aView->EngineRunning() || aView->Asleep()))
		return;
	FromUtf8(aUrl, s->url);
	if (aUrl.Compare(_L("about:problem")) == 0)
		FromUtf8(aUrl, s->problem_url);
	else if (Clip(aUrl, 6).Compare(_L("about:")) == 0)
		aUrl.Zero();
	}

// Edit > Copy page address: the address of the page showing, to the
// clipboard, for another program
void CPwAppUi::CopyAddressL()
	{
	TBuf<PW_URL_MAX> url;
	PageAddress(iView, url);
	if (url.Length() == 0)
		{
		iEikonEnv->InfoMsg(_L("No page address to copy"));
		return;
		}
	CClipboard* cb = CClipboard::NewForWritingLC(iCoeEnv->FsSession());
	CPlainText* t = CPlainText::NewL();
	CleanupStack::PushL(t);
	t->InsertL(0, url);
	t->CopyToStoreL(cb->Store(), cb->StreamDictionary(), 0, url.Length());
	cb->CommitL();
	CleanupStack::PopAndDestroy(2);     // t, cb
	iEikonEnv->InfoMsg(_L("Address copied"));
	}

// keys while the address is being typed in: Enter goes, Esc leaves it;
// the menu's shortcuts still work (they never reach here)
TBool CPwAppUi::AddressKeyL(const TKeyEvent& aKeyEvent, TEventCode aType)
	{
	if (!iEditing || !iAddress)
		return EFalse;
	if (aType != EEventKey)
		return ETrue;
	switch (aKeyEvent.iCode)
		{
	case EKeyEnter:
		// (a held Enter, still repeating from the key that opened
		// PsiWeb or chose a link, does not go anywhere)
		if (aKeyEvent.iRepeats == 0)
			EndAddressL(ETrue);
		return ETrue;
	case EKeyEscape:
		EndAddressL(EFalse);
		return ETrue;
	case EKeyUpArrow:
	case EKeyDownArrow:
	case EKeyPageUp:
	case EKeyPageDown:
	case EKeyTab:
		return ETrue;
		}
	iAddress->OfferKeyEventL(aKeyEvent, aType);
	// (0.87) a letter typed: the first favourite or recent address that
	// begins the same way is offered after it, chosen, so typing on
	// replaces it and Enter takes it
	if (aKeyEvent.iCode >= 0x20 && aKeyEvent.iCode < 0x100 && aKeyEvent.iCode != 0x7f)
		CompleteAddressL();
	return ETrue;
	}

// (0.87) Address completion: an address without its "http://", "https://"
// or "www." that begins with what was typed (in any case)
static TPtrC Bare(const TDesC& aUrl)
	{
	TPtrC u(aUrl);
	if (Clip(u, 8).CompareF(_L("https://")) == 0) u.Set(u.Mid(8));
	else if (Clip(u, 7).CompareF(_L("http://")) == 0) u.Set(u.Mid(7));
	if (Clip(u, 4).CompareF(_L("www.")) == 0) u.Set(u.Mid(4));
	return u;
	}

static TBool Offers(const TDesC& aUrl, const TDesC& aTyped, TPtrC& aShown)
	{
	TPtrC full(aUrl), bare(Bare(aUrl));
	if (aTyped.Length() < full.Length() && Clip(full, aTyped.Length()).CompareF(aTyped) == 0)
		{
		aShown.Set(full);
		return ETrue;
		}
	if (aTyped.Length() < bare.Length() && Clip(bare, aTyped.Length()).CompareF(aTyped) == 0)
		{
		aShown.Set(bare);
		return ETrue;
		}
	return EFalse;
	}

void CPwAppUi::CompleteAddressL()
	{
	if (!iAddress || !iFavs || !iHist)
		return;
	TBuf<PW_URL_MAX> typed;
	iAddress->GetText(typed);
	if (typed.Length() < 2 || iAddress->CursorPos() != typed.Length() || typed.Locate(' ') >= 0)
		return;                              // (words to search for: nothing offered)
	TPtrC shown;
	TBool found = EFalse;
	for (TInt i = 0; i < iFavs->Count() && !found; i++)
		found = Offers(iFavs->At(i).iUrl, typed, shown);
	for (TInt j = 0; j < iHist->Count() && !found; j++)
		found = Offers(iHist->At(j).iUrl, typed, shown);
	if (!found)
		return;
	TBuf<PW_URL_MAX> all(typed);
	all.Append(Clip(shown.Mid(typed.Length()), all.MaxLength() - all.Length()));   // (Left() longer than the text panics: USER 22)
	iAddress->SetTextL(&all);
	iAddress->SetSelectionL(all.Length(), typed.Length());
	iAddress->DrawNow();
	}

// the address asks for the focus (a tap on it)
void CPwAppUi::HandleControlEventL(CCoeControl* aControl, TCoeEvent aEventType)
	{
	if (aControl == iAddress && aEventType == EEventRequestFocus)
		StartAddressL();
	}


CPwAppUi::~CPwAppUi()
	{
	PsiMemUnregister(iMemChunk, KUidPsiWeb.iUid);
	iReading.Leave(iCoeEnv->FsSession(), KUidPsiWeb);   // the user's contrast and backlight back
	if (iView)
		{
		RemoveFromStack(iView);
		delete iView;
		}
	delete iFavs;
	SaveHistory();                           // (0.96) what the last minute added
	delete iHist;
	delete iSetupIdle;
	delete iOfferIdle;
	delete iBar;
	}

void CPwAppUi::LoadSettings(TPwSettings& aSettings)
	{
	aSettings.iBaudIndex = 4;          // 115200, as PsiTerm recommends
	aSettings.iRtsCts = 0;
	aSettings.iNetMode = 0;
	aSettings.iPppStart.Copy(_L("ATDT777"));
	aSettings.iUseProxy = 0;
	aSettings.iProxyHost.Zero();
	aSettings.iProxyPort = 8080;
	aSettings.iHome = KDefaultHome;
	aSettings.iImages = 0;            // pictures only when asked (View > Show pictures)
	aSettings.iZoom = 100;
	// (0.78) a tall screen (the netBook) starts with the toolband along the
	// top; (0.99) and the toolbar, as every program of the family opens
	aSettings.iToolbar = 1;
	aSettings.iToolband = TallScreen() ? 1 : 0;
	aSettings.iDisplay = 0;           // sharp text, Reading mode off (docs/display.md)
	aSettings.iSearch = PW_SEARCH_DDG;
	aSettings.iStyle = -1;            // (0.86) the route's standard
	aSettings.iFlags = 0;
	aSettings.iFullServer.Zero();     // (0.88) none until chosen
	aSettings.iFullCommand = TallScreen() ? _L("carbonyl") : _L("w3m");   // (the 5mx's screen suits w3m)
	RFile file;
	if (file.Open(iCoeEnv->FsSession(), KIniFile, EFileRead) != KErrNone)
		{
		UseSharedLink(iCoeEnv->FsSession(), aSettings);
		// (0.85) a new PsiWeb on a tall screen (the netBook) with the network
		// card or dial-up route: pictures on every page, as the line is fast;
		// on a modem only when asked, as before
		aSettings.iImages = TallScreen() && aSettings.iNetMode && !aSettings.iUseProxy;
		return;
		}
	TBuf8<400> d;
	TInt tallZoom = -1;                  // (0.89) the tall screen's zoom byte: -1 an older file
	if (file.Read(d) == KErrNone && d.Length() >= 10 && d[0] == 1)
		{
		TInt pos = 1;
		aSettings.iBaudIndex = d[pos++] % 5;
		aSettings.iRtsCts = d[pos++] != 0;
		aSettings.iNetMode = d[pos++] != 0;
		aSettings.iUseProxy = d[pos++] != 0;
		aSettings.iProxyPort = d[pos] | (d[pos + 1] << 8); pos += 2;
		pos++;               // (NetSurf's pictures setting, up to 0.61: see below)
		aSettings.iZoom = d[pos++];
		if (aSettings.iZoom < 30) aSettings.iZoom = 100;
		TInt len = d[pos++];
		if (pos + len <= d.Length() && len <= 60)
			{
			aSettings.iProxyHost.Copy(d.Mid(pos, len));
			pos += len;
			if (pos < d.Length())
				{
				len = d[pos++];
				if (pos + len <= d.Length() && len <= 200)
					{
					aSettings.iHome.Copy(d.Mid(pos, len));
					pos += len;
					if (pos < d.Length())            // (0.55) after the home page
						aSettings.iToolbar = d[pos++] != 0;
					// (0.62) pictures on every page: a new byte, so that
					// NetSurf's setting (on by default) does not carry over
					if (pos < d.Length())
						aSettings.iImages = d[pos++] != 0;
					if (pos < d.Length())            // (0.81) display: Reading mode, text
						aSettings.iDisplay = d[pos++] & (KPwDisplayReading | KPwDisplayScaledText | KPwKeepLog);
					if (pos < d.Length())            // (0.78) the toolband: bit 0 shown, bit 1 chosen
						{
						TInt band = d[pos++];
						if (band & 2)
							aSettings.iToolband = band & 1;
						}
					if (pos < d.Length())            // (0.85) Search with
						aSettings.iSearch = d[pos++] == PW_SEARCH_FROGFIND ? PW_SEARCH_FROGFIND : PW_SEARCH_DDG;
					if (pos < d.Length())            // (0.86) Page style: 0 the route's standard, else style + 1
						{
						TInt b = d[pos++];
						aSettings.iStyle = b >= 1 && b <= 4 ? b - 1 : -1;
						}
					if (pos < d.Length())            // (0.85) KPwFlag* bits
						aSettings.iFlags = d[pos++] & (KPwFlagKeepCookies | KPwFlagStyleAsked);
					// (0.88) Open in full browser: the server, the program
					if (pos < d.Length() && pos + 1 + d[pos] <= d.Length() && d[pos] <= 24)
						{
						len = d[pos++];
						aSettings.iFullServer.Copy(d.Mid(pos, len));
						pos += len;
						if (pos < d.Length() && pos + 1 + d[pos] <= d.Length() && d[pos] <= 40 && d[pos] > 0)
							{
							len = d[pos++];
							aSettings.iFullCommand.Copy(d.Mid(pos, len));
							pos += len;
							}
						// (0.89) the netBook's (a tall screen's) zoom, after the
						// full browser's two texts (0: none chosen yet)
						if (pos < d.Length())
							tallZoom = d[pos++];
						}
					}
				}
			}
		}
	file.Close();
	// (0.89) Zoom is remembered per machine: the old byte is the 5mx's (a
	// short screen's), the new one the netBook's, so PsiWeb.ini restored
	// from the other machine does not bring its size along. A file older
	// than 0.89 has only the one, for the machine that wrote it.
	pw_zoom_load(TallScreen(), aSettings.iZoom, tallZoom, &aSettings.iZoom, &iOtherZoom);
	UseSharedLink(iCoeEnv->FsSession(), aSettings);
	}

void CPwAppUi::SaveSettings(const TPwSettings& aSettings)
	{
	RFs& fs = iCoeEnv->FsSession();
	SaveSharedLink(fs, aSettings);
	fs.MkDirAll(KIniFile);
	// (0.81) to a temporary file, then renamed over the old one, so a full
	// disk or a switch-off part way never leaves half a settings file
	TFileName tmpName(KIniFile);
	tmpName.Append('~');
	RFile file;
	if (file.Replace(fs, tmpName, EFileWrite) != KErrNone)
		return;
	TBuf8<400> d;
	d.Append(1);                         // version
	d.Append((TUint8)aSettings.iBaudIndex);
	d.Append((TUint8)aSettings.iRtsCts);
	d.Append((TUint8)aSettings.iNetMode);
	d.Append((TUint8)aSettings.iUseProxy);
	d.Append((TUint8)(aSettings.iProxyPort & 0xff));
	d.Append((TUint8)(aSettings.iProxyPort >> 8));
	d.Append((TUint8)aSettings.iImages);
	// (0.89) the 5mx's zoom here, the netBook's at the end (LoadSettings)
	TInt shortZoom, tallZoom;
	pw_zoom_save(TallScreen(), aSettings.iZoom, iOtherZoom, &shortZoom, &tallZoom);
	d.Append((TUint8)shortZoom);
	TBuf8<200> tmp;
	tmp.Copy(aSettings.iProxyHost);
	d.Append((TUint8)tmp.Length());
	d.Append(tmp);
	tmp.Copy(aSettings.iHome);
	d.Append((TUint8)tmp.Length());
	d.Append(tmp);
	d.Append((TUint8)aSettings.iToolbar);
	d.Append((TUint8)aSettings.iImages);    // (0.62)
	d.Append((TUint8)aSettings.iDisplay);   // (0.81)
	d.Append((TUint8)(2 | (aSettings.iToolband ? 1 : 0)));   // (0.78)
	d.Append((TUint8)aSettings.iSearch);    // (0.85)
	d.Append((TUint8)(aSettings.iStyle >= 0 ? aSettings.iStyle + 1 : 0));     // (0.86)
	d.Append((TUint8)aSettings.iFlags);     // (0.85)
	tmp.Copy(aSettings.iFullServer);        // (0.88)
	d.Append((TUint8)tmp.Length());
	d.Append(tmp);
	tmp.Copy(aSettings.iFullCommand);
	d.Append((TUint8)tmp.Length());
	d.Append(tmp);
	d.Append((TUint8)tallZoom);             // (0.89)
	TInt r = file.Write(d);
	if (r == KErrNone)
		r = file.Flush();
	file.Close();
	if (r == KErrNone)
		r = fs.Replace(tmpName, KIniFile);
	if (r != KErrNone)
		fs.Delete(tmpName);
	else
		{
		PsiKeepFile(fs, KIniFile);           // (0.95) and the card's copy
		PsiKeepFile(fs, KPsiLinkFile);
		}
	}

void CPwAppUi::DynInitMenuPaneL(TInt aMenuId, CEikMenuPane* aMenuPane)
	{
	PwShared* s = iView->Shared();
	if (aMenuId == R_PW_FAVS_MENU)
		FavsMenuL(aMenuPane);
	else if (aMenuId == R_PW_HISTORY_MENU)
		HistoryMenuL(aMenuPane);
	else if (aMenuId == R_PW_GO_MENU)
		{
		aMenuPane->SetItemDimmed(EPwCmdBack, !s->can_back);
		aMenuPane->SetItemDimmed(EPwCmdForward, !s->can_forward);
		}
	else if (aMenuId == R_PW_FILE_MENU)
		{
		aMenuPane->SetItemDimmed(EPwCmdStop, !s->busy);
		aMenuPane->SetItemDimmed(EPwCmdConnect, s->busy);   // (a page is using the link)
		// (0.73) Psion Internet: Disconnect while the link is up (the modem
		// route's stays as it was: the engine frees the port)
		aMenuPane->SetItemDimmed(EPwCmdHangup, iView->Settings().iNetMode && !PgLinkUp());
		}
	else if (aMenuId == R_PW_EDIT_MENU)
		{
		// (0.94) Cut and Copy work on the address being typed in
		TBool sel = iEditing && iAddress && iAddress->SelectionLength() > 0;
		aMenuPane->SetItemDimmed(EEikCmdEditCut, !sel);
		aMenuPane->SetItemDimmed(EEikCmdEditCopy, !sel);
		TBuf<PW_URL_MAX> url;
		PageAddress(iView, url);
		aMenuPane->SetItemDimmed(EPwCmdCopyAddress, url.Length() == 0);
		}
	else if (aMenuId == R_PW_LOG_MENU)
		{
		// dimmed, not gone, until a proxy is set: the log goes to it;
		// (0.77) and while no log is kept
		const TPwSettings& st = iView->Settings();
		aMenuPane->SetItemDimmed(EPwCmdSendLog,
			!(st.iDisplay & KPwKeepLog) || !st.iUseProxy || st.iProxyHost.Length() == 0);
		aMenuPane->SetItemButtonState(EPwCmdKeepLog, (st.iDisplay & KPwKeepLog) ? EEikMenuItemSymbolOn : 0);
		}
	else if (aMenuId == R_PW_STYLE_MENU)
		aMenuPane->SetItemButtonState(EPwCmdStyleFull + StyleNow(iView->Settings()), EEikMenuItemSymbolOn);
	else if (aMenuId == R_PW_SHOW_MENU)
		{
		aMenuPane->SetItemButtonState(EPwCmdToggleToolbar,
			iView->Settings().iToolbar ? EEikMenuItemSymbolOn : 0);
		aMenuPane->SetItemButtonState(EPwCmdToggleToolband,
			iView->Settings().iToolband ? EEikMenuItemSymbolOn : 0);
		aMenuPane->SetItemButtonState(EPwCmdReading,
			(iView->Settings().iDisplay & KPwDisplayReading) ? EEikMenuItemSymbolOn : 0);
		}
	else if (aMenuId == R_PW_VIEW_MENU)
		aMenuPane->SetItemButtonState(EPwCmdImages, iView->PicturesShown() ? EEikMenuItemSymbolOn : 0);
	}

// a dimmed command chosen: say why (Tools > Troubleshooting > Send log to helper)
void CPwAppUi::HandleAttemptDimmedSelectionL(TInt aCommandId)
	{
	if (aCommandId != EPwCmdSendLog)
		{
		CEikAppUi::HandleAttemptDimmedSelectionL(aCommandId);
		return;
		}
	if (!(iView->Settings().iDisplay & KPwKeepLog))
		iEikonEnv->InfoMsg(_L("No log kept - tick Tools > Troubleshooting > Keep a log"));
	else
		iEikonEnv->InfoMsg(_L("Not available - set a web helper in Connection settings"));
	}

// cuts aText to aWidth pixels of aFont, ending in "..." (one character on
// the Psion), as the style guide has long names shown
static void Ellipsis(TDes& aText, const CFont& aFont, TInt aWidth)
	{
	if (aFont.TextWidthInPixels(aText) <= aWidth)
		return;
	const TText KEllipsis = 0x85;
	TInt n = aText.Length();
	while (n > 0)
		{
		n--;
		if (aFont.TextWidthInPixels(aText.Left(n)) + aFont.CharWidthInPixels(KEllipsis) <= aWidth)
			break;
		}
	aText.SetLength(n);
	aText.Append(KEllipsis);
	}

// (0.85) The route in the user's words (core/docs/setup-assistant.md);
// (0.99) the family's words (style sheet 2.13): "By modem, 115200 baud" or
// "By network card or dial-up", then the web helper if there is one: "By
// modem, 115200 baud, through the web helper" (the modem's own helper), "By
// network card or dial-up, through the web helper at 192.168.1.20" - the
// start page, Page information
void RouteWords(const TPwSettings& aS, TDes& aOut)
	{
	static const TInt KBaud[] = { 9600, 19200, 38400, 57600, 115200 };
	TBuf8<64> h;
	h.Copy(Clip(aS.iProxyHost, 60));
	h.ZeroTerminate();
	TBool helper = aS.iUseProxy && aS.iProxyHost.Length() > 0;
	if (aS.iNetMode)
		aOut.Copy(_L("By network card or dial-up"));
	else
		aOut.Format(_L("By modem, %d baud"), KBaud[aS.iBaudIndex >= 0 && aS.iBaudIndex < 5 ? aS.iBaudIndex : 4]);
	if (!helper)
		return;
	if (pg_su_is_atom_host((const char*)h.Ptr()))
		aOut.Append(_L(", through the web helper"));
	else
		{
		aOut.Append(_L(", through the web helper at "));
		aOut.Append(Clip(aS.iProxyHost, aOut.MaxLength() - aOut.Length() - 6));
		if (aS.iProxyPort != PG_SU_PSIPROXY_PORT)
			aOut.AppendFormat(_L(":%d"), aS.iProxyPort);
		}
	}

// View > Page information: the title, the address, what the engine says,
// memory, the route. Each line is cut to fit beside its prompt.
void CPwAppUi::PageInfoL()
	{
	PwShared* s = iView->Shared();
	TBuf<160> lines[6];
	FromUtf8(lines[0], s->title);
	lines[0].Trim();
	if (lines[0].Length() == 0)
		lines[0] = _L("(no title)");
	FromUtf8(lines[1], s->url);
	if (lines[1].Compare(_L("about:problem")) == 0)
		FromUtf8(lines[1], s->problem_url);
	else if (Clip(lines[1], 6).Compare(_L("about:")) == 0)
		lines[1] = _L("(PsiWeb's own page)");
	FromUtf8(lines[2], s->status);
	lines[2].Trim();
	if (lines[2].Length() == 0)
		lines[2] = _L("-");
	TMemoryInfoV1Buf mem;
	UserHal::MemoryInfo(mem);
	// (0.99) MB with one decimal, as the toolbar shows the free memory
	TInt kb = mem().iFreeRamInBytes / 1024;
	lines[3].Format(_L("%d.%d MB"), kb / 1024, (kb % 1024) * 10 / 1024);
	// (0.86) the route and how secure the page is
	RouteWords(iView->Settings(), lines[4]);
	TBool https = Clip(lines[1], 6).Compare(_L("https:")) == 0;
	if (https && s->page_secure == PW_SECURE_CHECKED)
		lines[4].Append(TallScreen() ? _L(" - secure, checked by the netBook") : _L(" - secure, checked by the Series 5mx"));
	else if (https && s->page_secure == PW_SECURE_MODEM)
		lines[4].Append(_L(" - secure, checked by the modem"));   // (0.90) the Atom made the TLS connection
	else if (https && s->page_secure == PW_SECURE_HELPER)
		lines[4].Append(_L(" - secure, checked by the web helper"));
	else if (Clip(lines[1], 5).Compare(_L("http:")) == 0)
		lines[4].Append(_L(" - not secure (http)"));
	// (0.86) the page's style and size, and its pictures
	static const TText* const KNames[] = { _S("Full"), _S("Lighter"), _S("Reader"), _S("Text only") };
	TInt style = s->page_style >= 0 && s->page_style <= 3 ? s->page_style : 0;
	lines[5].Copy(TPtrC(KNames[style]));
	if (s->page_bytes > 0)
		{
		if (style != PW_STYLE_FULL && s->page_kept != s->page_bytes)
			lines[5].AppendFormat(_L(" - %d KB of %d KB kept"), (s->page_kept + 1023) / 1024, (s->page_bytes + 1023) / 1024);
		else
			lines[5].AppendFormat(_L(" - %d KB"), (s->page_bytes + 1023) / 1024);
		}
	if (s->page_pics_out > 0)
		lines[5].AppendFormat(s->page_pics_out == 1 ? _L(", 1 picture left out") : _L(", %d pictures left out"), s->page_pics_out);
	if (s->page_scripts)
		lines[5].Append(_L(", drawn by scripts"));
	// the prompts take about 120 px of the 640, the frame and margins more:
	// the dialog's labels are bold, which the normal font's widths allow for.
	// 300 px keeps it inside the size of Psion's own dialogs (about 370-560).
	const CFont* font = iEikonEnv->NormalFont();
	TPtrC ptrs[6];
	for (TInt i = 0; i < 6; i++)
		{
		Ellipsis(lines[i], *font, 300);
		ptrs[i].Set(lines[i]);
		}
	CPwInfoDialog* dlg = new(ELeave) CPwInfoDialog(ptrs, 6);
	dlg->ExecuteLD(R_PW_INFO_DIALOG);
	}

// (0.99) the route is in Page information (Shift+Ctrl+Q), not in About
void CPwAppUi::AboutL()
	{
	CPwAboutDialog* dlg = new(ELeave) CPwAboutDialog(TallScreen());
	dlg->ExecuteLD(R_PW_ABOUT_DIALOG);
	}


// Tools > Update PsiWeb: where from (Update.ini, in PsiMail's words:
// "github", "github-dev" or "host:port" - the engine reads it), then the
// engine fetches and checks the new version
void CPwAppUi::UpdateL()
	{
	if (iView->Updating())
		{
		iEikonEnv->InfoMsg(_L("Not available while updating"));
		return;
		}
	RFs& fs = iCoeEnv->FsSession();
	TBuf<60> src;
	RFile f;
	if (f.Open(fs, KUpdIniFile, EFileRead) == KErrNone)
		{
		TBuf8<60> b;
		f.Read(b);
		f.Close();
		src.Copy(b);
		src.Trim();
		}
	TInt source = 0, port = 8686;
	TBuf<50> host;
	if (src.CompareF(_L("github-dev")) == 0)
		source = 1;
	else if (src.Length() && src.CompareF(_L("github")) != 0)
		{
		source = 2;
		TInt c = src.Locate(':');
		host.Copy(src.Left(c >= 0 ? (c < 50 ? c : 50) : (src.Length() < 50 ? src.Length() : 50)));
		if (c >= 0)
			{
			TLex lex(src.Mid(c + 1));
			if (lex.Val(port) != KErrNone || port <= 0) port = 8686;
			}
		}
	CPwUpdateDialog* dlg = new(ELeave) CPwUpdateDialog(source, host, port);
	if (!dlg->ExecuteLD(R_PW_UPDATE_DIALOG))
		return;
	if (source == 0) src = _L("github");
	else if (source == 1) src = _L("github-dev");
	else
		{
		src = host;
		src.AppendFormat(_L(":%d"), port);
		}
	fs.MkDirAll(KUpdIniFile);
	if (f.Replace(fs, KUpdIniFile, EFileWrite) == KErrNone)
		{
		TBuf8<60> b;
		b.Copy(src);
		f.Write(b);
		f.Close();
		}
	PsiKeepFile(fs, KUpdIniFile);            // (0.95)
	iView->StartUpdateL();
	}

// (0.94) the connection settings the engine reads at its start
// (1.03) the link itself (not the web helper): a change needs the engine
// started again, which hangs the link up and dials afresh
static TBool LinkChanged(const TPwSettings& aWas, const TPwSettings& aNow)
	{
	return aWas.iBaudIndex != aNow.iBaudIndex || aWas.iRtsCts != aNow.iRtsCts
		|| aWas.iNetMode != aNow.iNetMode || aWas.iPppStart.Compare(aNow.iPppStart) != 0;
	}

static TBool HelperChanged(const TPwSettings& aWas, const TPwSettings& aNow)
	{
	return (aWas.iUseProxy != 0) != (aNow.iUseProxy != 0)
		|| (aNow.iUseProxy && (aWas.iProxyHost.CompareF(aNow.iProxyHost) != 0 || aWas.iProxyPort != aNow.iProxyPort));
	}


// (1.00) while a command runs (a dialog up) PsiWeb is busy for the memory
// watcher: not put to sleep or closed then
void CPwAppUi::HandleCommandL(TInt aCommand)
	{
	iCommands++;
	SetMemBusy(iViewBusy);
	CleanupStack::PushL(TCleanupItem(CommandDone, this));
	DoCommandL(aCommand);
	CleanupStack::PopAndDestroy();
	}

void CPwAppUi::CommandDone(TAny* aSelf)
	{
	CPwAppUi* ui = (CPwAppUi*)aSelf;
	ui->iCommands--;
	ui->SetMemBusy(ui->iViewBusy);
	}

void CPwAppUi::SetMemBusy(TBool aViewBusy)
	{
	iViewBusy = aViewBusy;
	TBool busy = iViewBusy || iCommands > 0;
	if (busy != iMemBusyNow)
		{
		iMemBusyNow = busy;
		PsiMemBusy(iMemChunk, KUidPsiWeb.iUid, busy);
		}
	}

void CPwAppUi::DoCommandL(TInt aCommand)
	{
	TPwSettings& st = iView->Settings();
	PwShared* sh = iView->Shared();
	// the Zoom in and Zoom out icons beside the screen come as EIKON's own
	// zoom commands (or as their keys: CPwView::OfferKeyEventL)
	if (aCommand == EEikCmdZoomIn) aCommand = EPwCmdZoomIn;
	else if (aCommand == EEikCmdZoomOut) aCommand = EPwCmdZoomOut;
	else if (aCommand == EEikCmdZoomNormal) aCommand = EPwCmdZoomNormal;
	// any other command leaves the address as it was (a button, a menu);
	// (0.94) not the clipboard's, which work on it
	if (iEditing && aCommand != EPwCmdAddress && aCommand != EEikCmdEditCut
		&& aCommand != EEikCmdEditCopy && aCommand != EEikCmdEditPaste)
		EndAddressL(EFalse);
	switch (aCommand)
		{
	case EEikCmdExit:
		iView->StopEngine();
		Exit();
		break;
	case EPwCmdOpen:
		{
			TBuf<500> url;
		FromUtf8(url, sh->url);
		if (url.Compare(_L("about:problem")) == 0)
			FromUtf8(url, sh->problem_url);
		else if (Clip(url, 6).Compare(_L("about:")) == 0)
			url.Zero();                      // (0.85) never an internal name
		CPwOpenDialog* dlg = new(ELeave) CPwOpenDialog(url, *iHist);
		if (dlg->ExecuteLD(R_PW_OPEN_DIALOG) && url.Length() > 0)
			GoL(url);
		break;
		}
	case EPwCmdHome:
		iView->Command(PW_CMD_HOME, KNullDesC);
		break;
	case EPwCmdReload:
		iView->Command(PW_CMD_RELOAD, KNullDesC);
		break;
	case EPwCmdReloadStop:
		// (0.87) the toolband's one button: Stop while a page loads, else Reload
		iView->Command(iView->EngineRunning() && sh->busy ? PW_CMD_STOP : PW_CMD_RELOAD, KNullDesC);
		break;
	case EPwCmdStop:
		// dimmed on the menu when nothing is loading, but the key still arrives
		if (iView->EngineRunning() && !sh->busy)
			iEikonEnv->InfoMsg(_L("Nothing to stop"));
		else
			iView->Command(PW_CMD_STOP, KNullDesC);
		break;
	case EPwCmdBack:
		// the toolbar button and the key arrive whatever the history holds
		if (iView->EngineRunning() && !sh->can_back)
			iEikonEnv->InfoMsg(_L("Nothing to go back to"));
		else
			iView->Command(PW_CMD_BACK, KNullDesC);
		break;
	case EPwCmdForward:
		if (iView->EngineRunning() && !sh->can_forward)
			iEikonEnv->InfoMsg(_L("Nothing to go forward to"));
		else
			iView->Command(PW_CMD_FORWARD, KNullDesC);
		break;
	case EPwCmdTop:
		iView->Command(PW_CMD_TOP, KNullDesC);
		break;
	case EPwCmdBottom:
		iView->Command(PW_CMD_BOTTOM, KNullDesC);
		break;
	case EPwCmdZoomIn:
	case EPwCmdZoomOut:
	case EPwCmdZoomNormal:
		{
		// Zoom in and out cycle round the sizes (style guide 7.3.7): past the
		// largest comes the smallest, and the other way
		TInt i = 0;
		while (i < KZoomCount - 1 && KZoomSteps[i] < st.iZoom)
			i++;
		if (aCommand == EPwCmdZoomIn) i = (i + 1) % KZoomCount;
		if (aCommand == EPwCmdZoomOut) i = (i + KZoomCount - 1) % KZoomCount;
		st.iZoom = aCommand == EPwCmdZoomNormal ? 100 : KZoomSteps[i];
		TBuf<8> z;
		z.Num(st.iZoom);
		iView->Command(PW_CMD_ZOOM, z);
		// (0.99) the page shows the new size, as Psion's own programs say
		// nothing; Normal size, which may change nothing, says what it is
		if (aCommand == EPwCmdZoomNormal)
			iEikonEnv->InfoMsg(_L("Normal size - 100%"));
		SaveSettings(st);
		break;
		}
	case EPwCmdImages:
		{
		// View > Show pictures (a tick box) and the Pictures button: the
		// pictures of the page showing, on or off (pictures on every page is
		// in Preferences). The button shows what the page has come to.
		TBool on = iView->PicturesShown();
		if (!iView->EngineRunning())
			iView->Command(PW_CMD_IMAGES, _L("1"));   // (says why not)
		else if (st.iImages)
			iEikonEnv->InfoMsg(_L("Pictures are already shown on every page"));
		else if (sh->page_pics)
			{
			iView->Command(PW_CMD_IMAGES, _L("0"));
			on = EFalse;
			}
		else
			{
			TBuf<16> u;
			FromUtf8(u, sh->url);
			if (u.Length() == 0 || Clip(u, 6).Compare(_L("about:")) == 0)   // (Left(6) of a shorter text panics)
				iEikonEnv->InfoMsg(_L("No pictures to show"));
			else
				{
				iView->Command(PW_CMD_IMAGES, _L("1"));
				on = ETrue;
				}
			}
		ShowPicturesState(on);
		break;
		}
	case EPwCmdToggleToolbar:
		st.iToolbar = !st.iToolbar;
		SaveSettings(st);
		ShowToolBarL(st.iToolbar);
		break;
	case EPwCmdToggleToolband:
		st.iToolband = !st.iToolband;
		SaveSettings(st);
		ShowToolBandL(st.iToolband);
		break;
	case EPwCmdAddress:
		// Ctrl+L: the toolband's address if it is shown, else Open address
		if (iToolBand && iToolBand->IsVisible() && iAddress)
			StartAddressL();
		else
			HandleCommandL(EPwCmdOpen);
		break;
	case EPwCmdReading:                      // View > Reading mode (docs/display.md)
		st.iDisplay ^= KPwDisplayReading;
		SaveSettings(st);
		UpdateReading();
		iEikonEnv->InfoMsg((st.iDisplay & KPwDisplayReading) ? _L("Bright screen on") : _L("Bright screen off"));
		break;
	case EPwCmdPageInfo:
	case EPwCmdRouteLight:
		PageInfoL();
		break;
	case EPwCmdStyleFull:
	case EPwCmdStyleLighter:
	case EPwCmdStyleReader:
	case EPwCmdStyleText:
		SetStyleL(aCommand - EPwCmdStyleFull);
		break;
	case EPwCmdStyleNext:
		SetStyleL((StyleNow(st) + 1) % 4);
		break;
	case EPwCmdStyleMenu:
		break;
	case EEikCmdEditCut:
	case EEikCmdEditCopy:
	case EEikCmdEditPaste:
		ClipboardL(aCommand);
		break;
	case EPwCmdCopyAddress:
		CopyAddressL();
		break;
	case EPwCmdAddFav:
		AddFavouriteL();
		break;
	case EPwCmdOrganiseFav:
		OrganiseL();
		break;
	case EPwCmdFavMore:
		PickAndOpenL(*iFavs);
		break;
	case EPwCmdHistoryFull:
		PickAndOpenL(*iHist);
		break;
	case EPwCmdClearHistory:
		ClearHistoryL();
		break;
	case EPwCmdHistNone:
	case EPwCmdHistory:
		break;
	case EPwCmdSetup:
		SetupL();
		break;
	case EPwCmdFullBrowser:
		FullBrowserL();
		break;
	case EPwCmdStart:
		iView->OpenUrlL(_L("about:start"));
		break;
	case EPwCmdShowMenu:
	case EPwCmdLogMenu:
		break;
	case EPwCmdConnSettings:
		{
		TPwSettings was(st);
		CPwConnDialog* dlg = new(ELeave) CPwConnDialog(st);
		if (dlg->ExecuteLD(R_PW_CONN_DIALOG))
			{
			SaveSettings(st);
			UseSharedLink(iCoeEnv->FsSession(), st);
			UpdateStartPageL();
			// (0.94) the engine started again only if the route changed:
			// a restart loses what was typed into the page's forms. (1.03)
			// Only the link itself: a new web helper is taken at once
			if (!iView->EngineRunning() || LinkChanged(iView->StartedWith(), st))
				{
				if (!iView->Asleep())
					RestartEngineL();
				}
			else if (HelperChanged(iView->StartedWith(), st))
				iView->HelperChangedL();
			}
		break;
		}
	case EPwCmdPrefs:
		{
		TPwSettings was(st);
		CPwPrefsDialog* dlg = new(ELeave) CPwPrefsDialog(st);
		if (dlg->ExecuteLD(ColourScreen() ? R_PW_PREFS_DIALOG_COLOUR : R_PW_PREFS_DIALOG))
			{
			SaveSettings(st);
			UpdateStartPageL();
			ShowPicturesState(st.iImages);   // (pictures on every page, or not)
			// (0.94) what the engine reads as it goes is handed over; it is
			// started again (losing what was typed into a form) only for
			// what it reads at its start: the text, and pictures on every page
			if (sh)
				{
				CopyToC(sh->home_url, sizeof(sh->home_url), st.iHome);
				sh->keep_cookies = (st.iFlags & KPwFlagKeepCookies) != 0;
				}
			TBool textChanged = (was.iDisplay & KPwDisplayScaledText) != (st.iDisplay & KPwDisplayScaledText);
			if (textChanged || (was.iImages != 0) != (st.iImages != 0))
				RestartEngineL();
			else if (StyleNow(was) != StyleNow(st))
				SetStyleL(StyleNow(st));
			}
		break;
		}
	case EPwCmdConnect:
		{
		// as PsiTerm's File > Connect: brings the link up now, before a page
		// needs it - the Psion's Internet dialled, or the modem checked - with
		// the shared connection settings, and says what it found
		TPsiLink link;
		link.SetDefaults();
		link.Load(iCoeEnv->FsSession());
		PgLinkConnectL(link.iBaudIndex, link.iRtsCts, link.iNetMode, link.iPppStart,
			R_PW_TEST_DIALOG, EPwDlgTest1, _L("Connect"));
		break;
		}
	case EPwCmdHangup:
		iView->Command(PW_CMD_HANGUP, KNullDesC);
		if (iView->Settings().iNetMode)
			PgLinkDisconnectL();             // (0.73) the link itself too, even when "Stay connected" holds it
		else if (iView->EngineRunning())
			iEikonEnv->InfoMsg(_L("Disconnecting\x85"));
		break;
	case EPwCmdUpdate:
		UpdateL();
		break;
	case EPwCmdRestart:
		RestartEngineL();
		break;
	case EPwCmdSendLog:
		iView->SendLog();
		break;
	case EPwCmdKeepLog:
		// (0.77) off unless ticked; what is in PsiWeb.log already stays. The
		// engine looks at each line, so it starts or stops at once.
		st.iDisplay ^= KPwKeepLog;
		SaveSettings(st);
		if (iView->Shared())
			iView->Shared()->net.log_on = (st.iDisplay & KPwKeepLog) || PwDiagOn() ? 1 : 0;
		break;                               // (0.99: the tick says it; no infoprint)
	case EPwCmdAbout:
		AboutL();
		break;
	case EPwCmdHelp:
		HelpL();
		break;
	default:
		if (aCommand >= EPwCmdFav0 && aCommand < EPwCmdFav0 + KPwCmdSlots)
			OpenFromListL(*iFavs, aCommand - EPwCmdFav0);
		else if (aCommand >= EPwCmdHist0 && aCommand < EPwCmdHist0 + KPwCmdSlots)
			OpenFromListL(*iHist, aCommand - EPwCmdHist0);
		break;
		}
	}

// Tools > Restart browser engine, and after settings the engine reads when
// it starts: it takes a few seconds, so say so, bottom left
void CPwAppUi::RestartEngineL()
	{
	iView->RestartL(ETrue);              // (it shows "Restarting..." until the engine draws)
	}

// PsiMail starts PsiWeb with a URL as the command line's tail ...
TBool CPwAppUi::ProcessCommandParametersL(TApaCommand /*aCommand*/, TFileName& aDocumentName, const TDesC8& aTail)
	{
	if (aTail.Length() > 0)
		{
		TBuf<PW_URL_MAX> url;
		url.Copy(Clip(aTail, url.MaxLength()));
		url.Trim();
		if (url.Length() > 0 && iView)
			iView->OpenUrlL(url);
		}
	aDocumentName.Zero();          // PsiWeb has no document file
	return EFalse;
	}

// ... or, when PsiWeb is already running, sends it the URL as a message
void CPwAppUi::ProcessMessageL(TUid aUid, const TDesC8& aParams)
	{
	// (0.97) short of memory (psimem.h): PsiHub's words. Before, they went
	// to CEikAppUi, which read them as a file to open and did nothing: the
	// Free memory dialog's "Put PsiWeb to sleep" never put it to sleep.
	//   "sleepnow"  the dialog, the user's choice: asleep now, answered
	//   "sleep"     under 1.5 MB free: asleep if behind and nothing would be lost
	//   "trim"      under 3 MB free, or "Clear PsiWeb's stored pages"
	if (aUid.iUid == KPsiMemMsgUid)
		{
		if (!iView)
			return;
		if (aParams == KPsiMemSleepNow)
			{
			TBuf<64> why;
			TBool slept = iView->SleepNow(why);
			PsiMemAnswer(iCoeEnv->WsSession(), KUidPsiWeb.iUid, slept ? KPsiMemSlept : KPsiMemCannot, why);
			}
		else if (aParams == KPsiMemSleep && !iForeground)
			{
			TBuf<64> why;
			if (!iView->SleepNow(why))
				iView->TrimNow();
			}
		else if (aParams == KPsiMemSleep || aParams == KPsiMemTrim)
			iView->TrimNow();
		return;
		}
	if (aUid != KUidPsiWeb)
		{
		CEikAppUi::ProcessMessageL(aUid, aParams);
		return;
		}
	TBuf<PW_URL_MAX> url;
	url.Copy(Clip(aParams, url.MaxLength()));
	url.Trim();
	if (url.Length() > 0 && iView)
		iView->OpenUrlL(url);
	}

// ===========================================================================
// Set up PsiWeb (0.85; core/docs/setup-assistant.md)
// ===========================================================================

// Pictures' standard by the way online: on every page for the netBook's
// network card (or dial-up), else (a modem, the 5mx) only when asked.
// (1.03) By the connection only: PsiProxy on a computer over the network
// card sends small pictures quickly too (it was only when asked there)
TInt CPwSetupDialog::StandardPictures(const PgSuChoice& aChoice, TBool aTall)
	{
	return aTall && aChoice.conn == PG_SU_CONN_NET;
	}

void CPwSetupDialog::AppPreLayoutL()
	{
	// the settings PsiWeb has (a file there), else the route's standard
	TInt std = StandardPictures(Choice(), iTall);
	TInt pics = iHadFile ? iSettings.iImages : std;
	// (1.03) Pictures set otherwise than the route's standard was a choice:
	// another way online no longer changes it behind the person's back
	iPicturesChosen = iHadFile && (pics != 0) != (std != 0);
	SetChoiceListCurrentItem(EPwDlgSuPictures, pics ? 1 : 0);
	SetChoiceListCurrentItem(EPwDlgSuSearch, iSettings.iSearch == PW_SEARCH_FROGFIND ? 1 : 0);
	SetChoiceListCurrentItem(EPwDlgSuStyle, iHadFile && iSettings.iStyle >= 0 ? iSettings.iStyle : StandardStyle(Choice()));
	ShowPicturesNoteL();
	ShowStyleNoteL();
	}

// (0.86) Page style's standard: Full, on every route. (Measured in the ARM
// harness: Links parses a whole page so quickly that simplifying it first on
// the Psion costs as much as it saves - Lighter about 10 to 25% more, Reader
// about the same - so the styles are for reading, not for speed: Reader puts
// an article first. A helper lightens pages before they arrive anyway.)
TInt CPwSetupDialog::StandardStyle(const PgSuChoice& /*aChoice*/)
	{
	return PW_STYLE_FULL;
	}

void CPwSetupDialog::ShowPicturesNoteL()
	{
	SetLabelL(EPwDlgSuPicturesNote, ChoiceListCurrentItem(EPwDlgSuPictures) == 1
		? _L("On every page suits a network card - Ctrl+I turns them off for one page")
		: _L("Only when asked suits a modem - Ctrl+I shows one page's pictures"));
	}

// (1.03) Page style's note follows the fetcher: without PsiProxy a big
// site's page starts with a screen of its menus in Full and Lighter, and
// Text only leaves them out. Offered here, not chosen: the person picks it
void CPwSetupDialog::ShowStyleNoteL()
	{
	PgSuChoice c;
	ReadNow(c);
	pg_su_fix(&c);
	TInt style = ChoiceListCurrentItem(EPwDlgSuStyle);
	if (c.helper != PG_SU_HELPER_PSIPROXY && style != PW_STYLE_TEXT && style != PW_STYLE_READER)
		SetLabelL(EPwDlgSuStyleNote, _L("Without PsiProxy, Text only suits big sites - it skips their menus"));
	else
		SetLabelL(EPwDlgSuStyleNote, _L("Reader shows the article first - Shift+Ctrl+R changes the page's style"));
	}

void CPwSetupDialog::AppControlChangedL(TInt aControlId)
	{
	if (aControlId == EPwDlgSuPictures)
		{
		iPicturesChosen = ETrue;
		ShowPicturesNoteL();
		}
	else if (aControlId == EPwDlgSuStyle)
		{
		iStyleChosen = ETrue;
		ShowStyleNoteL();
		}
	}

void CPwSetupDialog::AppChoiceChangedL()
	{
	if (!iStyleChosen)
		SetChoiceListCurrentItem(EPwDlgSuStyle, StandardStyle(Choice()));
	ShowStyleNoteL();
	if (iPicturesChosen)
		return;
	TInt was = ChoiceListCurrentItem(EPwDlgSuPictures);
	TInt now = StandardPictures(Choice(), iTall) ? 1 : 0;
	SetChoiceListCurrentItem(EPwDlgSuPictures, now);
	ShowPicturesNoteL();
	// (1.03) the Pages page is out of sight: say what the new way online
	// did to Pictures (it was a quiet change)
	if (was != now)
		iEikonEnv->InfoMsg(now ? _L("Pictures on every page - it suits a network card (Pages)")
			: _L("Pictures only when asked - it suits a modem (Pages)"));
	}

// (1.03) the dialog's choice as it stands (core's ReadChoice is private)
void CPwSetupDialog::ReadNow(PgSuChoice& aChoice)
	{
	Mem::FillZ(&aChoice, sizeof(aChoice));
	aChoice.conn = ChoiceListCurrentItem(EPgSuDlgConn);
	aChoice.baud_index = ChoiceListCurrentItem(EPgSuDlgBaud);
	aChoice.helper = ChoiceListCurrentItem(EPgSuDlgHelper);
	TBuf<PG_SU_HOST_MAX> h;
	GetEdwinText(h, EPgSuDlgHost);
	h.Trim();
	TInt n = h.Length();
	for (TInt i = 0; i < n; i++)
		aChoice.host[i] = (char)h[i];
	aChoice.host[n] = 0;
	aChoice.port = NumberEditorValue(EPgSuDlgPort);
	}

static void PwToSuLink(const TPsiLink& aIn, PgSuLink& aOut)
	{
	aOut.net_mode = aIn.iNetMode;
	aOut.baud_index = aIn.iBaudIndex;
	aOut.use_proxy = aIn.iUseProxy;
	TInt n = aIn.iProxyHost.Length() < PG_SU_HOST_MAX ? aIn.iProxyHost.Length() : PG_SU_HOST_MAX;
	for (TInt i = 0; i < n; i++)
		aOut.host[i] = (char)aIn.iProxyHost[i];
	aOut.host[n] = 0;
	aOut.port = aIn.iProxyPort;
	}

static void SuToPwLink(const PgSuLink& aIn, TPsiLink& aOut)
	{
	aOut.iNetMode = aIn.net_mode;
	aOut.iBaudIndex = aIn.baud_index;
	aOut.iUseProxy = aIn.use_proxy;
	aOut.iProxyHost.Copy(TPtrC8((const TUint8*)aIn.host));
	aOut.iProxyPort = aIn.port;
	}

// (1.03) OK as core's assistant does it (the shared link first, then
// PsiWeb's own), except the question about plain pages on the network:
// asked only when PsiProxy on a computer is new - not the helper the
// settings already had, at the same address and port - so OK with nothing
// changed, or after another change, does not ask again. Its words are in
// the dialog, not its title bar.
TBool CPwSetupDialog::OkToExitL(TInt aButtonId)
	{
	if (aButtonId == EPgSuBidTest)
		{
		TestNowL();
		return EFalse;
		}
	if (aButtonId != EEikBidOk)
		return ETrue;
	ReadNow(iNow);
	pg_su_fix(&iNow);
	if (pg_su_check(&iNow) == 1)
		{
		iEikonEnv->InfoMsg(_L("No address entered"));
		TryChangeFocusToL(EPgSuDlgHost);
		return EFalse;
		}
	RFs& fs = iCoeEnv->FsSession();
	TPsiLink link;
	link.SetDefaults();
	TBool had = link.Load(fs);
	PgSuLink was, now;
	PwToSuLink(link, was);
	now = was;
	pg_su_to_link(&iNow, &now);
	TBool known = had && was.use_proxy && pg_su_same(was.host, now.host) && was.port == now.port;
	if (pg_su_needs_privacy_query(&iNow) && !known)
		{
		TBuf<120> why;
		if (iNow.conn == PG_SU_CONN_ATOM)
			why.Copy(_L("Between the Atom modem and PsiProxy - fine at home, not on public WiFi"));
		else
			{
			TBuf<PG_SU_HOST_MAX> h;
			h.Copy(TPtrC8((const TUint8*)now.host));
			TPtrC hc = Clip(h, 24);
			why.Format(iTall ? _L("Between the netBook and PsiProxy at %S - fine at home, not on public WiFi")
				: _L("Between the Series 5mx and PsiProxy at %S - fine at home, not on public WiFi"), &hc);
			}
		if (!CPwQueryDialog::AskL(_L("PsiProxy on a computer"), _L("Pages and passwords cross your network unencrypted"),
			why, _L("Use PsiProxy on this network?")))
			return EFalse;
		}
	SuToPwLink(now, link);
	link.Save(fs);
	AppSaveL();
	return ETrue;
	}

void WebTestThen(PgLinkTest* aTest, void* aCtx);   // (0.89) below

// (1.03) Test: the way online as shown; and, when the dialog shows the
// route the browser engine uses now, a page through it as well, so the
// result names the web helper when it works too, not only when it fails
static void SetupTestNote(PgLinkTest* aTest, void* aCtx)
	{
	if (pg_lt_link_ok(aTest))
		pg_lt_add_line(aTest, (const char*)aCtx);
	}

void CPwSetupDialog::TestNowL()
	{
	PgSuChoice c;
	ReadNow(c);
	pg_su_fix(&c);
	TPsiLink link;
	link.SetDefaults();
	link.Load(iCoeEnv->FsSession());
	PgSuLink l;
	PwToSuLink(link, l);
	pg_su_to_link(&c, &l);
	CPwAppUi* ui = (CPwAppUi*)iEikonEnv->EikAppUi();
	CPwView* v = ui->View();
	const TPwSettings& e = v->StartedWith();
	TBuf<PG_SU_HOST_MAX> h;
	h.Copy(TPtrC8((const TUint8*)l.host));
	TBool same = v->EngineRunning() && (e.iNetMode != 0) == (l.net_mode != 0) && e.iBaudIndex == l.baud_index
		&& (e.iUseProxy != 0) == (l.use_proxy != 0)
		&& (!l.use_proxy || (e.iProxyHost.CompareF(h) == 0 && e.iProxyPort == l.port));
	if (same)
		PgLinkTestThenL(l.baud_index, link.iRtsCts, l.net_mode, link.iPppStart, R_PW_TEST_DIALOG, EPwDlgTest1,
			WebTestThen, ui);
	else
		PgLinkTestThenL(l.baud_index, link.iRtsCts, l.net_mode, link.iPppStart, R_PW_TEST_DIALOG, EPwDlgTest1,
			SetupTestNote, (void*)(l.use_proxy ? "A page through the web helper is tried once OK is pressed"
				: "A page is tried once OK is pressed"));
	}

// (1.03) CPwQueryDialog: a question with its words in the dialog
TBool CPwQueryDialog::AskL(const TDesC& aTitle, const TDesC& aFact, const TDesC& aWhy, const TDesC& aQuestion)
	{
	CPwQueryDialog* d = new(ELeave) CPwQueryDialog(aFact, aWhy, aQuestion);
	d->PrepareLC(R_PW_QUERY_DIALOG);
	d->SetTitleL(aTitle);
	return d->RunLD() == EEikBidYes;
	}

void CPwQueryDialog::PreLayoutDynInitL()
	{
	SetLabelL(EPwDlgQueryFact, iFact);
	SetLabelL(EPwDlgQueryWhy, iWhy);
	SetLabelL(EPwDlgQueryQuestion, iQuestion);
	}

void CPwSetupDialog::AppSaveL()
	{
	iSettings.iImages = ChoiceListCurrentItem(EPwDlgSuPictures) == 1;
	iSettings.iSearch = ChoiceListCurrentItem(EPwDlgSuSearch) == 1 ? PW_SEARCH_FROGFIND : PW_SEARCH_DDG;
	TInt style = ChoiceListCurrentItem(EPwDlgSuStyle);
	iSettings.iStyle = iStyleChosen || style != StandardStyle(Choice()) ? style : -1;
	// (0.91) "Modem provides TLS", in the shared PsiLink.ini, as PsiMail's
	// assistant sets it: on for the Atom modem, off for another one (the
	// Atom does the TLS on 443, so PsiWeb must not wrap its own inside)
	RFs& fs = iEikonEnv->FsSession();
	TPsiLink link;
	link.SetDefaults();
	link.Load(fs);
	link.iModemTls = iNow.conn == PG_SU_CONN_ATOM ? 1 : 0;
	link.Save(fs);
	}

TInt CPwAppUi::SetupCallback(TAny* aSelf)
	{
	CPwAppUi* ui = (CPwAppUi*)aSelf;
	TRAPD(err, ui->SetupL());
	(void)err;
	return 0;
	}

// Tools > Set up PsiWeb..., and at the first start of a new user. OK saves
// the shared link (the assistant) and PsiWeb's own settings; Cancel changes
// nothing, but PsiWeb.ini is written all the same at a first start, so it
// asks only once. (1.03) The engine starts again only for what it takes at
// its start - the route (Links is given the web helper then) and pictures
// on every page - on the page that was showing; a new page style is laid
// out at once, and the rest is used as it is. (It restarted at every OK, and
// the page was lost.)
void CPwAppUi::SetupL()
	{
	TPwSettings& st = iView->Settings();
	TInt styleWas = StyleNow(st);
	iView->StartBusyCancel();            // (the engine's "Starting..." is not over the dialog)
	CPwSetupDialog* dlg = new(ELeave) CPwSetupDialog(st, R_PW_TEST_DIALOG, EPwDlgTest1);
	TBool ok = dlg->ExecuteLD(R_PW_SETUP_DIALOG);
	if (ok)
		UseSharedLink(iCoeEnv->FsSession(), st);
	SaveSettings(st);
	if (!ok)
		return;
	ShowPicturesState(st.iImages);
	UpdateStartPageL();
	if (iView->Asleep())
		return;                          // (it starts with them when it wakes)
	const TPwSettings& e = iView->StartedWith();
	if (!iView->EngineRunning() || LinkChanged(e, st) || (e.iImages != 0) != (st.iImages != 0))
		{
		RestartEngineL();
		return;
		}
	// (1.03) a new web helper (Direct, the Atom's, PsiProxy, another
	// address): taken by the running engine, so the page, Back and Forward
	// and the connection stay
	if (HelperChanged(e, st))
		iView->HelperChangedL();
	else
		iView->LinkToShared();               // ("Modem provides TLS" and the rest, read at each connection)
	PwShared* sh = iView->Shared();
	if (sh && StyleNow(st) != styleWas)
		{
		sh->style = StyleNow(st);
		iView->Command(PW_CMD_RESTYLE, KNullDesC);
		}
	}

// (0.85) What a "Page not opened" page's links ask for
void WebTestThen(PgLinkTest* aTest, void* aCtx);   // (0.89) below

void CPwAppUi::EngineCommandL(TInt aCmd)
	{
	if (aCmd == PW_APP_SETUP)
		{
		SetupL();
		return;
		}
	if (aCmd == PW_APP_HELP)              // (0.87) the start page's What works
		{
		CPwHelpDialog* dlg = new(ELeave) CPwHelpDialog(2);
		dlg->ExecuteLD(R_PW_HELP_DIALOG);   // (0.99) one resource; its text sized by the screen
		return;
		}
	if (aCmd == PW_APP_FAVS)              // (0.87) the start page's Add...
		{
		AddNewFavouriteL();                  // (0.98) what it says, not Organise
		return;
		}
	TPsiLink link;
	link.SetDefaults();
	link.Load(iCoeEnv->FsSession());
	// (0.89) the link's test, then a small page through the route in use
	PgLinkTestThenL(link.iBaudIndex, link.iRtsCts, link.iNetMode, link.iPppStart, R_PW_TEST_DIALOG, EPwDlgTest1,
		WebTestThen, this);
	}

// ----- (0.89) Test the connection: the web part ---------------------------------
// After the link's own test (core's pglinktest: the modem answers, the
// network is up, a name is looked up), the engine fetches a small file
// through the route PsiWeb uses - its own TLS and certificate check direct,
// or the web helper - exactly as a page would come, and the result window
// says in a line or two whether it came. The file is the one PsiWeb's
// updates are checked with, on GitHub.
_LIT(KWebTestUrl, "https://raw.githubusercontent.com/danieledge/psiterm/main/dist/PsiWeb-version.txt");
const TInt KWebTestTickUs = 250000;
const TInt KWebTestMaxTicks = 4 * 120;     // two minutes: past a dial-up and a slow handshake on a 5mx

// Takes every key while the page is fetched; Esc stops it
class CPwTestKeys : public CCoeControl
	{
public:
	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
		{
		if (aType == EEventKey && aKeyEvent.iCode == EKeyEscape)
			iStop = ETrue;
		return EKeyWasConsumed;
		}
	TBool iStop;
	};

struct TPwTestWait
	{
	PwShared* iShared;
	CPwTestKeys* iKeys;
	TInt iTicks;
	TBool iEnded;                      // the loop was ended here (not by someone else's Stop)
	};

static TInt WebTestTick(TAny* aPtr)
	{
	TPwTestWait* w = (TPwTestWait*)aPtr;
	w->iTicks++;
	if (w->iTicks % 80 == 0)
		UserHal::ResetAutoSwitchOffTimer();
	if (w->iShared->test_state != PW_TEST_RUNNING || w->iKeys->iStop || w->iTicks >= KWebTestMaxTicks)
		{
		w->iEnded = ETrue;
		CActiveScheduler::Stop();
		return 0;
		}
	return 1;
	}

void WebTestThen(PgLinkTest* aTest, void* aCtx)
	{
	TRAPD(err, ((CPwAppUi*)aCtx)->WebTestL(aTest));
	if (err != KErrNone)
		pg_lt_add_line(aTest, "PsiWeb could not try a page - not enough memory");
	}

struct TPwTestStacked
	{
	CCoeAppUi* iAppUi;
	CCoeControl* iControl;
	CPwView* iView;
	};

static void WebTestUnstack(TAny* aPtr)
	{
	TPwTestStacked* t = (TPwTestStacked*)aPtr;
	t->iAppUi->RemoveFromStack(t->iControl);
	t->iView->iTesting = EFalse;
	}

void CPwAppUi::WebTestL(PgLinkTest* aTest)
	{
	TInt linkOk = pg_lt_link_ok(aTest);
	if (!linkOk)
		return;                          // (the link's own lines say what is wrong)
	PwShared* s = iView->Shared();
	if (!s || !iView->EngineRunning())
		{
		pg_lt_add_line(aTest, "PsiWeb did not try a page - the browser engine is not running");
		return;
		}
	TBuf<32> route;
	RouteBusyWords(iView->Settings(), route);
	TBuf<80> busy;
	busy.Format(_L("Opening the test page%S\x85"), &route);
	// (1.03) the outcome names the route the engine uses, whether the page
	// opened or not: "through PsiProxy at 192.0.2.7:8081", "through the
	// Atom modem helper", "directly"
	TBuf<80> how;
	{
	const TPwSettings& e = iView->StartedWith();
	TBuf8<64> h8;
	h8.Copy(Clip(e.iProxyHost, 60));
	h8.ZeroTerminate();
	if (!e.iUseProxy || e.iProxyHost.Length() == 0)
		how.Copy(_L(" directly"));
	else if (pg_su_is_atom_host((const char*)h8.Ptr()))
		how.Copy(_L(" through the Atom modem helper"));
	else
		{
		how.Copy(_L(" through PsiProxy at "));
		how.Append(Clip(e.iProxyHost, 40));
		how.AppendFormat(_L(":%d"), e.iProxyPort > 0 ? e.iProxyPort : 8080);
		}
	}
	CPwTestKeys* keys = new(ELeave) CPwTestKeys;
	CleanupStack::PushL(keys);
	CPeriodic* tick = CPeriodic::NewL(CActive::EPriorityStandard);
	CleanupStack::PushL(tick);
	TPwTestStacked stacked;
	stacked.iAppUi = this;
	stacked.iControl = keys;
	stacked.iView = iView;
	CleanupStack::PushL(TCleanupItem(WebTestUnstack, &stacked));
	AddToStackL(keys, ECoeStackPriorityAlert, ECoeStackFlagRefusesFocus);
	iView->iTesting = ETrue;
	iEikonEnv->BusyMsgL(busy, EHLeftVBottom, TTimeIntervalMicroSeconds32(0));
	s->test_state = PW_TEST_RUNNING;
	iView->Command(PW_CMD_TEST, KWebTestUrl);
	TPwTestWait wait;
	wait.iShared = s;
	wait.iKeys = keys;
	wait.iTicks = 0;
	wait.iEnded = EFalse;
	tick->Start(KWebTestTickUs, KWebTestTickUs, TCallBack(WebTestTick, &wait));
	CActiveScheduler::Start();           // until the page has come or not, Esc, or the limit
	tick->Cancel();
	TInt state = s->test_state;
	if (state == PW_TEST_RUNNING)
		{
		iView->Command(PW_CMD_STOP, KNullDesC);   // Esc, the limit, or the app closing
		s->test_state = PW_TEST_STOPPED;
		}
	iEikonEnv->BusyMsgCancel();
	CleanupStack::PopAndDestroy(3);      // unstack, tick, keys
	if (!wait.iEnded)
		{
		CActiveScheduler::Stop();        // someone else's stop (the app closing): passed on
		return;
		}
	TBuf<PG_LT_LINE> line;
	if (linkOk == 2 && (state == PW_TEST_OK || (state == PW_TEST_FAILED
		&& s->test_kind != PW_PROB_PORT_BUSY && s->test_kind != PW_PROB_NO_LINK && s->test_kind != PW_PROB_OTHER)))
		{
		// the serial port was held by PsiWeb's own connection, which reached
		// a server just now: the link's "in use" lines were about us
		aTest->nlines = 0;
		pg_lt_add_line(aTest, "PsiWeb's own connection has the serial port and the modem");
		}
	if (state == PW_TEST_OK)
		{
		TInt ms = s->test_ms;
		if (ms < 10000)
			line.Format(_L("The test page opened%S in %d.%d s"), &how, ms / 1000, (ms % 1000) / 100);
		else
			line.Format(_L("The test page opened%S in %d s"), &how, (ms + 500) / 1000);
		}
	else if (state == PW_TEST_FAILED)
		line.Format(_L("The test page did not open%S"), &how);
	else
		line = _L("Stopped before the test page opened");
	TBuf8<PG_LT_LINE> l8;
	l8.Copy(Clip(line, PG_LT_LINE - 1));
	pg_lt_add_line(aTest, (const char*)l8.PtrZ());
	if (state == PW_TEST_FAILED)
		pg_lt_add_line(aTest, s->test_why);
	}

TInt StyleNow(const TPwSettings& aS)
	{
	if (aS.iStyle >= PW_STYLE_FULL && aS.iStyle <= PW_STYLE_TEXT)
		return aS.iStyle;
	return PW_STYLE_FULL;
	}

// (0.86) View > Page style and Shift+Ctrl+R: kept as the choice from now
// on, and the page showing laid out again at once (no restart)
void CPwAppUi::SetStyleL(TInt aStyle)
	{
	TPwSettings& st = iView->Settings();
	st.iStyle = aStyle;
	SaveSettings(st);
	PwShared* s = iView->Shared();
	if (s)
		s->style = aStyle;
	if (iView->EngineRunning())
		iView->Command(PW_CMD_RESTYLE, KNullDesC);
	static const TText* const KNames[] = { _S("Full"), _S("Lighter"), _S("Reader"), _S("Text only") };
	iEikonEnv->InfoMsg(TPtrC(KNames[aStyle & 3]));
	}

// (1.03) Fetched directly or by the Atom modem's helper, a big site's page
// comes whole: in Full and Lighter it starts with a screen or more of the
// site's menus, which PsiProxy would have taken out. The first time such a
// page has loaded, PsiWeb offers Text only, once (the answer is kept, and
// View > Page style changes it at any time). Shown from an idle callback,
// not from the tick that saw the page.
static TBool WithoutPsiProxy(const TPwSettings& aS)
	{
	if (!aS.iUseProxy || aS.iProxyHost.Length() == 0)
		return ETrue;
	TBuf8<64> h;
	h.Copy(Clip(aS.iProxyHost, 60));
	h.ZeroTerminate();
	return pg_su_is_atom_host((const char*)h.Ptr());
	}

const TInt KPwOfferBytes = 150 * 1024;   // a page this big (as it came) is a big site's

void CPwAppUi::StyleOfferL()
	{
	PwShared* s = iView->Shared();
	TPwSettings& st = iView->Settings();
	if (!s || !iView->EngineRunning() || (st.iFlags & KPwFlagStyleAsked) || (iOfferIdle && iOfferIdle->IsActive()))
		return;
	TInt style = StyleNow(st);
	if ((style != PW_STYLE_FULL && style != PW_STYLE_LIGHTER) || s->page_style != style
		|| s->page_bytes < KPwOfferBytes || !(WithoutPsiProxy(iView->StartedWith()) || s->direct_now))
		return;
	TBuf<8> u;
	FromUtf8(u, s->url);
	if (Clip(u, 5).CompareF(_L("http:")) != 0 && Clip(u, 6).CompareF(_L("https:")) != 0)
		return;
	if (!iOfferIdle)
		iOfferIdle = CIdle::NewL(CActive::EPriorityIdle);
	iOfferIdle->Start(TCallBack(OfferCallback, this));
	}

TInt CPwAppUi::OfferCallback(TAny* aSelf)
	{
	CPwAppUi* ui = (CPwAppUi*)aSelf;
	TPwSettings& st = ui->iView->Settings();
	if ((st.iFlags & KPwFlagStyleAsked) || !ui->iForeground || ui->IsDisplayingMenuOrDialog() || ui->iEditing)
		return 0;                            // (another page will offer it)
	st.iFlags |= KPwFlagStyleAsked;          // asked once, whatever the answer
	ui->SaveSettings(st);
	TBool yes = EFalse;
	TRAPD(err, yes = CPwQueryDialog::AskL(_L("Page style"),
		_L("Without PsiProxy, big sites start with a screen of menus"),
		_L("Text only leaves menus and pictures out - View > Page style changes it"),
		_L("Show pages as Text only?")));
	if (err == KErrNone && yes)
		{
		TRAP(err, ui->SetStyleL(PW_STYLE_TEXT));
		}
	(void)err;
	return 0;
	}

// (0.86) The route light; (0.99) a picture only, as toolband buttons are:
// the padlock shut when the page came by https and was checked on the way
// (by the netBook or 5mx, the modem or the helper), open for any other.
// How the page came and its style are in Page information, a tap away.
void CPwAppUi::RouteLightL()
	{
	CEikCommandButton* b = iToolBand ? (CEikCommandButton*)iToolBand->ControlById(EPwCmdRouteLight) : NULL;
	PwShared* s = iView ? iView->Shared() : NULL;
	if (!b || !s || !iToolBand->IsVisible())
		return;
	TBool url = iView->EngineRunning() && Clip(iShownUrl, 6).Compare(_L("https:")) == 0;
	TInt lock = url && s->page_secure != PW_SECURE_NO ? 1 : 0;
	if (lock == iLockShown)
		return;
	iLockShown = lock;
	BandPictureL(EPwCmdRouteLight, lock ? EMbmBandLock : EMbmBandNoLock);
	}

// (0.88) Route 4, a full browser elsewhere: the page showing, opened by
// PsiTerm on one of its saved servers with a program that draws it there
// (Carbonyl: Chrome in a terminal; w3m). PsiTerm is started, or handed the
// request if it runs already, with "ssh:<server>\t<command>"
// (CPsiTermAppUi::ProcessCommandParametersL). The address goes in single
// quotes, any quote in it made safe, so the server's shell takes it as one
// word and runs nothing in it.
const TUid KUidPsiTerm = { 0x01000A77 };

void CPwFullDialog::PreLayoutDynInitL()
	{
	SetEdwinTextL(EPwDlgFullServer, &iSettings.iFullServer);
	SetEdwinTextL(EPwDlgFullCommand, &iSettings.iFullCommand);
	}

TBool CPwFullDialog::OkToExitL(TInt /*aButtonId*/)
	{
	TBuf<24> server;
	GetEdwinText(server, EPwDlgFullServer);
	server.Trim();
	TBuf<40> cmd;
	GetEdwinText(cmd, EPwDlgFullCommand);
	cmd.Trim();
	if (server.Length() == 0)
		{
		iEikonEnv->InfoMsg(_L("No server entered"));
		TryChangeFocusToL(EPwDlgFullServer);
		return EFalse;
		}
	if (cmd.Length() == 0)
		{
		iEikonEnv->InfoMsg(_L("No program entered"));
		TryChangeFocusToL(EPwDlgFullCommand);
		return EFalse;
		}
	for (TInt i = 0; i < server.Length(); i++)
		if (server[i] < ' ' || server[i] == '\t')
			{
			iEikonEnv->InfoMsg(_L("Not a server's name"));
			TryChangeFocusToL(EPwDlgFullServer);
			return EFalse;
			}
	iSettings.iFullServer = server;
	iSettings.iFullCommand = cmd;
	return ETrue;
	(void)iUrl;
	}

void CPwAppUi::FullBrowserL()
	{
	// the page showing, or the one that would not open here (a page that
	// needs more than PsiWeb has is what the full browser is for)
	PwShared* s = iView->Shared();
	TBuf<PW_URL_MAX> url;
	if (iView->EngineRunning())
		FromUtf8(url, s->url);
	if (url.Compare(_L("about:problem")) == 0)
		FromUtf8(url, s->problem_url);
	url.Trim();
	if (url.Length() == 0 || Clip(url, 6).Compare(_L("about:")) == 0)
		{
		iEikonEnv->InfoMsg(_L("Not available - no page open"));
		return;
		}
	TPwSettings& st = iView->Settings();
	CPwFullDialog* dlg = new(ELeave) CPwFullDialog(st, url);
	if (!dlg->ExecuteLD(R_PW_FULL_DIALOG))
		return;
	SaveSettings(st);
	// the command: program 'address' (PsiTerm runs a saved server's command
	// of at most 100 characters)
	TBuf8<200> line;
	line.Copy(_L8("ssh:"));
	TBuf8<24> server;
	server.Copy(st.iFullServer);
	line.Append(server);
	line.Append('\t');
	TBuf8<40> prog;
	prog.Copy(st.iFullCommand);
	prog.ZeroTerminate();
	TBuf8<PW_URL_MAX> u8;
	u8.Copy(Clip(url, u8.MaxLength()));
	char cmdc[101];                          // (PsiTerm runs at most 100 characters)
	TInt cn = pw_shell_command((const char*)prog.Ptr(), u8.Ptr(), u8.Length(), cmdc, sizeof(cmdc));
	if (cn < 0)
		{
		iEikonEnv->InfoMsg(_L("Address too long for the full browser"));
		return;
		}
	TPtrC8 cmd((const TUint8*)cmdc, cn);
	line.Append(cmd);
	// PsiTerm running: hand it the request; else start it with it
	TApaTaskList tasks(iEikonEnv->WsSession());
	TApaTask task = tasks.FindApp(KUidPsiTerm);
	if (task.Exists())
		{
		task.SendMessage(KUidPsiTerm, line);
		task.BringToForeground();
		return;
		}
	RApaLsSession ls;
	User::LeaveIfError(ls.Connect());
	CleanupClosePushL(ls);
	TApaAppInfo info;
	TInt r = ls.GetAppInfo(info, KUidPsiTerm);
	CleanupStack::PopAndDestroy();          // ls
	if (r != KErrNone)
		{
		iEikonEnv->InfoWinL(_L("PsiTerm is not installed"), _L("The full browser runs in PsiTerm, on a server it has saved"));
		return;
		}
	CApaCommandLine* cl = CApaCommandLine::NewLC();
	cl->SetLibraryNameL(info.iFullName);
	cl->SetCommandL(EApaCommandRun);
	cl->SetTailEndL(line);
	EikDll::StartAppL(*cl);
	CleanupStack::PopAndDestroy();          // cl
	iEikonEnv->BusyMsgL(_L("Opening in PsiTerm\x85"), EHLeftVBottom, TTimeIntervalMicroSeconds32(0));
	iFullBusyAt = User::TickCount();
	}

// (0.85) Opens what was typed: an address as it is, words through the
// search page chosen in Preferences (pwaddr.h)
void CPwAppUi::GoL(const TDesC& aTyped)
	{
	TBuf8<PW_URL_MAX> in;
	in.Copy(Clip(aTyped, in.MaxLength()));
	char out[PW_URL_MAX];
	TInt n = pw_addr_make(in.Ptr(), in.Length(), iView->Settings().iSearch, out, sizeof(out));
	if (n < 0)
		{
		iEikonEnv->InfoMsg(_L("Too many words to search for"));
		return;
		}
	TBuf<PW_URL_MAX> url;
	url.Copy(TPtrC8((const TUint8*)out, n));
	iView->Command(PW_CMD_OPEN, url);
	}

// ===========================================================================
// Document / Application / entry points
// ===========================================================================

CPwDocument::CPwDocument(CEikApplication& aApp)
	: CEikDocument(aApp)
	{
	}

CEikAppUi* CPwDocument::CreateAppUiL()
	{
	return new(ELeave) CPwAppUi;
	}

TUid CPwApplication::AppDllUid() const
	{
	return KUidPsiWeb;
	}

CApaDocument* CPwApplication::CreateDocumentL()
	{
	return new(ELeave) CPwDocument(*this);
	}

EXPORT_C CApaApplication* NewApplication()
	{
	return new CPwApplication;
	}

GLDEF_C TInt E32Dll(TDllReason)
	{
	return KErrNone;
	}
