// pwstart.cpp - the start page (0.87): about:start, PsiWeb's first page and
// Go > Start page. PsiWeb.app makes it (the engine serves it from the
// shared chunk, psiweb.h start_page): a search box, the speed dial - the
// first eight favourites, so Organise favourites is its editor - the pages
// seen last, and the route in plain words. One source for both machines;
// the netBook's 480 lines get bigger tiles and a Recent list, the 5mx's 240
// a line of each. Keys 1 to 8 open the tiles (CPwView::OfferKeyEventL).
#include "pwapp.h"
#include "pwaddr.h"
#include "pwhtml.h"

// A new PsiWeb's favourites, which are its first tiles: light sites that
// work direct (the speed dial's eighth tile is Add... until there are eight)
struct TPwDefaultFav { const TText* iName; const TText* iUrl; };
static const TPwDefaultFav KDefaultFavs[] =
	{
	{ _S("Search"), _S("https://html.duckduckgo.com/html/") },
	{ _S("68k.news"), _S("http://68k.news/") },
	{ _S("BBC News"), _S("https://www.bbc.co.uk/news") },
	{ _S("Wikipedia"), _S("https://en.m.wikipedia.org/") },
	{ _S("CNN lite"), _S("https://lite.cnn.com/") },
	{ _S("NPR text"), _S("https://text.npr.org/") },
	{ _S("Hacker News"), _S("https://news.ycombinator.com/") }
	};
const TInt KTiles = 8;

// the first start of a new PsiWeb (no Favourites.dat): the standard tiles
void CPwAppUi::SeedFavouritesL()
	{
	TEntry e;
	if (iFavs->Count() > 0 || iCoeEnv->FsSession().Entry(_L("C:\\System\\Apps\\PsiWeb\\Favourites.dat"), e) == KErrNone)
		return;
	for (TUint i = 0; i < sizeof(KDefaultFavs) / sizeof(KDefaultFavs[0]); i++)
		iFavs->AddEnd(TPtrC(KDefaultFavs[i].iName), TPtrC(KDefaultFavs[i].iUrl));
	}

// (0.94) Every append is checked: eight favourites with long addresses
// could run past the page's buffer (a panic at every start). What does not
// fit is left out, and KReserve stays free for the closing tags.
const TInt KReserve = 32;

static void Add(TDes8& aOut, const TDesC8& aText)
	{
	TInt room = aOut.MaxLength() - KReserve - aOut.Length();
	if (room > 0)
		aOut.Append(aText.Left(aText.Length() < room ? aText.Length() : room));
	}

static void AddNumber(TDes8& aOut, const TDesC8& aFormat, TInt aNumber)
	{
	TBuf8<200> b;
	b.Format(aFormat, aNumber);
	Add(aOut, b);
	}

// HTML for 8-bit text: & < > " made safe, control characters as spaces
static void Esc(TDes8& aOut, const TDesC& aText, TInt aMax)
	{
	TInt n = aText.Length() < aMax ? aText.Length() : aMax;
	for (TInt i = 0; i < n && aOut.Length() < aOut.MaxLength() - KReserve - 8; i++)
		{
		TUint c = aText[i];
		if (c == '&') aOut.Append(_L8("&amp;"));
		else if (c == '<') aOut.Append(_L8("&lt;"));
		else if (c == '>') aOut.Append(_L8("&gt;"));
		else if (c == '"') aOut.Append(_L8("&quot;"));
		else aOut.Append((TUint8)(c < 0x20 ? ' ' : c));
		}
	if (aText.Length() > n && aOut.Length() < aOut.MaxLength() - KReserve - 1)
		aOut.Append((TUint8)0x85);                 // "..." (Windows-1252)
	}

// (0.98) "<a href=\"address\">" with the whole address, or nothing (EFalse)
// when it does not fit: never a cut address, which would open another page
static TBool AddLinkStart(TDes8& aOut, const TDesC& aUrl)
	{
	TInt start = aOut.Length();
	TInt room = aOut.MaxLength() - KReserve - start - 11;    // <a href=" and ">
	if (room <= 0)
		return EFalse;
	aOut.Append(_L8("<a href=\""));
	TInt n = pw_attr_esc((unsigned char*)aOut.Ptr() + aOut.Length(), room, (const unsigned char*)aUrl.Ptr(), aUrl.Length());
	if (n < 0)
		{
		aOut.SetLength(start);
		return EFalse;
		}
	aOut.SetLength(aOut.Length() + n);
	aOut.Append(_L8("\">"));
	return ETrue;
	}

// The tiles' headers in EPOC's own colours on the netBook (its dark blue,
// maroon and teal); the 5mx's dark grey, with the number in white
static const TText8* const KTileColours[] = { _S8("#000080"), _S8("#800000"), _S8("#008080"), _S8("#000080") };

