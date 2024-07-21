static void sv_usage(void)
	{
	fprintf(stderr,"usage: ssltest [args ...]\n");
	fprintf(stderr,"\n");
	fprintf(stderr," -server_auth  - check server certificate\n");
	fprintf(stderr," -client_auth  - do client authentication\n");
	fprintf(stderr," -proxy        - allow proxy certificates\n");
	fprintf(stderr," -proxy_auth <val> - set proxy policy rights\n");
	fprintf(stderr," -proxy_cond <val> - experssion to test proxy policy rights\n");
	fprintf(stderr," -v            - more output\n");
	fprintf(stderr," -d            - debug output\n");
	fprintf(stderr," -reuse        - use session-id reuse\n");
	fprintf(stderr," -num <val>    - number of connections to perform\n");
	fprintf(stderr," -bytes <val>  - number of bytes to swap between client/server\n");
#ifndef OPENSSL_NO_DH
	fprintf(stderr," -dhe1024      - use 1024 bit key (safe prime) for DHE\n");
	fprintf(stderr," -dhe1024dsa   - use 1024 bit key (with 160-bit subprime) for DHE\n");
	fprintf(stderr," -no_dhe       - disable DHE\n");
#endif
#ifndef OPENSSL_NO_ECDH
	fprintf(stderr," -no_ecdhe     - disable ECDHE\n");
#endif
#ifndef OPENSSL_NO_PSK
	fprintf(stderr," -psk arg      - PSK in hex (without 0x)\n");
#endif
#ifndef OPENSSL_NO_SSL2
	fprintf(stderr," -ssl2         - use SSLv2\n");
#endif
#ifndef OPENSSL_NO_SSL3
	fprintf(stderr," -ssl3         - use SSLv3\n");
#endif
#ifndef OPENSSL_NO_TLS1
	fprintf(stderr," -tls1         - use TLSv1\n");
#endif
	fprintf(stderr," -CApath arg   - PEM format directory of CA's\n");
	fprintf(stderr," -CAfile arg   - PEM format file of CA's\n");
	fprintf(stderr," -cert arg     - Server certificate file\n");
	fprintf(stderr," -key arg      - Server key file (default: same as -cert)\n");
	fprintf(stderr," -c_cert arg   - Client certificate file\n");
	fprintf(stderr," -c_key arg    - Client key file (default: same as -c_cert)\n");
	fprintf(stderr," -cipher arg   - The cipher list\n");
	fprintf(stderr," -bio_pair     - Use BIO pairs\n");
	fprintf(stderr," -f            - Test even cases that can't work\n");
	fprintf(stderr," -time         - measure processor time used by client and server\n");
	fprintf(stderr," -zlib         - use zlib compression\n");
	fprintf(stderr," -rle          - use rle compression\n");
#ifndef OPENSSL_NO_ECDH
	fprintf(stderr," -named_curve arg  - Elliptic curve name to use for ephemeral ECDH keys.\n" \
	               "                 Use \"openssl ecparam -list_curves\" for all names\n"  \
	               "                 (default is sect163r2).\n");
#endif
	fprintf(stderr," -test_cipherlist - verifies the order of the ssl cipher lists\n");
#ifndef OPENSSL_NO_NPN
	fprintf(stderr," -npn_client - have client side offer NPN\n");
	fprintf(stderr," -npn_server - have server side offer NPN\n");
	fprintf(stderr," -npn_server_reject - have server reject NPN\n");
#endif
	}