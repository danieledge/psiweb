/* psi_store.c - the decoded pictures' purgeable store (PsiWeb 1.00, the
 * memory round's P2-07).
 *
 * A finished picture's bitmap (one byte a pixel, psi_drv.c) is moved out of
 * the engine's heap into a global chunk of its own, "PsiPurgeWebPics", made
 * by psion-core's ssh/psipurge.h convention (psi_region_*: epoc/psi_purge.cpp
 * on the Psion, emu/links_rt.c in the harness). Coda's memory hook
 * (PsiMemHook, mkimage --mem-hook) empties every such chunk that is not
 * pinned when a heap anywhere cannot grow: the pages go back to the Psion
 * at once, without the engine running.
 *
 * The engine pins the store whenever it runs and unpins it only while it
 * waits in psi_select (epoc/psi_os.c), so a picture is never read, drawn or
 * freed while the store may be emptied. When the engine next wakes and
 * finds the store emptied, psi_drv.c forgets every picture that was in it
 * (psi_store_purged): each is decoded again from Links' cache when it is
 * next drawn, as after PsiWeb's own eviction. The frame on the screen keeps
 * its pixels meanwhile: the worst case is a picture drawn again later.
 *
 * Blocks: an 8-byte head (size with head; where the owner keeps the
 * pointer, 0 when free), first fit, joined when freed. The owner's pointer
 * is moved when the store is compacted (psi_store_compact, at Trim and when
 * a page is done) and the chunk shrunk to what is used.
 *
 * Copyright (C) 2026 Dan Edge. GPL-2.0-or-later (as Links). */
#include <stddef.h>
#include <string.h>

unsigned char *psi_region_open(long max);	/* the region's base, 0: no store (PSI_PURGE off) */
int psi_region_pin(void);			/* 1 emptied since the last pin, 0 not, -1 not now */
int psi_region_commit(long bytes);		/* while pinned: 0, -1 no memory */
void psi_region_unpin(void);
void psi_region_wait(void);			/* a moment, before pinning again */
int pwb_depth(void);

#define BLK_HEAD 8
#define PAGE 4096L
#define SHRINK_SLACK (64L * 1024L)	/* committed beyond the top before it is given back */

typedef struct blk { unsigned long size; void **owner; } blk;

static unsigned char *base;		/* 0: not opened (or no store) */
static int tried, pinned;
static long max_bytes, top, committed, used;
static unsigned long purges;

static long round_page(long n) { return (n + PAGE - 1) & ~(PAGE - 1); }

static int open_store(void)
{
	if (base) return 1;
	if (tried) return 0;
	tried = 1;
	/* (address space only: the netBook's 20 MB engine has room for more) */
	max_bytes = pwb_depth() == 8 ? 8L * 1024 * 1024 : 4L * 1024 * 1024;
	base = psi_region_open(max_bytes);
	if (!base) return 0;
	while (psi_region_pin() < 0) psi_region_wait();
	pinned = 1;
	top = committed = used = 0;
	return 1;
}

int psi_store_has(const void *p)
{
	return base && (const unsigned char *)p >= base && (const unsigned char *)p < base + max_bytes;
}

static int commit_to(long want)
{
	long c = round_page(want);
	if (c == committed) return 0;
	if (psi_region_commit(c) != 0) return -1;
	committed = c;
	return 0;
}

void *psi_store_alloc(size_t n, void **owner)
{
	long need = (long)((n + BLK_HEAD + 7) & ~(size_t)7), at = 0;
	blk *b;
	if (!n || !open_store() || n > (size_t)max_bytes || !pinned) return 0;
	/* first fit among the free blocks */
	while (at < top) {
		b = (blk *)(base + at);
		if (!b->owner && (long)b->size >= need) {
			if ((long)b->size - need >= 64) {
				blk *rest = (blk *)(base + at + need);
				rest->size = b->size - need;
				rest->owner = 0;
				b->size = need;
			}
			b->owner = owner;
			used += b->size;
			return (unsigned char *)b + BLK_HEAD;
		}
		at += b->size;
	}
	/* else at the top */
	if (top + need > max_bytes || (top + need > committed && commit_to(top + need) != 0))
		return 0;
	b = (blk *)(base + top);
	b->size = need;
	b->owner = owner;
	top += need;
	used += need;
	return (unsigned char *)b + BLK_HEAD;
}

/* the top: the end of the last block in use */
static void lower_top(void)
{
	long at = 0, end = 0;
	while (at < top) {
		blk *b = (blk *)(base + at);
		at += b->size;
		if (b->owner) end = at;
	}
	top = end;
	if (committed - round_page(top) >= SHRINK_SLACK) commit_to(top);
}

void psi_store_free(void *p)
{
	blk *b, *n;
	if (!psi_store_has(p) || !pinned) return;
	b = (blk *)((unsigned char *)p - BLK_HEAD);
	if (!b->owner) return;
	b->owner = 0;
	used -= b->size;
	/* joined with the free blocks after it */
	while ((unsigned char *)b + b->size < base + top) {
		n = (blk *)((unsigned char *)b + b->size);
		if (n->owner) break;
		b->size += n->size;
	}
	if ((unsigned char *)b + b->size >= base + top) lower_top();
}

/* every block in use slid down to the bottom, the owners told, the chunk
   given back down to the top */
void psi_store_compact(void)
{
	long at = 0, to = 0;
	if (!base || !pinned) return;
	while (at < top) {
		blk *b = (blk *)(base + at);
		unsigned long sz = b->size;
		if (b->owner) {
			if (to != at) {
				void **owner = b->owner;
				memmove(base + to, b, sz);
				*owner = base + to + BLK_HEAD;
			}
			to += (long)sz;
		}
		at += (long)sz;
	}
	top = to;
	used = to;
	commit_to(top);
}

/* the engine waits (psi_select): the store may be emptied meanwhile */
void psi_store_idle(void)
{
	if (base && pinned) {
		pinned = 0;
		psi_region_unpin();
	}
}

/* the engine runs again: pinned; 1 if it was emptied meanwhile (every
   picture in it is gone: psi_drv.c forgets them before anything else) */
int psi_store_busy(void)
{
	int r;
	if (!base || pinned) return 0;
	while ((r = psi_region_pin()) < 0) psi_region_wait();
	pinned = 1;
	if (r == 1) {
		top = committed = used = 0;
		purges++;
		return 1;
	}
	return 0;
}

long psi_store_used(void) { return used; }
long psi_store_committed(void) { return committed; }
unsigned long psi_store_purges(void) { return purges; }
