// pwhelp.cpp - Tools > Help on PsiWeb (Shift+Ctrl+H): the help topics, in
// an EIKON dialog with a topic list and a read-only text viewer - the same
// dialog as PsiMail's (mail/app/pmhelp.cpp), which says why the help lives
// in the program rather than in a .hlp file.
//
// The wording follows the EIKON style guide: the user's words, key names as
// the keyboard prints them, menu commands named as they appear (without
// their "..."), "pictures" not "images", "program" not "application".
#include "pwapp.h"
#include <eikchlst.h>
#include <eikrted.h>
#include <txtrich.h>
#include <eikenv.h>
#include <frmtlay.h>
#include <eikcolor.hrh>
#include <eikfctry.h>
#include <barsread.h>
#include <frmtview.h>

// (0.85) iTall and iShort: a paragraph for the netBook and Series 7 (a
// screen 400 or more high), or for the Series 5mx, added after the text
// (0 for none): one help, the right words on each machine
struct TPwHelpTopic { const char* iTitle; const char* iText; const char* iTall; const char* iShort; };

static const TPwHelpTopic KHelpTopics[] =
	{
	{ "Getting started",
	  "Type an address, or words to search for, and press Enter: Ctrl+L puts the cursor in the "
	  "address along the top (the toolband), and File > Open address (Ctrl+O) asks for one. Words "
	  "go to the search page chosen in Tools > Preferences (DuckDuckGo as standard).\n"
	  "\n"
	  "Tap a link to follow it. Go > Back (Ctrl+B) and Forward (Ctrl+F) move through the pages you "
	  "have seen, Esc stops a page that is loading, and Go > Start page goes back to PsiWeb's own "
	  "first page, which needs no connection.\n"
	  "\n"
	  "Use Tools > Set up PsiWeb to choose how to go online and how pages are fetched. It also "
	  "opens on first start. A page that cannot be opened says why, with Try "
	  "again, Test the connection and Set up PsiWeb on it. Test the connection checks the modem "
	  "or the network, then fetches a small file from GitHub by the way pages come, and says "
	  "whether it came. While a page opens, the message at the bottom left says which page, how "
	  "much has come, and whether it comes through the web helper.",
	  "On the netBook the toolbar is on the right, as in every program, and the toolband with the "
	  "address along the top. View > Show hides either (Ctrl+T, Shift+Ctrl+T), and the page takes "
	  "the room. Pages are in colour.",
	  "On the Series 5mx the toolbar is shown on the right; View > Show > Toolband (Shift+Ctrl+T) "
	  "adds the address along the top. Pages are in 16 greys." },

	{ "Ways to connect",
	  "How the machine goes online is set once for PsiWeb, PsiMail and PsiTerm (Tools > Connection "
	  "settings, or Set up PsiWeb): a network card or dial-up (the machine's own TCP/IP, set up in "
	  "the Control panel), or a WiFi modem on the serial port, such as the PsiConnect Atom.\n"
	  "\n"
	  "Pages then reach PsiWeb in one of three ways:\n"
	  "\n"
	  "Direct: PsiWeb fetches pages itself. Nothing extra is needed. Secure sites (https) are "
	  "checked by the machine itself and nobody in between sees the page. Fast on a network card; "
	  "slow for big sites over a modem. JPEG, PNG and GIF pictures; some newer kinds show as a box. "
	  "Works wherever the machine is online. Direct through the PsiConnect Atom with Modem provides "
	  "TLS on (Set up PsiWeb sets it for the Atom; PsiMail's Connection settings), the Atom makes the secure connection and checks the "
	  "site instead, which is much quicker.\n"
	  "\n"
	  "Atom modem helper: the PsiConnect Atom fetches pages and makes them lighter, about three "
	  "times quicker than direct over a modem. The modem checks secure sites, and it sees the page. "
	  "Small pictures are converted. Works wherever the modem has WiFi. In Connection settings it "
	  "is the web helper psiproxy, port 8080.\n"
	  "\n"
	  "PsiProxy on a computer: a computer at home, switched on and running PsiProxy "
	  "(github.com/danieledge/psiproxy), fetches pages and makes them much lighter: the quickest "
	  "way, and every kind of picture. The computer checks secure sites, but the page and any "
	  "password you type cross your home network unencrypted. Some pages drawn by scripts work if "
	  "that is turned on in PsiProxy. Does not work away from home. In Connection settings it is "
	  "the web helper at the computer's address, port 8081. Tools > Troubleshooting > Send log to helper "
	  "needs it.\n"
	  "\n"
	  "A full browser on another computer: Tools > Open in full browser asks PsiTerm to connect to "
	  "one of its saved servers and run a browser there with the page showing. Carbonyl "
	  "(github.com/fathyb/carbonyl) is Chrome drawn in a terminal: scripts, video stills, logins "
	  "and all, so pages PsiWeb cannot run open there. Install it on any Linux or Mac computer with "
	  "Node: npm install --global carbonyl (or, with Docker: docker run --rm -ti fathyb/carbonyl), "
	  "save that computer as a server in PsiTerm, and give its name and the program, carbonyl, in "
	  "the dialog. The computer fetches and draws the page; PsiWeb's machine shows it. On the Series 5mx's "
	  "screen w3m (apt install w3m, or brew install w3m) suits better, and is the program offered "
	  "there. PsiTerm and PsiWeb share the connection as they always do.\n"
	  "\n"
	  "Test (Ctrl+T, in Connection settings and Set up PsiWeb) tries the settings shown before OK. "
	  "File > Connect brings the connection up now; File > Disconnect (Ctrl+U) hangs up the modem "
	  "and frees the serial port for PsiTerm or PsiMail, or takes the network or dial-up "
	  "connection down. The serial port can be used by one program at a time; the Remote link "
	  "must be off (System screen, Ctrl+L).",
	  "On the netBook a PC Card network card is the best way: choose Network card or dial-up and "
	  "Direct. Pictures are shown on every page as standard there.",
	  "On the Series 5mx the Atom modem with its helper is the everyday way; direct suits light "
	  "sites, and PsiProxy on a computer is the quickest at home." },

	{ "What works and what does not",
	  "Works: pages written as HTML on the server, which is most news, reference and text sites; "
	  "tables and frames; JPEG, PNG and GIF pictures; plain forms, such as searches and simple "
	  "logins; cookies, until PsiWeb closes or, with Tools > Preferences > Cookies > Kept for next "
	  "time, from one day to the next (in PsiWeb's folder; choosing Until PsiWeb closes again "
	  "deletes them); user names and passwords that the site asks for in a "
	  "dialog; saving a file PsiWeb cannot show, such as a PDF, to a disk.\n"
	  "\n"
	  "Does not work: pages drawn by scripts (JavaScript), such as YouTube, X, the new Reddit, most "
	  "web programs and bank logins: they come out blank or nearly so. Style sheets, video and "
	  "sound. Sites that still use only old security (TLS 1.2), which say \"The site needs older "
	  "security than PsiWeb has\": they may open through PsiProxy on a computer.\n"
	  "\n"
	  "Light editions of big sites suit PsiWeb best: 68k.news, text.npr.org, lite.cnn.com, "
	  "en.m.wikipedia.org, old.reddit.com, html.duckduckgo.com: on the full site, a line at the "
	  "top of the page offers the lighter edition. FrogFind (frogfind.com) searches "
	  "and can read other pages as plain text; it is someone else's service, which sees the "
	  "addresses you read.",
	  0, 0 },

	{ "Page styles",
	  "View > Page style chooses how much of a page PsiWeb keeps before it lays the page out. "
	  "Shift+Ctrl+R goes round them for the page showing, and the choice stays for the pages after.\n"
	  "\n"
	  "Full: the page as it comes.\n"
	  "Lighter: scripts, style sheets, hidden parts and tracking pictures taken out, the site's "
	  "menus and its footer each put in one line of links, and pictures marked as decoration "
	  "left out: a news site's headlines start on the first screen.\n"
	  "Reader: the page's text, with its menus, sidebars, banners and lists of links taken out "
	  "too. Best for an article on a news site. A page of headlines (a news site's front page) "
	  "has no article: Reader shows it as Text only does, and says so.\n"
	  "Text only: no menus, no sidebars and no pictures; a picture that is a link shows its "
	  "description in its place.\n"
	  "\n"
	  "Until you choose a style, it follows the way pages come (Set up PsiWeb > Pages says which). "
	  "Fetched directly or by the Atom modem helper, a big site's page starts with a screen of "
	  "menus, so Text only is the standard there; PsiProxy on a computer tidies pages before they "
	  "arrive, so Full is the standard with it. A style you choose stays on every route (without "
	  "PsiProxy, if it is Full or Lighter, PsiWeb offers Text only once). Page styles make "
	  "reading easier, but do not make loading faster. The style is ticked in View > Page style and "
	  "shown in Page information, with how "
	  "much of the page was kept.\n"
	  "\n"
	  "Set up PsiWeb > Pages > Fetch the next page ahead: while you read, PsiWeb fetches the page "
	  "you are likely to open next - an article's next part, the first headlines of a front page, "
	  "or the link the cursor rests on - so it opens at once. It is on as standard with PsiProxy "
	  "on a computer and off on a modem or Direct; it is used only by the network, only while "
	  "pages come quickly and only with memory to spare, and what it fetched goes when memory is "
	  "short.\n"
	  "\n"
	  "A line at the top of a page says when something was left out: a page cut short at the "
	  "machine's limit (Show the rest), pictures left out (Show pictures), or a page drawn by "
	  "scripts, which PsiWeb cannot run (Try the reader service, FrogFind: someone else's service, "
	  "which sees the address).",
	  "The netBook keeps up to 2 MB of a lightened page.",
	  "The Series 5mx keeps up to 512 KB of a lightened page." },

	{ "Reading pages",
	  "Up and Down scroll a line; Fn+Up and Fn+Down (Pg Up and Pg Dn) a screen; Go > Top of page and "
	  "End of page go to the ends. Tap a link to follow it, a field to type in it, a button to press "
	  "it; once a field is chosen, type into it. Left and Right scroll a wide page sideways. The "
	  "scroll bar beside the page, there while the page is longer than the screen, works with "
	  "the pen.\n"
	  "\n"
	  "View > Zoom in and Zoom out (Ctrl+M and Shift+Ctrl+M, or the Zoom icons beside the screen) "
	  "go round the sizes from 50% to 200%; Normal size is 100%. Each machine keeps its own size, "
	  "so settings copied from a 5mx to a netBook do not bring the 5mx's along.\n"
	  "\n"
	  "View > Show pictures (Ctrl+I, or the Pictures button) fetches the pictures of the page "
	  "showing, and takes them away again; the button stays pressed in while they show. Pictures "
	  "on every page is set in Tools > Preferences.\n"
	  "\n"
	  "View > Page information (Shift+Ctrl+Q, or a tap on the padlock at the end of the toolband) shows the page's title "
	  "and address, what the engine is doing, the free memory, how the page came (by modem or "
	  "network, and through the web helper or not) and whether it is secure and checked, and the "
	  "page's style, its size and the pictures left out.\n"
	  "\n"
	  "A file PsiWeb cannot show can be saved to a disk, as big as the disk has room for (less a "
	  "little for the system): the bottom left shows how much has come, and Esc stops it. On a long page with pictures, those far from the screen "
	  "are let go and made again when you come back; a page that still does not fit says \"Page "
	  "too big\" and shows what fits.",
	  0, 0 },

	{ "Favourites & history",
	  "Favourites > Add to favourites (Shift+Ctrl+D) saves the page showing: the name is the page's "
	  "title and can be changed. The first five favourites are on the Favourites menu itself; More "
	  "favourites lists them all. Organise favourites has Edit and Delete, which act at once. "
	  "PsiWeb keeps up to 40.\n"
	  "\n"
	  "Go > History lists the last five pages you have seen, newest first; Full history lists the "
	  "last 50, and Clear history removes them all. File > Open address has a Recent line with the "
	  "last twelve.\n"
	  "\n"
	  "Favourites and History are kept in Favourites.dat and History.dat, in PsiWeb's folder on "
	  "C:, and stay when PsiWeb closes.",
	  0, 0 },

	{ "Screen",
	  "View > Bright screen (Shift+Ctrl+B) raises the "
	  "screen's contrast and keeps the backlight on while PsiWeb is in front; your own settings "
	  "come back when it goes to the background or closes.\n"
	  "\n"
	  "Tools > Preferences > Text: Sharp (the standard) draws text with fonts made ahead for the "
	  "screen at 100%; Scaled uses Links' own fonts at every size.",
	  "On the netBook the page is drawn in EPOC's 256 colours: text takes the nearest colour, and "
	  "pictures are blended from the colours.",
	  "On the Series 5mx pictures are dithered into 16 greys. Tools > Preferences > Screen greys "
	  "shows a picture four ways: choose the one that looks best with 1 to 4, then Up and Down for "
	  "lighter or darker, and Enter. PsiTerm and PsiMail use the same greys." },

	{ "If the engine stops",
	  "The browser engine is a separate program. If it stops, PsiWeb shows a message on the page. "
	  "Tools > Troubleshooting > Restart browser engine starts it again. Settings "
	  "the engine reads when it starts also restart it, on the same page.\n"
	  "\n"
	  "PsiWeb keeps no log unless Tools > Troubleshooting > Keep a log is ticked. Then it writes what it does "
	  "(the addresses of the pages, the connection's own lines) to PsiWeb.log, for whoever is "
	  "looking into a problem. Send log to helper sends it to PsiProxy on a computer, as plain "
	  "HTTP: it can hold the names of the sites you visited.",
	  0, 0 },

	{ "Updating PsiWeb",
	  "Tools > Update PsiWeb looks for a newer PsiWeb on GitHub (the published releases, or the test "
	  "builds) or on a local server on your own network, downloads it, checks its signature and "
	  "offers to install it. Esc stops the download. PsiWeb closes while the installer runs.\n"
	  "\n"
	  "Tools > About PsiWeb (Shift+Ctrl+A) shows the version you have.",
	  0, 0 },

	{ "Keyboard shortcuts",
	  "Up/Down, Fn+Up/Fn+Down (Pg Up/Pg Dn): scroll\n"
	  "Esc: stop loading\n"
	  "\n"
	  "Ctrl+L / Ctrl+O: type an address or words / open an address\n"
	  "Ctrl+H / Ctrl+R: home page / reload\n"
	  "Ctrl+B / Ctrl+F: back / forward\n"
	  "(Ctrl+F and Ctrl+H are Find and Replace in Psion's own programs: PsiWeb has neither, so "
	  "they go forward and home, as in other browsers)\n"
	  "Ctrl+M / Shift+Ctrl+M, or the Zoom icons: zoom in / out\n"
	  "Ctrl+I: show pictures (this page, on or off)\n"
	  "Ctrl+T / Shift+Ctrl+T: toolbar / toolband\n"
	  "Shift+Ctrl+R: the next page style\n"
	  "Shift+Ctrl+B: Bright screen\n"
	  "Shift+Ctrl+Q: page information\n"
	  "Shift+Ctrl+D: add to favourites\n"
	  "Ctrl+U: disconnect\n"
	  "Ctrl+K / Shift+Ctrl+H / Shift+Ctrl+A / Ctrl+E: preferences / help / about / close",
	  0, 0 }
	};

