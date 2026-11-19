// PWAPP.H - PsiWeb.app: the Psion side of the NetSurf web browser
//
// PsiWeb.app owns the screen, keyboard, pen, menus and settings. The browser
// itself (NetSurf) runs in psiweb.exe, started by the app, which draws pages
// into a chunk of memory shared with the app (see ../psiweb.h).

#ifndef __PWAPP_H
#define __PWAPP_H

#include <coecntrl.h>
#include <coemain.h>
#include <eikappui.h>
#include <eikapp.h>
#include <eikdoc.h>
#include <eikenv.h>
#include <eikmenup.h>
#include <eikcmds.hrh>
#include <eikmenu.hrh>
#include <f32file.h>
#include <eikdialg.h>
#include <eikdialg.hrh>
#include <fbs.h>
#include <eiksbfrm.h>
#include <eiksbobs.h>
#include <eiklbo.h>
#include <eikedwin.h>
#include <frmtview.h>
#include <eikprogi.h>
#include <eiktbar.h>

extern "C" {
#include <psiweb.h>
}
#include "psibell.h"           // (0.81) doorbells with the engine: power

#include "psidisp.h"
#include <psiweb.rsg>
#include "psiweb.hrh"
#include "pgsetup.h"           // (0.85) the setup assistant (core/ssh)
#include "pglinktest.h"        // (0.89) PgLinkTest: Test the connection with its web part

const TUid KUidPsiWeb = { 0x01000A7A };

// Left() that doesn't mind a short text: EPOC's Left(n) panics (USER 22)
// when n is more than the length
inline TPtrC Clip(const TDesC& aText, TInt aMax)
	{
	return aText.Left(aMax < 0 ? 0 : aMax < aText.Length() ? aMax : aText.Length());
	}


// text from the engine (UTF-8) -> the Psion's 8-bit text (psiweb.cpp)
void FromUtf8(TDes& aDst, const char* aSrc);

struct TPwSettings;
// the route in the user's words: About, Page information (psiweb.cpp)
void RouteWords(const TPwSettings& aSettings, TDes& aOut);
// (0.86) the page style in use: the one chosen, or the standard, Full
TInt StyleNow(const TPwSettings& aSettings);

// Favourites and History (pwfavs.cpp): one entry is a title and an address.
// They are kept in their own files (Favourites.dat, History.dat) beside
// PsiWeb.ini, not in TPwSettings, whose size must not change.
const TInt KPwTitleMax = 80;
const TInt KPwMaxFavs = 40;
const TInt KPwMaxHist = 50;
const TInt KPwMenuFavs = 5;            // favourites shown on the menu itself
const TInt KPwMenuHist = 5;            // pages shown in the History cascade
const TInt KPwCmdSlots = 16;           // EPwCmdFav0.. and EPwCmdHist0..
const TInt KPwRecent = 12;             // pages in the Open address dialog's Recent line

struct TPwEntry
	{
	TBuf<KPwTitleMax> iTitle;
	TBuf<PW_URL_MAX> iUrl;
	};

// A list of entries kept in a file: "PW" version count(1) then, for each,
// title and address, each with a 2-byte length. On the heap (about 600 bytes
// an entry). A missing, old or damaged file is an empty list.
class CPwList : public CBase
	{
public:
	static CPwList* NewL(RFs& aFs, const TDesC& aFile, TInt aMax);
	~CPwList();
	TInt Count() const { return iCount; }
	const TPwEntry& At(TInt aIndex) const { return iItems[aIndex]; }
	TInt Find(const TDesC& aUrl) const;          // index, or KErrNotFound
	void AddFront(const TDesC& aTitle, const TDesC& aUrl);   // History: newest first, no repeats
	TInt AddEnd(const TDesC& aTitle, const TDesC& aUrl);     // Favourites: KErrAlreadyExists, KErrOverflow
	void Set(TInt aIndex, const TDesC& aTitle, const TDesC& aUrl);
	void SetTitleOnly(TInt aIndex, const TDesC& aTitle);
	void Delete(TInt aIndex);
	void Clear();
	void Save();
private:
	CPwList(RFs& aFs, TInt aMax) : iFs(aFs), iMax(aMax) {}
	void ConstructL(const TDesC& aFile);
	void Load();
	RFs& iFs;
	TInt iMax;
	TInt iCount;
	TPwEntry* iItems;
	TFileName iFile;
	};

