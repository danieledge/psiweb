// psi_simp.cpp - the C entry point to the page simplifier (htmlsimp.cpp,
// the PsiConnect Atom's, copied unchanged) for psiweb.exe (0.86). Built
// without exceptions or RTTI; it makes no heap allocations of its own: the
// simplifier's state (about 5 KB) is a static block, set up with
// placement new for each page.
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "htmlsimp.h"

// (the compiler's own size type: ESTLIB's size_t is not the one operator new takes)
inline void* operator new(unsigned int, void* aPlace) { return aPlace; }
// (the sink's virtual destructor names these; nothing is ever deleted)
void operator delete(void* aPtr) { free(aPtr); }
extern "C" void __cxa_pure_virtual() {}
extern "C" void __pure_virtual() {}

extern "C" {
#include "psi_simp.h"
}

namespace {

class CSink : public am::HtmlSink
	{
public:
	CSink(psi_simp_put aPut, void* aCtx) : iPut(aPut), iCtx(aCtx), iInTag(0), iText(0) {}
	void Put(const char* aData, size_t aLen)
		{
		for (size_t i = 0; i < aLen; i++)
			{
			char c = aData[i];
			if (c == '<') iInTag = 1;
			else if (c == '>') iInTag = 0;
			else if (!iInTag && c != ' ' && c != '\n' && c != '\r' && c != '\t') iText++;
			}
		iPut(iCtx, aData, aLen);
		}
	unsigned long Text() const { return iText; }
private:
	psi_simp_put iPut;
	void* iCtx;
	int iInTag;
	unsigned long iText;
	};

// the simplifier's state: a static block, set up afresh for each page
union UStore
	{
	char iBytes[sizeof(am::HtmlSimplifier)];
	double iAlign;
	long iAlign2;
	};
UStore gStore;

int Lower(int c) { return c >= 'A' && c <= 'Z' ? c + 'a' - 'A' : c; }

}

static am::HtmlSimplifier* gSimp;     // the page being simplified, between calls
static CSink* gSink;
union USinkStore { char iBytes[sizeof(CSink)]; double iAlign; long iAlign2; };
static USinkStore gSinkStore;
static int gScripts;
static int gMatch;                    // "<script" matched so far, across pieces

static void CountScripts(const unsigned char* aIn, unsigned long aLen)
{
	static const char kTag[] = "<script";
	unsigned long i = 0;
	while (i < aLen && gScripts < 1000)
		{
		if (gMatch == 0)
			{
			const unsigned char* p = (const unsigned char*)memchr(aIn + i, '<', aLen - i);
			if (!p)
				break;
			i = (unsigned long)(p - aIn) + 1;
			gMatch = 1;
			continue;
			}
		if ((aIn[i] | 0x20) == kTag[gMatch])
			{
			i++;
			if (++gMatch == 7) { gScripts++; gMatch = 0; }
			}
		else
			gMatch = 0;                      // (this byte is looked at again: it may be a '<')
		}
}

extern "C" void psi_simp_begin(int aMode, psi_simp_put aPut, void* aCtx)
{
	gSink = new(&gSinkStore) CSink(aPut, aCtx);
	gSimp = new(&gStore) am::HtmlSimplifier();
	gSimp->Begin(gSink, aMode, 0);
	gScripts = 0;
	gMatch = 0;
}

extern "C" void psi_simp_feed(const unsigned char* aIn, unsigned long aLen)
{
	if (!gSimp) return;
	CountScripts(aIn, aLen);
	gSimp->Feed(aIn, aLen);
}

extern "C" void psi_simp_end(unsigned long* aText, int* aScripts)
{
	if (!gSimp) return;
	gSimp->End();
	if (aText) *aText = gSink->Text();
	if (aScripts) *aScripts = gScripts;
	gSimp = 0;
}

extern "C" unsigned long psi_simp_run(const unsigned char* aIn, unsigned long aLen, int aMode,
	psi_simp_put aPut, void* aCtx, unsigned long* aText, int* aScripts)
{
	psi_simp_begin(aMode, aPut, aCtx);
	psi_simp_feed(aIn, aLen);
	unsigned long out = gSimp->Out();
	psi_simp_end(aText, aScripts);
	return out;
}