static const TInt KHelpCount = sizeof(KHelpTopics) / sizeof(KHelpTopics[0]);

// ----- the text, with its scroll bar (0.99; 1.01: whole lines) ----------------
//
// As PsiMail's Help (pmhelp.cpp CPmHelpText): a read-only rich text editor
// with a scroll bar drawn beside it, the text scrolled with its view: Up and
// Down a line, Pg Up and Pg Dn a page, Home and End, and the pen on the bar.
// Up at the top goes back to the Topic line. The line's resource is the
// editor's (RTXTED); the box is whole lines high: 7 on the 5mx's 240 lines,
// 12 on 480 (the style sheet's Help), whatever the resource says.
// (1.01) The text moves a whole line at a time (the view's line at the top,
// SetViewLineAtTopL), so the top line is never cut: ScrollDisplayL had moved
// it by pixels and left the last line at the bottom, a part line at the top.
// The bar is drawn only when the topic is longer than the box; a topic that
// fits has none, and its text takes the bar's room. The line height is the
// text's own (the netBook's fonts can change), 17 if it cannot be read.

const TInt KHelpBarW = 23;                 // EIKON's scroll bar width
const TInt KHelpLineH = 17;                // a line of the help's text (the normal font's), if not read
const TInt KHelpButH = 20;                 // the bar's arrow buttons