struct TPwSettings
	{
	TInt iBaudIndex;       // 0=9600 .. 4=115200
	TInt iRtsCts;
	TInt iNetMode;         // 0 = modem "ATDT host:port", 1 = Psion TCP/IP
	TBuf<40> iPppStart;    // Psion TCP/IP: sent to the modem first (shared, see psilink.h)
	TInt iUseProxy;
	TBuf<60> iProxyHost;
	TInt iProxyPort;
	TBuf<200> iHome;
	TInt iImages;
	TInt iZoom;            // percent
	TInt iToolbar;         // View > Show toolbar
	TInt iToolband;        // (0.78) View > Show toolband: the address, Back, Forward...
	TInt iDisplay;         // KPwDisplay* bits (docs/display.md)
	TInt iSearch;          // (0.85) Search with: PW_SEARCH_* (pwaddr.h)
	TInt iStyle;           // (0.86) Page style: PW_STYLE_* (psiweb_cmds.h); -1 the route's standard
	TInt iFlags;           // (0.85) KPwFlag* bits
	TBuf<24> iFullServer;  // (0.88) File > Open in full browser: PsiTerm's saved server
	TBuf<40> iFullCommand; // ...and the program run there with the address (carbonyl, w3m)
	};

// TPwSettings::iFlags, one byte after the toolband's in PsiWeb.ini (0.85)
const TInt KPwFlagKeepCookies = 1;     // (0.87) Preferences > Cookies: kept when PsiWeb closes

// TPwSettings::iDisplay, one byte at the end of PsiWeb.ini (0.81); all 0 is the standard
const TInt KPwDisplayReading = 1;      // View > Reading mode
const TInt KPwDisplayScaledText = 2;   // Preferences > Text: scaled (Links' own fonts)
const TInt KPwKeepLog = 4;             // (0.77) Tools > Keep a log: PsiWeb.log is written only with
                                       // this bit (in the same byte; older files have it 0: off)

class CPwView;

class CPwWatcher : public CActive
	{
public:
	CPwWatcher(CPwView& aView);
	~CPwWatcher();
	void Watch(RProcess& aProcess);
private:
	void RunL();
	void DoCancel();
	CPwView& iView;
	RProcess* iProcess;
	};

