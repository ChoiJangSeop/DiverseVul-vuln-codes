int ssl_cipher_list_to_bytes(SSL *s,STACK_OF(SSL_CIPHER) *sk,unsigned char *p,
			     int (*put_cb)(const SSL_CIPHER *, unsigned char *))
	{
	int i,j=0;
	SSL_CIPHER *c;
	unsigned char *q;
	int no_scsv = s->renegotiate;
	/* Set disabled masks for this session */
	ssl_set_client_disabled(s);

	if (sk == NULL) return(0);
	q=p;

	for (i=0; i<sk_SSL_CIPHER_num(sk); i++)
		{
		c=sk_SSL_CIPHER_value(sk,i);
		/* Skip disabled ciphers */
		if (ssl_cipher_disabled(s, c, SSL_SECOP_CIPHER_SUPPORTED))
			continue;
#ifdef OPENSSL_SSL_DEBUG_BROKEN_PROTOCOL
		if (c->id == SSL3_CK_SCSV)
			{
			if (no_scsv)
				continue;
			else
				no_scsv = 1;
			}
#endif
		j = put_cb ? put_cb(c,p) : ssl_put_cipher_by_char(s,c,p);
		p+=j;
		}
	/* If p == q, no ciphers and caller indicates an error. Otherwise
	 * add SCSV if not renegotiating.
	 */
	if (p != q && !no_scsv)
		{
		static SSL_CIPHER scsv =
			{
			0, NULL, SSL3_CK_SCSV, 0, 0, 0, 0, 0, 0, 0, 0, 0
			};
		j = put_cb ? put_cb(&scsv,p) : ssl_put_cipher_by_char(s,&scsv,p);
		p+=j;
#ifdef OPENSSL_RI_DEBUG
		fprintf(stderr, "SCSV sent by client\n");
#endif
		}

	return(p-q);
	}