CPwHelpText::~CPwHelpText()
	{
	delete iEditor;
	}

void CPwHelpText::ConstructFromResourceL(TResourceReader& aReader)
	{
	iEditor = new(ELeave) CEikRichTextEditor(TEikBorder(TEikBorder::ENone));
	if (DrawableWindow())
		iEditor->SetContainerWindowL(*this);
	iEditor->ConstructFromResourceL(aReader);
	}

void CPwHelpText::SetContainerWindowL(const CCoeControl& aContainer)
	{
	CCoeControl::SetContainerWindowL(aContainer);
	if (iEditor)
		iEditor->SetContainerWindowL(*this);
	}

// the text's own line height (its first formatted line)
TInt CPwHelpText::LineHeight() const
	{
	CTextLayout* lay = iEditor ? iEditor->TextLayout() : NULL;
	TInt h = 0;
	if (lay && lay->NumFormattedLines() > 0)
		{
		TRect line;
		lay->GetLineRect(1, line);
		h = line.Height();
		}
	return h >= 8 && h <= 40 ? h : KHelpLineH;
	}

TSize CPwHelpText::MinimumSize()
	{
	TBool tall = iEikonEnv->ScreenDevice()->SizeInPixels().iHeight >= 400;
	TInt lines = tall ? 12 : 7;
	return TSize(iEditor->MinimumSize().iWidth + KHelpBarW + 2, lines * LineHeight() + 2);
	}

