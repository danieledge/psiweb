// psi_purge.cpp - the region under psi_store.c on the Psion: a global chunk
// "PsiPurgeWebPics" with psion-core's purgeable head (ssh/psipurge.h: the
// magic, the pin word, the purged flag), so that PsiPurgeAll (Coda's memory
// hook, PsiMemHook) can empty it from any process while it is not pinned.
// Built with PSI_PURGE=1 (links/epoc.mk); with 0 there is no store and the
// pictures stay in the engine's heap, as before.
//
// The pin word follows psipurge.h: User::LockedInc returns the value before,
// and -1 means the watcher is emptying the chunk now. The owner's own Adjust
// (to grow or shrink what it uses) is only made while pinned.
//
// Copyright (C) 2026 Dan Edge. GPL-2.0-or-later (as Links).
#include <e32std.h>
#include "psipurge.h"

_LIT(KPwPurgeName, "PsiPurgeWebPics");

static RChunk gChunk;
static TPsiPurgeHead* gHead;

extern "C" unsigned char* psi_region_open(long aMax)
	{
#if PSI_PURGE
	if (gHead)
		return gChunk.Base() + KPsiPurgeHead;
	if (aMax <= 0 || gChunk.CreateGlobal(KPwPurgeName, KPsiPurgeHead, KPsiPurgeHead + aMax) != KErrNone)
		return 0;
	gHead = (TPsiPurgeHead*)gChunk.Base();
	Mem::FillZ(gHead, sizeof(*gHead));
	gHead->iMagic = KPsiPurgeMagic;
	return gChunk.Base() + KPsiPurgeHead;
#else
	(void)aMax;
	return 0;
#endif
	}

extern "C" int psi_region_pin(void)
	{
	if (!gHead)
		return 0;
	if (User::LockedInc(gHead->iPin) < 0)
		{
		User::LockedDec(gHead->iPin);        // the watcher has it: in a moment
		return -1;
		}
	if (gHead->iPurged)
		{
		gHead->iPurged = 0;
		return 1;
		}
	return 0;
	}

extern "C" int psi_region_commit(long aBytes)
	{
	if (!gHead)
		return -1;
	return gChunk.Adjust(KPsiPurgeHead + aBytes) == KErrNone ? 0 : -1;
	}

extern "C" void psi_region_unpin(void)
	{
	if (gHead)
		User::LockedDec(gHead->iPin);
	}

extern "C" void psi_region_wait(void)
	{
	User::After(2000);
	}