void CPwAppUi::UpdateStartPageL()
	{
	PwShared* s = iView ? iView->Shared() : NULL;
	if (!s || !iFavs || !iHist)
		return;
	TBool tall = TallScreen();
	HBufC8* page = HBufC8::NewLC(sizeof(s->start_page) - 1);
	TPtr8 p = page->Des();
	Add(p, _L8("<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=windows-1252\">"
		"<title>PsiWeb</title></head><body>"));
	// the search box: the search page chosen in Preferences
	if (iView->Settings().iSearch == PW_SEARCH_FROGFIND)
		Add(p, _L8("<form action=\"http://frogfind.com/\" method=\"get\">"));
	else
		Add(p, _L8("<form action=\"https://html.duckduckgo.com/html/\" method=\"get\">"));
	AddNumber(p, _L8("<b>PsiWeb</b> &nbsp; <input type=\"text\" name=\"q\" size=\"%d\"> <input type=\"submit\" value=\"Search\"></form>"),
		tall ? 44 : 30);
	if (tall)
		Add(p, _L8("<p><small>Speed dial - press 1 to 8, or tap</small></p>"));
	// the speed dial: 4 x 2
	Add(p, _L8("<table width=\"100%\" cellspacing=\"4\" cellpadding=\"0\">"));
	TInt n = iFavs->Count() < KTiles ? iFavs->Count() : KTiles;
	for (TInt t = 0; t < KTiles; t++)
		{
		if (t % 4 == 0)
			Add(p, _L8("<tr>"));
		Add(p, _L8("<td width=\"25%\" valign=\"top\"><table width=\"100%\" border=\"1\" cellspacing=\"0\" cellpadding=\""));
		Add(p, tall ? _L8("4") : _L8("2"));
		Add(p, _L8("\"><tr><td bgcolor=\""));
		Add(p, TPtrC8(tall ? KTileColours[t % 4] : (const TText8*)"#555555"));
		AddNumber(p, _L8("\"><font color=\"#ffffff\"><b>%d</b></font></td></tr><tr><td>"), t + 1);
		if (t < n)
			{
			const TPwEntry& e = iFavs->At(t);
			TBool link = AddLinkStart(p, e.iUrl);
			Esc(p, e.iTitle.Length() ? (const TDesC&)e.iTitle : (const TDesC&)e.iUrl, tall ? 16 : 11);
			if (link)
				Add(p, _L8("</a>"));
			}
		else if (t == n)
			Add(p, _L8("<a href=\"about:cmd-favs\">Add...</a>"));
		else
			Add(p, _L8("&nbsp;"));
		Add(p, _L8("</td></tr></table></td>"));
		if (t % 4 == 3)
			Add(p, _L8("</tr>"));
		}
	Add(p, _L8("</table>"));
	// the route, in plain words, with the way to change it
	TBuf<120> route;
	RouteWords(iView->Settings(), route);
	const TPwSettings& st = iView->Settings();
	TBool helper = st.iUseProxy && st.iProxyHost.Length();
	if (tall)
		{
		// the pages seen last
		if (iHist->Count() > 0)
			{
			Add(p, _L8("<p><b>Recent</b></p><ul>"));
			for (TInt i = 0; i < iHist->Count() && i < 4; i++)
				{
				const TPwEntry& e = iHist->At(i);
				Add(p, _L8("<li>"));
				TBool link = AddLinkStart(p, e.iUrl);
				Esc(p, e.iTitle.Length() ? (const TDesC&)e.iTitle : (const TDesC&)e.iUrl, 60);
				if (link)
					Add(p, _L8("</a>"));
				Add(p, _L8("</li>"));
				}
			Add(p, _L8("</ul>"));
			}
		Add(p, _L8("<table width=\"100%\" border=\"1\" cellspacing=\"0\" cellpadding=\"4\"><tr><td>Connection: "));
		Esc(p, route, 100);
		Add(p, _L8("<br><small>"));
		Add(p, helper ? _L8("Secure sites are checked by the helper, which sees the pages.")
		                : _L8("Secure sites are checked on this Psion."));
		Add(p, _L8(" Pages drawn by scripts may not work.</small> &nbsp; "
			"<a href=\"about:cmd-setup\">Change...</a> &nbsp; <a href=\"about:cmd-help\">What works</a></td></tr></table>"));
		}
	else
		{
		Add(p, _L8("<p><small>Connection: "));
		Esc(p, route, 100);
		Add(p, _L8(" &nbsp; <a href=\"about:cmd-setup\">Change...</a> &nbsp; <a href=\"about:cmd-help\">What works</a>"));
		if (iHist->Count() > 0)
			{
			Add(p, _L8("<br>Recent: "));
			for (TInt i = 0; i < iHist->Count() && i < 2; i++)
				{
				const TPwEntry& e = iHist->At(i);
				if (i) Add(p, _L8(" | "));
				TBool link = AddLinkStart(p, e.iUrl);
				Esc(p, e.iTitle.Length() ? (const TDesC&)e.iTitle : (const TDesC&)e.iUrl, 30);
				if (link)
					Add(p, _L8("</a>"));
				}
			}
		Add(p, _L8("</small></p>"));
		}
	p.Append(_L8("</body></html>"));       // (KReserve keeps room for it)
	TInt len = p.Length() < (TInt)sizeof(s->start_page) - 1 ? p.Length() : (TInt)sizeof(s->start_page) - 1;
	Mem::Copy(s->start_page, p.Ptr(), len);
	s->start_page[len] = 0;
	CleanupStack::PopAndDestroy();          // page
	}

// (0.98) after the favourites change from the start page (Add..., Organise):
// the page showing is laid out again, so the new tile is there at once
void CPwAppUi::RefreshStartPageL()
	{
	UpdateStartPageL();
	PwShared* s = iView ? iView->Shared() : NULL;
	if (s && iView->EngineRunning() && !s->busy
		&& TPtrC8((const TUint8*)s->url).Compare(_L8("about:start")) == 0)
		iView->Command(PW_CMD_RELOAD, KNullDesC);
	}

// a tile chosen with 1 to 8 (the start page's own links do the same by pen)
void CPwAppUi::OpenTileL(TInt aIndex)
	{
	TInt n = iFavs->Count() < KTiles ? iFavs->Count() : KTiles;
	if (aIndex < n)
		OpenFromListL(*iFavs, aIndex);
	else if (aIndex == n)
		AddNewFavouriteL();                  // (0.98) the Add... tile
	}