void CPwHelpText::SizeChangedL()
	{
	Layout();
	}

void CPwHelpText::PositionChanged()
	{
	Layout();
	}

// the whole text formatted (an edwin formats only the band in view once its
// text is long), so its lines can be counted and any one put at the top
void CPwHelpText::FormatL()
	{
	CTextView* view = iEditor->TextView();
	if (!view || !iEditor->TextLayout())
		return;
	iEditor->TextLayout()->SetAmountToFormat(CTextLayout::EFFormatAllText);
	view->FormatTextL();
	}

TInt CPwHelpText::Count() const
	{
	CTextLayout* lay = iEditor->TextLayout();
	return lay ? lay->NumFormattedLines() : 0;
	}

TInt CPwHelpText::Shown() const
	{
	return iLineH > 0 ? iEditor->Rect().Height() / iLineH : 0;
	}

// the text's box: whole lines high (any pixels over stay blank below it), as
// wide as the line less the bar when the text is longer than the box
void CPwHelpText::Layout()
	{
	TRAPD(err, LayoutL());
	(void)err;
	}

void CPwHelpText::LayoutL()
	{
	if (!iEditor)
		return;
	TRect r = Rect();
	r.Shrink(1, 1);
	if (r.Width() <= KHelpBarW + 8 || r.Height() <= 8)
		return;
	iLineH = LineHeight();
	TInt h = (r.Height() / iLineH) * iLineH;
	if (h <= 0)
		h = r.Height();
	iEditor->SetRectL(TRect(r.iTl, TSize(r.Width() - KHelpBarW, h)));
	FormatL();
	if (Count() > Shown())
		iBar = TRect(r.iBr.iX - KHelpBarW, r.iTl.iY, r.iBr.iX, r.iBr.iY);
	else
		{
		iBar = TRect();
		iEditor->SetRectL(TRect(r.iTl, TSize(r.Width(), h)));
		FormatL();
		}
	iTopLine = 0;
	}

