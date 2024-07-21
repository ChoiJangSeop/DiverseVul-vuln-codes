static void sv_usage(void)
	{
	BIO_printf(bio_err,"usage: s_server [args ...]\n");
	BIO_printf(bio_err,"\n");
	BIO_printf(bio_err," -accept arg   - port to accept on (default is %d)\n",PORT);
	BIO_printf(bio_err," -context arg  - set session ID context\n");
	BIO_printf(bio_err," -verify arg   - turn on peer certificate verification\n");
	BIO_printf(bio_err," -Verify arg   - turn on peer certificate verification, must have a cert.\n");
	BIO_printf(bio_err," -cert arg     - certificate file to use\n");
	BIO_printf(bio_err,"                 (default is %s)\n",TEST_CERT);
	BIO_printf(bio_err," -crl_check    - check the peer certificate has not been revoked by its CA.\n" \
	                   "                 The CRL(s) are appended to the certificate file\n");
	BIO_printf(bio_err," -crl_check_all - check the peer certificate has not been revoked by its CA\n" \
	                   "                 or any other CRL in the CA chain. CRL(s) are appened to the\n" \
	                   "                 the certificate file.\n");
	BIO_printf(bio_err," -certform arg - certificate format (PEM or DER) PEM default\n");
	BIO_printf(bio_err," -key arg      - Private Key file to use, in cert file if\n");
	BIO_printf(bio_err,"                 not specified (default is %s)\n",TEST_CERT);
	BIO_printf(bio_err," -keyform arg  - key format (PEM, DER or ENGINE) PEM default\n");
	BIO_printf(bio_err," -pass arg     - private key file pass phrase source\n");
	BIO_printf(bio_err," -dcert arg    - second certificate file to use (usually for DSA)\n");
	BIO_printf(bio_err," -dcertform x  - second certificate format (PEM or DER) PEM default\n");
	BIO_printf(bio_err," -dkey arg     - second private key file to use (usually for DSA)\n");
	BIO_printf(bio_err," -dkeyform arg - second key format (PEM, DER or ENGINE) PEM default\n");
	BIO_printf(bio_err," -dpass arg    - second private key file pass phrase source\n");
	BIO_printf(bio_err," -dhparam arg  - DH parameter file to use, in cert file if not specified\n");
	BIO_printf(bio_err,"                 or a default set of parameters is used\n");
#ifndef OPENSSL_NO_ECDH
	BIO_printf(bio_err," -named_curve arg  - Elliptic curve name to use for ephemeral ECDH keys.\n" \
	                   "                 Use \"openssl ecparam -list_curves\" for all names\n" \
	                   "                 (default is nistp256).\n");
#endif
#ifdef FIONBIO
	BIO_printf(bio_err," -nbio         - Run with non-blocking IO\n");
#endif
	BIO_printf(bio_err," -nbio_test    - test with the non-blocking test bio\n");
	BIO_printf(bio_err," -crlf         - convert LF from terminal into CRLF\n");
	BIO_printf(bio_err," -debug        - Print more output\n");
	BIO_printf(bio_err," -msg          - Show protocol messages\n");
	BIO_printf(bio_err," -state        - Print the SSL states\n");
	BIO_printf(bio_err," -CApath arg   - PEM format directory of CA's\n");
	BIO_printf(bio_err," -CAfile arg   - PEM format file of CA's\n");
	BIO_printf(bio_err," -nocert       - Don't use any certificates (Anon-DH)\n");
	BIO_printf(bio_err," -cipher arg   - play with 'openssl ciphers' to see what goes here\n");
	BIO_printf(bio_err," -serverpref   - Use server's cipher preferences\n");
	BIO_printf(bio_err," -quiet        - No server output\n");
	BIO_printf(bio_err," -no_tmp_rsa   - Do not generate a tmp RSA key\n");
#ifndef OPENSSL_NO_PSK
	BIO_printf(bio_err," -psk_hint arg - PSK identity hint to use\n");
	BIO_printf(bio_err," -psk arg      - PSK in hex (without 0x)\n");
# ifndef OPENSSL_NO_JPAKE
	BIO_printf(bio_err," -jpake arg    - JPAKE secret to use\n");
# endif
#endif
	BIO_printf(bio_err," -ssl2         - Just talk SSLv2\n");
	BIO_printf(bio_err," -ssl3         - Just talk SSLv3\n");
	BIO_printf(bio_err," -tls1_1       - Just talk TLSv1_1\n");
	BIO_printf(bio_err," -tls1         - Just talk TLSv1\n");
	BIO_printf(bio_err," -dtls1        - Just talk DTLSv1\n");
	BIO_printf(bio_err," -timeout      - Enable timeouts\n");
	BIO_printf(bio_err," -mtu          - Set link layer MTU\n");
	BIO_printf(bio_err," -chain        - Read a certificate chain\n");
	BIO_printf(bio_err," -no_ssl2      - Just disable SSLv2\n");
	BIO_printf(bio_err," -no_ssl3      - Just disable SSLv3\n");
	BIO_printf(bio_err," -no_tls1      - Just disable TLSv1\n");
	BIO_printf(bio_err," -no_tls1_1    - Just disable TLSv1.1\n");
#ifndef OPENSSL_NO_DH
	BIO_printf(bio_err," -no_dhe       - Disable ephemeral DH\n");
#endif
#ifndef OPENSSL_NO_ECDH
	BIO_printf(bio_err," -no_ecdhe     - Disable ephemeral ECDH\n");
#endif
	BIO_printf(bio_err," -bugs         - Turn on SSL bug compatibility\n");
	BIO_printf(bio_err," -www          - Respond to a 'GET /' with a status page\n");
	BIO_printf(bio_err," -WWW          - Respond to a 'GET /<path> HTTP/1.0' with file ./<path>\n");
	BIO_printf(bio_err," -HTTP         - Respond to a 'GET /<path> HTTP/1.0' with file ./<path>\n");
        BIO_printf(bio_err,"                 with the assumption it contains a complete HTTP response.\n");
#ifndef OPENSSL_NO_ENGINE
	BIO_printf(bio_err," -engine id    - Initialise and use the specified engine\n");
#endif
	BIO_printf(bio_err," -id_prefix arg - Generate SSL/TLS session IDs prefixed by 'arg'\n");
	BIO_printf(bio_err," -rand file%cfile%c...\n", LIST_SEPARATOR_CHAR, LIST_SEPARATOR_CHAR);
#ifndef OPENSSL_NO_TLSEXT
	BIO_printf(bio_err," -servername host - servername for HostName TLS extension\n");
	BIO_printf(bio_err," -servername_fatal - on mismatch send fatal alert (default warning alert)\n");
	BIO_printf(bio_err," -cert2 arg    - certificate file to use for servername\n");
	BIO_printf(bio_err,"                 (default is %s)\n",TEST_CERT2);
	BIO_printf(bio_err," -key2 arg     - Private Key file to use for servername, in cert file if\n");
	BIO_printf(bio_err,"                 not specified (default is %s)\n",TEST_CERT2);
	BIO_printf(bio_err," -tlsextdebug  - hex dump of all TLS extensions received\n");
	BIO_printf(bio_err," -no_ticket    - disable use of RFC4507bis session tickets\n");
	BIO_printf(bio_err," -legacy_renegotiation - enable use of legacy renegotiation (dangerous)\n");
#endif
	}