class CPwUpdateProgress;
class CPwView : public CCoeControl, public MEikScrollBarObserver
	{
public:
	~CPwView();
	void ConstructL(const TRect& aRect, const TPwSettings& aSettings);
	TPwSettings& Settings() { return iSettings; }
	PwShared* Shared() { return iShared; }
	TBool EngineRunning() const { return iRunning; }
	void StartEngineL();
	void StopEngine(TBool aSleep = EFalse);
	// (0.92) memory: behind another program, the engine is trimmed after
	// half a minute and put to sleep (ended, to start again on the same
	// page) after ten, when nothing would be lost
	void SetForeground(TBool aFront);
	TBool Asleep() const { return iAsleep; }
	void WakeL();
	void StartBusy(const TDesC& aText);      // busy message until the engine draws
	void StartBusyCancel();
	void Command(TInt aCmd, const TDesC& aArg);
	void Key(TUint aCode, TUint aMods);
	void EngineEnded();
	void ShowMessage(const TDesC& aLine1, const TDesC& aLine2);
	void StartUpdateL();
	void OpenUrlL(const TDesC& aUrl);
	void RestartL(TBool aSamePage);
	void SetPageRectL(const TRect& aRect);
	const TDesC& UpdateLog() const { return iUpdLog; }
	void SendLog();                            // Tools > Send log to server (PW_CMD_PUTLOG)
	void LogTick();                            // its busy message and outcome
	TBool Updating() const { return iUpdState == PW_UPD_RUNNING; }
	void StopUpdate() { if (iShared) { iShared->net.quit = 1; RingEngine(); } }   // the updater gives up and says so
	void RingEngine();                       // (0.81) something for the engine in the chunk: wake it
	TBool PicturesShown() const;             // this page's pictures are shown (the Pictures button)
	TBool Colour() const { return iColour; } // a colour screen (the netBook): EPOC's 256 colours
	TInt MaxHeight() const { return iMaxH; } // the screen's height, at most PW_MAX_H
private:
	// the page's scroll bar: EIKON's, beside the page (the engine says where
	// the page is: psiweb.h page_*)
	void LayoutL();                          // the bar and the page's room in Rect()
	void UpdateScrollBarL();                 // the thumb where the engine says the page is
	void ShowScrollBar(TBool aShow);         // (0.89) only while the page does not fit
	void ScrollTo(TInt aY);
	void HandleScrollEventL(CEikScrollBar* aScrollBar, TEikScrollEvent aEventType);
	TInt CountComponentControls() const;
	CCoeControl* ComponentControl(TInt aIndex) const;
	static TInt StartCallback(TAny* aSelf);
	void Draw(const TRect& aRect) const;
	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);
	void HandlePointerEventL(const TPointerEvent& aEvent);
	void PushEvent(TInt aType, TInt aCode, TInt aX, TInt aY);
	void AddEntropy(TUint aValue);
	static TInt TickCallback(TAny* aSelf);
	void Tick();
	// (0.81) power: 16 ticks a second while pages load or frames come, else
	// one every 2 s with the doorbell armed (the engine rings it with news)
	TBool TickWanted() const;
	void TickFast();
	void TickQuiet();
	static TInt BellCallback(TAny* aSelf);
	static TInt AskCallback(TAny* aSelf);    // the engine's questions (Links phase 5)
	void AskAuthL();                         // a user name and password
	void AskSaveL();                         // where to save a file PsiWeb cannot show
	void UpdateTickL();
	void UpdateLine(const TDesC& aLine);        // adds to the progress window's text
	void UpdateDialogL();                       // the progress window, then what the update came to
	void StartInstallerL();
