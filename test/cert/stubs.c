/* stubs.c - what psiweb.exe gives engine/pwcert.c, for the host tests */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
void pwb_set_status(const char *s) { fprintf(stderr, "[status] %s\n", s); }
void pw_log(const char *s) { fprintf(stderr, "[log] %s\n", s); }
long pw_utc(void) { const char *n = getenv("PW_NOW"); return n ? atol(n) : (long)time(NULL); }
