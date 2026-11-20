/* pwhtml.h - an address made safe for an HTML attribute (0.98), for the
 * start page's links (pwstart.cpp). Plain C with no EPOC calls, so
 * test/start/htmltest.c checks it on a PC.
 *
 * An address goes into a link whole or not at all: a cut one, or one with
 * "..." on the end, would open another page.
 */
#ifndef PWHTML_H
#define PWHTML_H

/* aText (aLen bytes of the Psion's 8-bit text) with & < > " as entities
 * and control characters as spaces, written to aOut if all of it fits in
 * aRoom bytes: the number of bytes written, or -1 (nothing written). */
static inline int pw_attr_esc(unsigned char* aOut, int aRoom, const unsigned char* aText, int aLen)
	{
	int i, n = 0;
	for (i = 0; i < aLen; i++)
		{
		unsigned c = aText[i];
		n += c == '&' ? 5 : c == '<' || c == '>' ? 4 : c == '"' ? 6 : 1;
		}
	if (aRoom < 0 || n > aRoom)
		return -1;
	n = 0;
	for (i = 0; i < aLen; i++)
		{
		unsigned c = aText[i];
		const char* e = c == '&' ? "&amp;" : c == '<' ? "&lt;" : c == '>' ? "&gt;" : c == '"' ? "&quot;" : 0;
		if (e)
			while (*e)
				aOut[n++] = (unsigned char)*e++;
		else
			aOut[n++] = (unsigned char)(c < 0x20 ? ' ' : c);
		}
	return n;
	}

#endif