TInt CPwHelpText::CountComponentControls() const
	{
	return iEditor ? 1 : 0;
	}

CCoeControl* CPwHelpText::ComponentControl(TInt /*aIndex*/) const
	{
	return iEditor;
	}

// the parts of the bar: the shaft, the thumb in it, the two arrow buttons
// at the foot (as EIKON's own bars); the model in lines
static void HelpBarParts(const TRect& r, TInt aTotal, TInt aShown, TInt aAbove,
	TRect& aShaft, TRect& aThumb, TRect& aUp, TRect& aDown)
	{
	aDown = TRect(r.iTl.iX, r.iBr.iY - KHelpButH, r.iBr.iX, r.iBr.iY);
	aUp = TRect(r.iTl.iX, r.iBr.iY - 2 * KHelpButH, r.iBr.iX, r.iBr.iY - KHelpButH);
	aShaft = TRect(r.iTl.iX, r.iTl.iY, r.iBr.iX, aUp.iTl.iY);
	if (aTotal < aShown) aTotal = aShown;
	TInt sh = aShaft.Height();
	TInt th = aTotal > 0 ? sh * aShown / aTotal : sh;
	if (th < 16) th = 16;
	if (th > sh) th = sh;
	TInt room = aTotal - aShown;
	TInt ty = aShaft.iTl.iY + (room > 0 ? (sh - th) * Min(aAbove, room) / room : 0);
	aThumb = TRect(aShaft.iTl.iX, ty, aShaft.iBr.iX, ty + th);
	}