private:
	TPwSettings iSettings;
	TBool iColour;                // the screen has colours: the page is EColor256
	TInt iMaxH;                   // the screen's height (240 on the 5mx, 480 on the netBook)
	TInt iChunkSize;              // PW_SHARED_SIZE for this screen
	CFbsBitmap* iBitmap;
	RChunk iChunk;
	TBool iChunkOpen;
	PwShared* iShared;
	RProcess iProcess;
	TBool iRunning;
	CPwWatcher* iWatcher;
	CPeriodic* iTimer;
	CPsiBellWaiter* iBellWaiter;  // (0.81) the engine's news
	TPsiBellRinger iEngRinger;    // ...and its doorbell
	TBool iTickQuiet;             // the tick is on its slow pace
	TInt iCalmTicks;              // ticks in a row with nothing going on
	TUint iFrameAt;               // tick count of the last new frame
	CIdle* iStarter;
	TBuf<PW_URL_MAX> iStartUrl;
	TUint iLastFrame;
	TUint iLinkSeq;               // last link message shown (PsiShared link_seq)
	TBool iLinkBusy;              // a link note is up as a busy message
	TBool iStartBusy;             // "Starting the browser engine..." is up, until its first page loads
	TUint iStartBusyAt;           // (tick count when it went up)
	TInt iEntropyPos;
	TBuf<80> iMsg1;
	TBuf<120> iMsg2;
	TBool iShowMsg;
	TInt iLogState;            // last PW_LOG_* seen (Tools > Send log to server)
	TInt iUpdState;            // last PW_UPD_* seen
	TBuf<128> iUpdMsg;
	TBuf<1200> iUpdLog;        // the progress window's text (paragraphs)
	CPwUpdateProgress* iUpdDlg;   // the window, while it is up
	TFileName iUpdateFile;
	CIdle* iAsker;             // shows the engine's questions outside Tick
	TBool iAsking;             // one of its dialogs is up
	CEikScrollBarFrame* iSBFrame;   // the page's scroll bar
	TRect iPageArea;           // the room the engine draws in (Rect() less the bar)
	TInt iSbH, iSbY, iSbVh;    // what the bar shows: the page's height, the view's top and height
	TInt iEngH, iEngY, iEngVh; // ...and what the engine last said (it may not have caught up)
	TBool iSbDragging;         // the thumb is being dragged: the engine's place waits
	TBool iSbPen;              // the pen went down on the bar: its events are the bar's
	TInt iPics;                // page_pics last seen (-1: not yet)
	TBool iLastBusy;           // busy at the last tick: a fall is a page loaded (History)
	void MemoryTick();
	void Sleep();
	TUint iBehindAt;           // tick count when PsiWeb went behind another program (0: in front)
	TBool iTrimmed;            // PW_CMD_TRIM sent since then
	TBool iAsleep;             // the engine was ended to free its memory: WakeL starts it again
	TBool iWaking;             // the engine starting now is to take up Sleep's history and cookies
	TInt iWakeY;               // the page's place when it slept, to scroll back to (-1: none)
	// (0.89) the busy message says the route: "Opening bbc.co.uk through
	// PsiProxy - 64 KB...", in place of the link's routine notes
	void LoadBusyL();
	TBool iLoadBusy;           // that message is up
	TBuf<100> iLoadShown;      // ...what it says
	TUint iLoadAt;             // ...and when it last changed (tick count)
	TBool iSbShown;            // (0.89) the scroll bar shows: only when the page does not fit
public:
	TBool iTesting;            // (0.89) Test the connection's web part runs: the link's routine notes are not shown
	};

// A page or the proxy needs a user name and password (HTTP authentication,
// Links phase 5): the engine asks through the shared chunk (psiweb.h auth_*)
class CPwAuthDialog : public CEikDialog
	{
public:
	CPwAuthDialog(const TDesC& aWho, const TDesC& aRealm, TDes& aUser, TDes& aPass)
		: iWho(aWho), iRealm(aRealm), iUser(aUser), iPass(aPass) {}
private:
	void PreLayoutDynInitL();
	TBool OkToExitL(TInt aButtonId);
	const TDesC& iWho;
	const TDesC& iRealm;
	TDes& iUser;
	TDes& iPass;
	};

// Page information: up to five lines of text (the title is the resource's)
class CPwInfoDialog : public CEikDialog
	{
public:
	// aLines: an array of TPtrC (not TBufs: a TDesC* would step by the
	// wrong size - the old Page info said "Not enough memory" for that)
	CPwInfoDialog(const TPtrC* aLines, TInt aCount)
		: iLines(aLines), iCount(aCount) {}
private:
	void PreLayoutDynInitL();
	const TPtrC* iLines;
	TInt iCount;
	};

// Help on PsiWeb: a topic list over a read-only text (pwhelp.cpp)
class CPwHelpDialog : public CEikDialog
	{
public:
	CPwHelpDialog(TInt aTopic) : iTopic(aTopic) {}
private:
	void PreLayoutDynInitL();
	void PostLayoutDynInitL();
	void HandleControlStateChangeL(TInt aControlId);
	TBool OkToExitL(TInt aButtonId);
	void ShowTopicL(TInt aTopic);
	TInt iTopic;
	};

class CPwAboutDialog : public CEikDialog
	{
public:
	CPwAboutDialog(const TDesC& aStatus) : iStatus(aStatus) {}
private:
	void PreLayoutDynInitL();
	const TDesC& iStatus;
	};

