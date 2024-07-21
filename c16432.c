static void sc_usage(void)
	{
	BIO_printf(bio_err,"usage: s_client args\n");
	BIO_printf(bio_err,"\n");
	BIO_printf(bio_err," -host host     - use -connect instead\n");
	BIO_printf(bio_err," -port port     - use -connect instead\n");
	BIO_printf(bio_err," -connect host:port - connect over TCP/IP (default is %s:%s)\n",SSL_HOST_NAME,PORT_STR);
	BIO_printf(bio_err," -unix path    - connect over unix domain sockets\n");
	BIO_printf(bio_err," -verify arg   - turn on peer certificate verification\n");
	BIO_printf(bio_err," -verify_return_error - return verification errors\n");
	BIO_printf(bio_err," -cert arg     - certificate file to use, PEM format assumed\n");
	BIO_printf(bio_err," -certform arg - certificate format (PEM or DER) PEM default\n");
	BIO_printf(bio_err," -key arg      - Private key file to use, in cert file if\n");
	BIO_printf(bio_err,"                 not specified but cert file is.\n");
	BIO_printf(bio_err," -keyform arg  - key format (PEM or DER) PEM default\n");
	BIO_printf(bio_err," -pass arg     - private key file pass phrase source\n");
	BIO_printf(bio_err," -CApath arg   - PEM format directory of CA's\n");
	BIO_printf(bio_err," -CAfile arg   - PEM format file of CA's\n");
	BIO_printf(bio_err," -trusted_first - Use local CA's first when building trust chain\n");
	BIO_printf(bio_err," -reconnect    - Drop and re-make the connection with the same Session-ID\n");
	BIO_printf(bio_err," -pause        - sleep(1) after each read(2) and write(2) system call\n");
	BIO_printf(bio_err," -prexit       - print session information even on connection failure\n");
	BIO_printf(bio_err," -showcerts    - show all certificates in the chain\n");
	BIO_printf(bio_err," -debug        - extra output\n");
#ifdef WATT32
	BIO_printf(bio_err," -wdebug       - WATT-32 tcp debugging\n");
#endif
	BIO_printf(bio_err," -msg          - Show protocol messages\n");
	BIO_printf(bio_err," -nbio_test    - more ssl protocol testing\n");
	BIO_printf(bio_err," -state        - print the 'ssl' states\n");
#ifdef FIONBIO
	BIO_printf(bio_err," -nbio         - Run with non-blocking IO\n");
#endif
	BIO_printf(bio_err," -crlf         - convert LF from terminal into CRLF\n");
	BIO_printf(bio_err," -quiet        - no s_client output\n");
	BIO_printf(bio_err," -ign_eof      - ignore input eof (default when -quiet)\n");
	BIO_printf(bio_err," -no_ign_eof   - don't ignore input eof\n");
#ifndef OPENSSL_NO_PSK
	BIO_printf(bio_err," -psk_identity arg - PSK identity\n");
	BIO_printf(bio_err," -psk arg      - PSK in hex (without 0x)\n");
# ifndef OPENSSL_NO_JPAKE
	BIO_printf(bio_err," -jpake arg    - JPAKE secret to use\n");
# endif
#endif
#ifndef OPENSSL_NO_SRP
	BIO_printf(bio_err," -srpuser user     - SRP authentification for 'user'\n");
	BIO_printf(bio_err," -srppass arg      - password for 'user'\n");
	BIO_printf(bio_err," -srp_lateuser     - SRP username into second ClientHello message\n");
	BIO_printf(bio_err," -srp_moregroups   - Tolerate other than the known g N values.\n");
	BIO_printf(bio_err," -srp_strength int - minimal mength in bits for N (default %d).\n",SRP_MINIMAL_N);
#endif
	BIO_printf(bio_err," -ssl2         - just use SSLv2\n");
	BIO_printf(bio_err," -ssl3         - just use SSLv3\n");
	BIO_printf(bio_err," -tls1_2       - just use TLSv1.2\n");
	BIO_printf(bio_err," -tls1_1       - just use TLSv1.1\n");
	BIO_printf(bio_err," -tls1         - just use TLSv1\n");
	BIO_printf(bio_err," -dtls1        - just use DTLSv1\n");    
	BIO_printf(bio_err," -mtu          - set the link layer MTU\n");
	BIO_printf(bio_err," -no_tls1_2/-no_tls1_1/-no_tls1/-no_ssl3/-no_ssl2 - turn off that protocol\n");
	BIO_printf(bio_err," -bugs         - Switch on all SSL implementation bug workarounds\n");
	BIO_printf(bio_err," -serverpref   - Use server's cipher preferences (only SSLv2)\n");
	BIO_printf(bio_err," -cipher       - preferred cipher to use, use the 'openssl ciphers'\n");
	BIO_printf(bio_err,"                 command to see what is available\n");
	BIO_printf(bio_err," -starttls prot - use the STARTTLS command before starting TLS\n");
	BIO_printf(bio_err,"                 for those protocols that support it, where\n");
	BIO_printf(bio_err,"                 'prot' defines which one to assume.  Currently,\n");
	BIO_printf(bio_err,"                 only \"smtp\", \"pop3\", \"imap\", \"ftp\" and \"xmpp\"\n");
	BIO_printf(bio_err,"                 are supported.\n");
	BIO_printf(bio_err," -xmpphost host - When used with \"-starttls xmpp\" specifies the virtual host.\n");
#ifndef OPENSSL_NO_ENGINE
	BIO_printf(bio_err," -engine id    - Initialise and use the specified engine\n");
#endif
	BIO_printf(bio_err," -rand file%cfile%c...\n", LIST_SEPARATOR_CHAR, LIST_SEPARATOR_CHAR);
	BIO_printf(bio_err," -sess_out arg - file to write SSL session to\n");
	BIO_printf(bio_err," -sess_in arg  - file to read SSL session from\n");
#ifndef OPENSSL_NO_TLSEXT
	BIO_printf(bio_err," -servername host  - Set TLS extension servername in ClientHello\n");
	BIO_printf(bio_err," -tlsextdebug      - hex dump of all TLS extensions received\n");
	BIO_printf(bio_err," -status           - request certificate status from server\n");
	BIO_printf(bio_err," -no_ticket        - disable use of RFC4507bis session tickets\n");
	BIO_printf(bio_err," -serverinfo types - send empty ClientHello extensions (comma-separated numbers)\n");
# ifndef OPENSSL_NO_NEXTPROTONEG
	BIO_printf(bio_err," -nextprotoneg arg - enable NPN extension, considering named protocols supported (comma-separated list)\n");
# endif
	BIO_printf(bio_err," -alpn arg         - enable ALPN extension, considering named protocols supported (comma-separated list)\n");
#endif
	BIO_printf(bio_err," -legacy_renegotiation - enable use of legacy renegotiation (dangerous)\n");
	BIO_printf(bio_err," -use_srtp profiles - Offer SRTP key management with a colon-separated profile list\n");
 	BIO_printf(bio_err," -keymatexport label   - Export keying material using label\n");
 	BIO_printf(bio_err," -keymatexportlen len  - Export len bytes of keying material (default 20)\n");
	}