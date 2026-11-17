/* certhost.c - PsiWeb's certificate check on a PC: psion-core's certcheck.c
 * with engine/pwcert.c, as psiweb.exe builds them, on a saved chain.
 *
 *   certhost CHAIN.msg HOST         the chain (a TLS 1.3 Certificate message
 *                                   body, from mkmsg.py) checked for HOST
 *   certhost --alert HOST           what the user is told when the server
 *                                   turns the handshake down
 *
 * Prints "trusted", or the message PsiWeb shows and the check's reason.
 * PW_NOW (seconds since 1970) sets the clock. certs.txt goes in the folder
 * it runs in. Run by run.sh. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pwcert.h"

const char *tlsv_certificate(const unsigned char *msg, int len);
const char *tlsv_problem(void);


int main(int argc, char **argv)
{
	static unsigned char buf[20000];
	char why[80];
	const char *e;
	FILE *f;
	int n;
	if (argc == 3 && !strcmp(argv[1], "--alert")) {
		pw_cert_host(argv[2], 443);
		strcpy(why, "server refused the connection (TLS alert)");
		pw_cert_why(argv[2], why, sizeof(why));
		printf("%s\n", why);
		return 1;
	}
	if (argc != 3 || !(f = fopen(argv[1], "rb"))) {
		fprintf(stderr, "usage: certhost CHAIN.msg HOST | --alert HOST\n");
		return 2;
	}
	n = (int)fread(buf, 1, sizeof(buf), f);
	fclose(f);
	pw_cert_host(argv[2], 443);
	e = tlsv_certificate(buf, n);
	if (!e) {
		printf("trusted\n");
		return 0;
	}
	snprintf(why, sizeof(why), "%s", e);
	if (!pw_cert_why(argv[2], why, sizeof(why)))
		printf("not a certificate failure: %s\n", why);
	else
		printf("%s | %s\n", why, tlsv_problem());
	return 1;
}