// Update PsiWeb: where from (as PsiMail's)
class CPwUpdateDialog : public CEikDialog
	{
public:
	CPwUpdateDialog(TInt& aSource, TDes& aHost, TInt& aPort)
		: iSource(aSource), iHost(aHost), iPort(aPort) {}
private:
	void PreLayoutDynInitL();
	TBool OkToExitL(TInt aButtonId);
	TInt& iSource;
	TDes& iHost;
	TInt& iPort;
	};

// Update PsiWeb's progress window, as PsiTerm's: the engine's words line by
// line (a download count replaces the last line), Stop while it works, Close
// when it has ended
class CPwUpdateProgress : public CEikDialog
	{
public:
	CPwUpdateProgress(CPwView& aView) : iView(aView), iFinished(EFalse) {}
	void ShowL(const TDesC& aText);      // the log so far
	void FinishL(const TDesC& aText);    // the job has ended: Stop -> Close
private:
	void SetSizeAndPositionL(const TSize& aSize);
	void PreLayoutDynInitL();
	TBool OkToExitL(TInt aButtonId);
	void SetButtonTextL(const TDesC& aText);
	CPwView& iView;
	TBool iFinished;
	};

class CPwOpenDialog : public CEikDialog
	{
public:
	CPwOpenDialog(TDes& aUrl, const CPwList& aHistory) : iUrl(aUrl), iHistory(aHistory) {}
private:
	void PreLayoutDynInitL();
	void HandleControlStateChangeL(TInt aControlId);
	TBool OkToExitL(TInt aButtonId);
	TDes& iUrl;
	const CPwList& iHistory;
	};

// Add to favourites / Edit: a name and an address (pwfavs.cpp)
class CPwFavDialog : public CEikDialog
	{
public:
	CPwFavDialog(TDes& aName, TDes& aUrl) : iName(aName), iUrl(aUrl) {}
private:
	void PreLayoutDynInitL();
	TBool OkToExitL(TInt aButtonId);
	TDes& iName;
	TDes& iUrl;
	};

// A list of favourites or of history (pwfavs.cpp). Mode 0: pick one to open
// (Cancel, Open); mode 1: Organise favourites (Edit, Delete, Close - the
// changes are made at once). aPick is the entry chosen.
class CPwListDialog : public CEikDialog, public MEikListBoxObserver
	{
public:
	CPwListDialog(CPwList& aList, TBool aOrganise, TInt& aPick)
		: iList(aList), iOrganise(aOrganise), iPick(aPick), iReady(EFalse) {}
private:
	void PreLayoutDynInitL();
	void PostLayoutDynInitL();
	TBool OkToExitL(TInt aButtonId);
	void HandleListBoxEventL(CEikListBox* aListBox, TListBoxEvent aEventType);
	void FillL();
	CPwList& iList;
	TBool iOrganise;
	TInt& iPick;
	TBool iReady;                  // laid out: the current row may be set
	};

class CPwConnDialog : public CEikDialog
	{
public:
	CPwConnDialog(TPwSettings& aSettings) : iSettings(aSettings) {}
private:
	void PreLayoutDynInitL();
	void HandleControlStateChangeL(TInt aControlId);
	TBool OkToExitL(TInt aButtonId);
	void TestL();             // the Test button (0.74)
	void DimProxyLines();     // (0.72) host and port wait for "Use proxy"
	void DimKeepUpLine();     // (0.73) "After use": dimmed on the modem route
	TPwSettings& iSettings;
	};

