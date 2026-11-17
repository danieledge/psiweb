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
	  "Tools > Set up PsiWeb chooses how the Psion goes online and how pages reach it, the first "
	  "time PsiWeb starts and whenever you like. A page that cannot be opened says why, with Try "
	  "again, Test the connection and Set up PsiWeb on it.",
	  "On the netBook the toolband is shown and the toolbar hidden, so the page has the whole "
	  "screen, in colour.",
	  "On the Series 5mx the toolbar is shown on the right; View > Show > Toolband (Shift+Ctrl+T) "
	  "adds the address along the top. Pages are in 16 greys." },

	{ "Ways to connect",
	  "How the Psion goes online is set once for PsiWeb, PsiMail and PsiTerm (Tools > Connection "
	  "settings, or Set up PsiWeb): a network card or dial-up (the Psion's own TCP/IP, set up in "
	  "the Control panel), or a WiFi modem on the serial port, such as the PsiConnect Atom.\n"
	  "\n"
	  "Pages then reach PsiWeb in one of three ways:\n"
	  "\n"
	  "Direct: PsiWeb fetches pages itself. Nothing extra is needed. Secure sites (https) are "
	  "checked by the Psion itself and nobody in between sees the page. Fast on a network card; "
	  "slow for big sites over a modem. JPEG, PNG and GIF pictures; some newer kinds show as a box. "
	  "Works wherever the Psion is online.\n"
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
	  "the web helper at the computer's address, port 8081. Tools > Log > Send log to server "
	  "needs it.\n"
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
	  "logins; cookies, until PsiWeb closes; user names and passwords that the site asks for in a "
	  "dialog; saving a file PsiWeb cannot show, such as a PDF, to a disk.\n"
	  "\n"
	  "Does not work: pages drawn by scripts (JavaScript), such as YouTube, X, the new Reddit, most "
	  "web programs and bank logins: they come out blank or nearly so. Style sheets, video and "
	  "sound. Sites that still use only old security (TLS 1.2), which say \"The site needs older "
	  "security than PsiWeb has\": they may open through PsiProxy on a computer.\n"
	  "\n"
	  "Light editions of big sites suit the Psion best: 68k.news, text.npr.org, lite.cnn.com, "
	  "en.m.wikipedia.org, old.reddit.com, html.duckduckgo.com. FrogFind (frogfind.com) searches "
	  "and can read other pages as plain text; it is someone else's service, which sees the "
	  "addresses you read.",
	  0, 0 },

	{ "Page styles",
	  "View > Page style chooses how much of a page PsiWeb keeps before it lays the page out. "
	  "Shift+Ctrl+R goes round them for the page showing, and the choice stays for the pages after.\n"
	  "\n"
	  "Full: the page as it comes.\n"
	  "Lighter: scripts, style sheets, hidden parts and tracking pictures taken out, which is most "
	  "of a modern page's size; it looks the same in PsiWeb.\n"
	  "Reader: the page's text, with its menus, sidebars, banners and lists of links taken out "
	  "too. Best for an article on a news site. A page that is all menus may come out nearly "
	  "empty: choose Lighter or Full for it.\n"
	  "Text only: as Lighter, with each picture's description in place of the picture.\n"
	  "\n"
	  "Full is the standard. The others are for reading, not for speed: on the Psion, making a "
	  "page lighter takes about as long as it saves. The Atom modem helper and PsiProxy on a "
	  "computer lighten pages before they arrive. The style is shown in the route light on the toolband (L, R or T) "
	  "and in Page information, with how much of the page was kept.\n"
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
	  "scroll bar beside the page works with the pen.\n"
	  "\n"
	  "View > Zoom in and Zoom out (Ctrl+M and Shift+Ctrl+M, or the Zoom icons beside the screen) "
	  "go round the sizes from 50% to 200%; Normal size is 100%.\n"
	  "\n"
	  "View > Show pictures (Ctrl+I, or the Pictures button) fetches the pictures of the page "
	  "showing, and takes them away again; the button stays pressed in while they show. Pictures "
	  "on every page is set in Tools > Preferences.\n"
	  "\n"
	  "View > Page information (Shift+Ctrl+Q, or a tap on the route light) shows the page's title "
	  "and address, what the engine is doing, the free memory, how the page came (Direct, the Atom "
	  "modem helper or PsiProxy on a computer) and whether it is secure and checked, and the "
	  "page's style, its size and the pictures left out.\n"
	  "\n"
	  "A file PsiWeb cannot show can be saved to a disk (up to 4 MB): the bottom left shows how "
	  "much has come, and Esc stops it. On a long page with pictures, those far from the screen "
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
	  "View > Show > Bright screen (Shift+Ctrl+B; called Reading mode before 0.85) raises the "
	  "screen's contrast and keeps the backlight on while PsiWeb is in front; your own settings "
	  "come back when it goes to the background or closes.\n"
	  "\n"
	  "Tools > Preferences > Text: Sharp (the standard) draws text with fonts made ahead for the "
	  "Psion's screen at 100%; Scaled (as before) uses Links' own fonts at every size.",
	  "On the netBook the page is drawn in EPOC's 256 colours: text takes the nearest colour, and "
	  "pictures are blended from the colours.",
	  "On the Series 5mx pictures are dithered into 16 greys. Tools > Preferences > Screen greys "
	  "shows a picture four ways: choose the one that looks best with 1 to 4, then Up and Down for "
	  "lighter or darker, and Enter. PsiTerm and PsiMail use the same greys." },

	{ "If the engine stops",
	  "The browser engine runs as its own program, so if it stops, the Psion carries on. PsiWeb "
	  "says so on the page, and Tools > Log > Restart browser engine starts it again. Settings "
	  "the engine reads when it starts also restart it, on the same page.\n"
	  "\n"
	  "PsiWeb keeps no log unless Tools > Log > Keep a log is ticked. Then it writes what it does "
	  "(the addresses of the pages, the connection's own lines) to PsiWeb.log, for whoever is "
	  "looking into a problem. Send log to server sends it to PsiProxy on a computer, as plain "
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

void CPwAppUi::HelpL()
	{
	CPwHelpDialog* dlg = new(ELeave) CPwHelpDialog(0);
	dlg->ExecuteLD(TallScreen() ? R_PW_HELP_DIALOG_TALL : R_PW_HELP_DIALOG);   // (0.87) the netBook's is the size of its own big dialogs
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
	CEikRichTextEditor* ed = (CEikRichTextEditor*)Control(EPwDlgHelpText);
	ed->SetTextL(b);
	TCharFormat cf;
	TCharFormatMask cm;
	cf.iFontSpec.iFontStyle.SetStrokeWeight(EStrokeWeightBold);
	cm.SetAttrib(EAttFontStrokeWeight);
	ed->RichText()->ApplyCharFormatL(cf, cm, 0, title.Length());
	ed->HandleTextChangedL();
	ed->SetCursorPosL(0, EFalse);
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