// a raised face (the thumb, the buttons): EIKON's button colours, so the
// themes reach it; pressed in while held
static void HelpFace(CWindowGc& aGc, const CCoeControl& aC, const TRect& aR, TBool aIn)
	{
	CEikonEnv* env = CEikonEnv::Static();
	aGc.SetPenStyle(CGraphicsContext::ESolidPen);
	aGc.SetPenColor(KRgbBlack);
	aGc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	aGc.SetBrushColor(env->ControlColor((TEikLogicalColor)(aIn ? EEikColorButtonFaceClearPressed : EEikColorButtonFaceClear), aC));
	aGc.DrawRect(aR);
	TRect r(aR);
	r.Shrink(1, 1);
	aGc.SetPenColor(aIn ? KRgbDarkGray : KRgbWhite);
	aGc.DrawLine(r.iTl, TPoint(r.iBr.iX - 1, r.iTl.iY));
	aGc.DrawLine(r.iTl, TPoint(r.iTl.iX, r.iBr.iY - 1));
	aGc.SetPenColor(aIn ? KRgbWhite : KRgbDarkGray);
	aGc.DrawLine(TPoint(r.iTl.iX, r.iBr.iY - 1), TPoint(r.iBr.iX, r.iBr.iY - 1));
	aGc.DrawLine(TPoint(r.iBr.iX - 1, r.iTl.iY), TPoint(r.iBr.iX - 1, r.iBr.iY));
	}

static void HelpArrow(CWindowGc& aGc, const TRect& aR, TBool aUp)
	{
	aGc.SetPenColor(KRgbBlack);
	TInt cx = aR.iTl.iX + aR.Width() / 2, cy = aR.iTl.iY + aR.Height() / 2;
	for (TInt k = 0; k < 4; k++)
		{
		TInt y = aUp ? cy - 2 + k : cy + 1 - k;
		aGc.DrawLine(TPoint(cx - k, y), TPoint(cx + k + 1, y));
		}
	}

void CPwHelpText::Draw(const TRect& /*aRect*/) const
	{
	CWindowGc& gc = SystemGc();
	// the edit line's edge, as the dialog's other lines have it
	gc.SetPenStyle(CGraphicsContext::ESolidPen);
	gc.SetPenColor(KRgbBlack);
	gc.SetBrushStyle(CGraphicsContext::ENullBrush);
	gc.DrawRect(Rect());
	// (1.01) white under the last whole line (the editor draws its own)
	if (iEditor)
		{
		TRect r = Rect();
		r.Shrink(1, 1);
		TRect below(r.iTl.iX, iEditor->Rect().iBr.iY, iBar.Width() > 0 ? iBar.iTl.iX : r.iBr.iX, r.iBr.iY);
		if (below.Height() > 0)
			{
			gc.SetPenStyle(CGraphicsContext::ENullPen);
			gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
			gc.SetBrushColor(KRgbWhite);
			gc.DrawRect(below);
			}
		}
	DrawBar(gc);
	}

void CPwHelpText::DrawBar(CWindowGc& aGc) const
	{
	if (iBar.Width() <= 0)
		return;                              // (1.01) a topic that fits: no bar
	TRect shaft, thumb, up, down;
	HelpBarParts(iBar, Count(), Shown(), iTopLine, shaft, thumb, up, down);
	aGc.SetPenStyle(CGraphicsContext::ESolidPen);
	aGc.SetPenColor(KRgbBlack);
	aGc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	aGc.SetBrushColor(iEikonEnv->ControlColor((TEikLogicalColor)EEikColorScrollBarShaft, *this));
	aGc.DrawRect(shaft);
	HelpFace(aGc, *this, thumb, iPress == 3);
	HelpFace(aGc, *this, up, iPress == 1);
	HelpArrow(aGc, up, ETrue);
	HelpFace(aGc, *this, down, iPress == 2);
	HelpArrow(aGc, down, EFalse);
	}