// Set up PsiWeb (0.85): the suite's two pages (pgsetup.h) and PsiWeb's
// own, Pages: pictures (their standard follows the route until changed)
// and where words typed for an address are searched for
class CPwSetupDialog : public CPgSetupDialog
	{
public:
	CPwSetupDialog(TPwSettings& aSettings, TInt aTestDialog, TInt aTestFirstLine)
		: CPgSetupDialog(aTestDialog, aTestFirstLine), iSettings(aSettings), iPicturesChosen(EFalse), iStyleChosen(EFalse) {}
	static TInt StandardPictures(const PgSuChoice& aChoice, TBool aTall);
	static TInt StandardStyle(const PgSuChoice& aChoice);
private:
	void AppPreLayoutL();
	void AppControlChangedL(TInt aControlId);
	void AppChoiceChangedL();
	void AppSaveL();
	void ShowPicturesNoteL();
	TPwSettings& iSettings;
	TBool iPicturesChosen;     // the person set Pictures: the route no longer changes it
	TBool iStyleChosen;        // (0.86) ...and Page style
	};

// (0.88) File > Open in full browser: which of PsiTerm's saved servers, and
// the program that shows the page there
class CPwFullDialog : public CEikDialog
	{
public:
	CPwFullDialog(TPwSettings& aSettings, const TDesC& aUrl) : iSettings(aSettings), iUrl(aUrl) {}
private:
	void PreLayoutDynInitL();
	TBool OkToExitL(TInt aButtonId);
	TPwSettings& iSettings;
	const TDesC& iUrl;
	};

class CPwPrefsDialog : public CEikDialog
	{
public:
	CPwPrefsDialog(TPwSettings& aSettings) : iSettings(aSettings) {}
private:
	void PreLayoutDynInitL();
	TBool OkToExitL(TInt aButtonId);
	TPwSettings& iSettings;
	};

// (0.87) how far the page has come, along the toolband address's lower
// edge while it loads (nothing when idle: no empty gap beside the address)
class CPwBar : public CCoeControl
	{
public:
	void ConstructL(const CCoeControl& aParent);
	void SetValue(TInt aPercent) { iValue = aPercent < 0 ? 0 : aPercent > 100 ? 100 : aPercent; }
private:
	void Draw(const TRect& aRect) const;
	TInt iValue;
	};

