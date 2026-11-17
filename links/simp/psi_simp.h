/* psi_simp.h - the page simplifier from C (psi_simp.cpp, 0.86) */
#ifndef PSI_SIMP_H
#define PSI_SIMP_H

/* htmlsimp.h's modes */
#define PSI_SIMP_LIGHTER 1          /* EKeepPictures */
#define PSI_SIMP_TEXT    2          /* ETextOnly */
#define PSI_SIMP_READER  3          /* EReader */

typedef void (*psi_simp_put)(void *ctx, const char *data, unsigned long len);

/* Runs the whole page aIn through the simplifier in aMode, handing the
 * output to aPut as it comes. *aText: the characters of text it kept (not
 * tags, not white space); *aScripts: how many <script> elements the page
 * had (at most 1000). Returns the output's length. */
unsigned long psi_simp_run(const unsigned char *aIn, unsigned long aLen, int aMode,
	psi_simp_put aPut, void *aCtx, unsigned long *aText, int *aScripts);
/* The same in pieces, for a page that is still coming (0.86): begin, feed
 * each new piece, end when the page is whole. One page at a time. */
void psi_simp_begin(int aMode, psi_simp_put aPut, void *aCtx);
void psi_simp_feed(const unsigned char *aIn, unsigned long aLen);
void psi_simp_end(unsigned long *aText, int *aScripts);
#endif