void CPwHelpText::UpdateBar()
	{
	if (!IsReadyToDraw() || iBar.Width() <= 0)
		return;
	ActivateGc();
	DrawBar(SystemGc());
	DeactivateGc();
	}

void CPwHelpText::TextChangedL()
	{
	iTopLine = 0;
	iPress = 0;
	if (iEditor->TextView())
		iEditor->TextView()->SetCursorVisibilityL(TCursor::EFCursorInvisible, TCursor::EFCursorInvisible);
	LayoutL();
	}

// (1.01) a whole line at the top (aTop: from 0), kept within the text
void CPwHelpText::ScrollToL(TInt aTop)
	{
	TInt most = Count() - Shown();
	if (aTop > most)
		aTop = most;
	if (aTop < 0)
		aTop = 0;
	if (aTop == iTopLine || !iEditor->TextView())
		return;
	TPoint moved = iEditor->TextView()->SetViewLineAtTopL(aTop + 1);
	iTopLine = aTop;
	if (moved.iY == CTextLayout::EFScrollRedrawWholeScreen)
		iEditor->DrawNow();
	UpdateBar();
	}

TKeyResponse CPwHelpText::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
	{
	if (aType != EEventKey)
		return EKeyWasNotConsumed;
	TInt page = Shown() > 1 ? Shown() - 1 : 1;
	switch (aKeyEvent.iCode)
		{
	case EKeyUpArrow:
		if (iTopLine > 0)
			ScrollToL(iTopLine - 1);
		else if (iDialog)
			iDialog->TopicLineL();           // at the top: back to the Topic line
		break;
	case EKeyDownArrow:
		ScrollToL(iTopLine + 1);
		break;
	case EKeyPageUp:
		ScrollToL(iTopLine - page);
		break;
	case EKeyPageDown:
		ScrollToL(iTopLine + page);
		break;
	case EKeyHome:
		ScrollToL(0);
		break;
	case EKeyEnd:
		ScrollToL(Count());
		break;
	default:
		return EKeyWasNotConsumed;
		}
	return EKeyWasConsumed;
	}

// the pen on the bar: the arrows a line, the shaft a page, the thumb dragged
void CPwHelpText::HandlePointerEventL(const TPointerEvent& aEvent)
	{
	if (iBar.Width() <= 0)
		return;                              // (the text: nothing to select)
	TPoint p = aEvent.iPosition;
	TRect shaft, thumb, up, down;
	HelpBarParts(iBar, Count(), Shown(), iTopLine, shaft, thumb, up, down);
	TInt page = Shown() > 1 ? Shown() - 1 : 1;
	if (aEvent.iType == TPointerEvent::EButton1Down)
		{
		iPress = 0;
		if (!iBar.Contains(p))
			return;
		if (up.Contains(p)) { iPress = 1; ScrollToL(iTopLine - 1); }
		else if (down.Contains(p)) { iPress = 2; ScrollToL(iTopLine + 1); }
		else if (thumb.Contains(p)) { iPress = 3; iGrab = p.iY - thumb.iTl.iY; }
		else ScrollToL(p.iY < thumb.iTl.iY ? iTopLine - page : iTopLine + page);
		UpdateBar();
		return;
		}
	if (aEvent.iType == TPointerEvent::EDrag && iPress == 3)
		{
		TInt room = shaft.Height() - thumb.Height(), most = Count() - Shown();
		if (room > 0 && most > 0)
			ScrollToL(((p.iY - iGrab - shaft.iTl.iY) * most + room / 2) / room);
		return;
		}
	if (aEvent.iType == TPointerEvent::EButton1Up && iPress)
		{
		iPress = 0;
		UpdateBar();
		}
	}

SEikControlInfo CPwHelpDialog::CreateCustomControlL(TInt aControlType)
	{
	SEikControlInfo info;
	info.iControl = NULL;
	info.iTrailerTextId = 0;
	info.iFlags = 0;
	if (aControlType == EPwCtHelpText)
		{
		CPwHelpText* t = new(ELeave) CPwHelpText;
		t->iDialog = this;
		info.iControl = t;
		}
	return info;
	}