class CPwAppUi : public CEikAppUi, public MCoeControlObserver
	{
public:
	void ConstructL();
	~CPwAppUi();
	void ShowPicturesState(TBool aOn);  // the Pictures button pressed in or not
	// the toolband (0.78): its address and progress follow the engine
	void ChromeTick();
	TBool AddressKeyL(const TKeyEvent& aKeyEvent, TEventCode aType);   // ETrue: the address had it
	TBool Editing() const { return iEditing; }
	void SelectAllAddressL();           // (0.94) Ctrl+A while the address is typed in
	void EndAddressL(TBool aGo);        // the address shows the page again (and goes there)
	void PageLoaded();                  // a page has loaded: History (pwfavs.cpp)
	void PageTick();                    // ...and its title may arrive a moment later
	void EngineCommandL(TInt aCmd);     // (0.85) a "Page not opened" page's links (PW_APP_*)
	void SetStyleL(TInt aStyle);        // (0.86) View > Page style
	void UpdateStartPageL();            // (0.87) the start page (pwstart.cpp)
	void OpenTileL(TInt aIndex);        // (0.87) ...its tile 1 to 8
	void SeedFavouritesL();             // (0.87) a new PsiWeb's favourites: its first tiles
	void RouteLightL();                 // (0.86) the toolband's route light follows the page
private:
	// Favourites and History (pwfavs.cpp)
	TBool HavePage() const;
	void AddFavouriteL();
	void OrganiseL();
	void OpenFromListL(CPwList& aList, TInt aIndex);
	void PickAndOpenL(CPwList& aList);
	void ClearHistoryL();
	void FavsMenuL(CEikMenuPane* aMenuPane);
	void HistoryMenuL(CEikMenuPane* aMenuPane);
	CPwList* iFavs;
	CPwList* iHist;
	TUint iHistUntil;                   // tick count to watch for the page's title until (0: not)
	// Reading mode (ssh/psidisp.h): on while PsiWeb is in front with the
	// tick on; off in the background, at switch-on and on closing
	void HandleWsEventL(const TWsEvent& aEvent, CCoeControl* aDestination);
	void UpdateReading(TBool aEnterOnly = EFalse);
	TPsiReading iReading;
	TBool iForeground;
	void HandleCommandL(TInt aCommand);
	void HandleSwitchOnEventL(CCoeControl* aDestination);
	TBool ProcessCommandParametersL(TApaCommand aCommand, TFileName& aDocumentName, const TDesC8& aTail);
	void ProcessMessageL(TUid aUid, const TDesC8& aParams);
	void DynInitMenuPaneL(TInt aMenuId, CEikMenuPane* aMenuPane);
	void HandleAttemptDimmedSelectionL(TInt aCommandId);
	void LoadSettings(TPwSettings& aSettings);
	void SaveSettings(const TPwSettings& aSettings);
	void PageInfoL();
	void AboutL();
	void UpdateL();
	void HelpL();                      // Tools > Help on PsiWeb (pwhelp.cpp)
	void ButtonPictureL(CEikToolBar* aBar, TInt aId, TInt aIcon);
	void ToolbarPicturesL();
	void ShowToolBarL(TBool aShow);
	void ShowToolBandL(TBool aShow);
	void RestartEngineL();
	void SetupL();                      // (0.85) Set up PsiWeb...
	static TInt SetupCallback(TAny* aSelf);
	CIdle* iSetupIdle;                  // the first-time setup, once the window is up
	void GoL(const TDesC& aTyped);      // (0.85) an address, or words to search for
	void FullBrowserL();                // (0.88) File > Open in full browser (PsiTerm)
	TRect PageRect();                   // the bars laid out as they are shown: the page's room
	TBool TallScreen() const;           // 480 lines (the netBook): the toolband at first, not the toolbar
	void StartAddressL();               // a tap on the address, or Ctrl+L
	void ClipboardL(TInt aCommand);     // (0.94) Edit > Cut, Copy, Paste: the address, or a page's field
	void CopyAddressL();                // (0.94) Edit > Copy page address
	void PasteToPageL();                // (0.94) the clipboard's text typed into the page's field
	void CompleteAddressL();            // (0.87) the address offered as it is typed
	void HandleControlEventL(CCoeControl* aControl, TCoeEvent aEventType);
	void SetAddressL(const TDesC& aText);
	void ShowAddressCursorL(TBool aShow);   // the address's cursor and selection, as if it had the focus
	CPwView* iView;
	CEikEdwin* iAddress;                // the toolband's address and progress (NULL: no toolband)
	CPwBar* iBar;                       // (0.87) the progress bar along the address
	TBool iBandBusy;                    // (0.87) Reload shows Stop
	TBool iEditing;                     // the address is being typed in
	TBuf<PW_URL_MAX> iShownUrl;         // the page it shows
	TBool iAddressStale;                // ...to be shown again, whatever it was
	TBool iPenTaken;                    // the pen's down-to-up is the address's first tap
	TInt iShownProgress;                // what the bar shows (-1: hidden)
	TBuf<16> iRouteShown;               // (0.86) what the route light says
	TUint iFullBusyAt;                  // (0.88) "Opening in PsiTerm..." went up (0: not up)
	TInt iLockShown;                    // ...and whether its padlock is shut (-1: not yet)
	TInt iOtherZoom;                    // (0.89) the other machine's zoom in PsiWeb.ini (0: none), kept
	                                    // as it was: zoom is remembered per machine
public:
	void WebTestL(PgLinkTest* aTest);   // (0.89) Test the connection's web part
	};

class CPwDocument : public CEikDocument
	{
public:
	CPwDocument(CEikApplication& aApp);
private:
	CEikAppUi* CreateAppUiL();
	};

class CPwApplication : public CEikApplication
	{
private:
	CApaDocument* CreateDocumentL();
	TUid AppDllUid() const;
	};

#endif
