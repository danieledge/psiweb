/* pwaddr.h - what was typed in the address: an address, or words to search
 * for (0.85). Plain C with no EPOC calls, so test/addr/addrtest.c checks it
 * on a PC. The app (EndAddressL, Open address) passes the text through
 * pw_addr_make before the engine sees it.
 *
 * Words go to the search page chosen in Preferences (Search with):
 *   0 DuckDuckGo's page without scripts  https://html.duckduckgo.com/html/?q=
 *   1 FrogFind                           http://frogfind.com/?q=
 * The text is the Psion's 8-bit text (Windows-1252, as FromUtf8 makes it);
 * it is sent as UTF-8, percent-encoded, spaces as '+'.
 */
#ifndef PWADDR_H
#define PWADDR_H

#define PW_SEARCH_DDG      0
#define PW_SEARCH_FROGFIND 1

static inline const char* pw_search_base(int aEngine)
	{
	return aEngine == PW_SEARCH_FROGFIND ? "http://frogfind.com/?q=" : "https://html.duckduckgo.com/html/?q=";
	}

static inline int pw_addr_starts(const unsigned char* s, int n, const char* p)
	{
	int i = 0;
	for (; p[i]; i++)
		{
		if (i >= n) return 0;
		int c = s[i];
		if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
		if (c != p[i]) return 0;
		}
	return 1;
	}

/* 1 if aText (aLen bytes, trimmed) is words to search for, 0 an address.
 * An address: a scheme ("://", or about:, mailto:, file:), or one word
 * with a dot or a colon in it (bbc.co.uk, 10.0.0.5:8081), or "localhost",
 * or a path. Words: anything with a space in it, or one word with no dot
 * (psion, "series 5mx"). */
static inline int pw_addr_is_search(const unsigned char* aText, int aLen)
	{
	if (aLen <= 0)
		return 0;
	if (pw_addr_starts(aText, aLen, "about:") || pw_addr_starts(aText, aLen, "mailto:")
		|| pw_addr_starts(aText, aLen, "file:") || pw_addr_starts(aText, aLen, "localhost"))
		return 0;
	int space = 0, dot = 0, colon = 0, i;
	for (i = 0; i < aLen; i++)
		{
		if (aText[i] == ' ') space = 1;
		else if (aText[i] == '.') dot = 1;
		else if (aText[i] == ':') colon = 1;
		}
	for (i = 0; i + 2 < aLen; i++)
		if (aText[i] == ':' && aText[i + 1] == '/' && aText[i + 2] == '/')
			return 0;
	if (aText[0] == '/')
		return 0;
	if (space)
		return 1;
	return !dot && !colon;
	}

/* Windows-1252 0x80..0x9f to Unicode (the rest are Latin-1) */
static inline unsigned pw_addr_cp1252(unsigned c)
	{
	static const unsigned short hi[32] = {
		0x20ac, 0x81, 0x201a, 0x192, 0x201e, 0x2026, 0x2020, 0x2021, 0x2c6, 0x2030, 0x160, 0x2039, 0x152, 0x8d, 0x17d, 0x8f,
		0x90, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0x2dc, 0x2122, 0x161, 0x203a, 0x153, 0x9d, 0x17e, 0x178 };
	return c >= 0x80 && c < 0xa0 ? hi[c - 0x80] : c;
	}

/* Makes the address to open in aOut (aMax bytes with its 0; ASCII): the
 * text itself, or the search page with the words. Returns the length, or
 * -1 if it did not fit (the caller then says "Too many words to search
 * for" and opens nothing). */
static inline int pw_addr_make(const unsigned char* aText, int aLen, int aEngine, char* aOut, int aMax)
	{
	int n = 0, i;
	if (aMax <= 0)
		return -1;
	while (aLen > 0 && aText[0] == ' ') { aText++; aLen--; }
	while (aLen > 0 && aText[aLen - 1] == ' ') aLen--;
	if (!pw_addr_is_search(aText, aLen))
		{
		if (aLen >= aMax)
			return -1;
		for (i = 0; i < aLen; i++)
			aOut[i] = (char)aText[i];
		aOut[aLen] = 0;
		return aLen;
		}
	const char* base = pw_search_base(aEngine);
	for (i = 0; base[i]; i++)
		{
		if (n >= aMax - 1) return -1;
		aOut[n++] = base[i];
		}
	static const char hex[] = "0123456789ABCDEF";
	for (i = 0; i < aLen; i++)
		{
		unsigned u = pw_addr_cp1252(aText[i]);
		unsigned char b[3];
		int nb = 0, k;
		if (u < 0x80) b[nb++] = (unsigned char)u;
		else if (u < 0x800) { b[nb++] = (unsigned char)(0xc0 | (u >> 6)); b[nb++] = (unsigned char)(0x80 | (u & 0x3f)); }
		else { b[nb++] = (unsigned char)(0xe0 | (u >> 12)); b[nb++] = (unsigned char)(0x80 | ((u >> 6) & 0x3f)); b[nb++] = (unsigned char)(0x80 | (u & 0x3f)); }
		for (k = 0; k < nb; k++)
			{
			unsigned c = b[k];
			int plain = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
				|| c == '-' || c == '_' || c == '.' || c == '~';
			if (c == ' ')
				{
				if (n >= aMax - 1) return -1;
				aOut[n++] = '+';
				}
			else if (plain)
				{
				if (n >= aMax - 1) return -1;
				aOut[n++] = (char)c;
				}
			else
				{
				if (n >= aMax - 3) return -1;
				aOut[n++] = '%';
				aOut[n++] = hex[c >> 4];
				aOut[n++] = hex[c & 15];
				}
			}
		}
	aOut[n] = 0;
	return n;
	}

#endif