void CPwAppUi::HelpL()
	{
	CPwHelpDialog* dlg = new(ELeave) CPwHelpDialog(0);
	dlg->ExecuteLD(R_PW_HELP_DIALOG);   // (0.99) one resource; the text sized by the screen (CPwHelpText)
	}

void CPwHelpDialog::PreLayoutDynInitL()
	{
	CDesCArrayFlat* titles = new(ELeave) CDesCArrayFlat(KHelpCount);
	CleanupStack::PushL(titles);
	TBuf<40> t;
	for (TInt i = 0; i < KHelpCount; i++)
		{
		t.Copy(TPtrC8((const TUint8*)KHelpTopics[i].iTitle));
		titles->AppendL(t);
		}
	CleanupStack::Pop();
	CEikChoiceList* cl = (CEikChoiceList*)Control(EPwDlgHelpTopic);
	cl->SetArrayL(titles);                  // it owns the array now
	if (iTopic < 0 || iTopic >= KHelpCount)
		iTopic = 0;
	cl->SetCurrentItem(iTopic);
	ShowTopicL(iTopic);
	}

// the topic's text: its title in bold, then the paragraphs
void CPwHelpDialog::ShowTopicL(TInt aTopic)
	{
	const TPwHelpTopic& topic = KHelpTopics[aTopic];
	TPtrC8 title((const TUint8*)topic.iTitle);
	// (0.85) the text, then this machine's own paragraph
	TBool tall = iEikonEnv->ScreenDevice()->SizeInPixels().iHeight >= 400;
	const char* own = tall ? topic.iTall : topic.iShort;
	HBufC8* all = HBufC8::NewLC(TPtrC8((const TUint8*)topic.iText).Length() + (own ? TPtrC8((const TUint8*)own).Length() + 2 : 0));
	TPtr8 ap = all->Des();
	ap.Copy(TPtrC8((const TUint8*)topic.iText));
	if (own)
		{
		ap.Append(_L8("\n\n"));
		ap.Append(TPtrC8((const TUint8*)own));
		}
	TPtrC8 text(*all);
	// (1.01) trailing new lines dropped: blank lines after the text made a
	// topic that fits look longer than the box
	TInt n = text.Length();
	while (n > 0 && text[n - 1] == '\n')
		n--;
	text.Set(text.Left(n));
	HBufC* b = HBufC::NewLC(title.Length() + text.Length() + 4);
	TPtr p = b->Des();
	p.Copy(title);
	p.Append(CEditableText::EParagraphDelimiter);
	for (TInt i = 0; i < text.Length(); i++)
		{
		TUint8 c = text[i];
		if (c == '\n')
			p.Append(CEditableText::EParagraphDelimiter);
		else
			p.Append((TText)c);
		}
	CPwHelpText* box = (CPwHelpText*)Control(EPwDlgHelpText);
	CEikRichTextEditor* ed = box->Editor();
	ed->SetTextL(b);
	TCharFormat cf;
	TCharFormatMask cm;
	cf.iFontSpec.iFontStyle.SetStrokeWeight(EStrokeWeightBold);
	cm.SetAttrib(EAttFontStrokeWeight);
	ed->RichText()->ApplyCharFormatL(cf, cm, 0, title.Length());
	ed->HandleTextChangedL();
	ed->SetCursorPosL(0, EFalse);
	box->TextChangedL();                    // (at its top, the bar up to date)
	if (IsActivated())
		box->DrawDeferred();                // (1.01: the bar shown or not, the text's width with it)
	CleanupStack::PopAndDestroy(2);         // b, all
	}

// the keys start on the topic list (Left and Right change the topic; Down
// moves into the text, where Up and Down scroll it)
void CPwHelpDialog::PostLayoutDynInitL()
	{
	TryChangeFocusToL(EPwDlgHelpTopic);
	}

void CPwHelpDialog::HandleControlStateChangeL(TInt aControlId)
	{
	if (aControlId != EPwDlgHelpTopic)
		return;
	TInt t = ((CEikChoiceList*)Control(EPwDlgHelpTopic))->CurrentItem();
	if (t != iTopic && t >= 0 && t < KHelpCount)
		{
		iTopic = t;
		ShowTopicL(t);
		}
	}

TBool CPwHelpDialog::OkToExitL(TInt /*aButtonId*/)
	{
	return ETrue;
	}
