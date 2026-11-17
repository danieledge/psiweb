/* pwcert.h - PsiWeb's side of the certificate check (see pwcert.c) */
#ifndef PWCERT_H
#define PWCERT_H

/* before pwn_connect with TLS: the name the certificate must carry */
void pw_cert_host(const char *host, int port);
/* after a failed TLS connection to host: if the certificate was the reason,
   why becomes the message for the user ("Certificate not trusted - host")
   and 1 is returned; the check's own reason goes to PsiWeb.log. (Also a
   handshake the server turned down: "Secure connection refused - host".) */
int  pw_cert_why(const char *host, char *why, int whymax);

#endif
