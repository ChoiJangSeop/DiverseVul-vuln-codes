static void print_stuff(BIO *bio, SSL *s, int full)
	{
	X509 *peer=NULL;
	char *p;
	static const char *space="                ";
	char buf[BUFSIZ];
	STACK_OF(X509) *sk;
	STACK_OF(X509_NAME) *sk2;
	const SSL_CIPHER *c;
	X509_NAME *xn;
	int j,i;
#ifndef OPENSSL_NO_COMP
	const COMP_METHOD *comp, *expansion;
#endif
	unsigned char *exportedkeymat;

	if (full)
		{
		int got_a_chain = 0;

		sk=SSL_get_peer_cert_chain(s);
		if (sk != NULL)
			{
			got_a_chain = 1; /* we don't have it for SSL2 (yet) */

			BIO_printf(bio,"---\nCertificate chain\n");
			for (i=0; i<sk_X509_num(sk); i++)
				{
				X509_NAME_oneline(X509_get_subject_name(
					sk_X509_value(sk,i)),buf,sizeof buf);
				BIO_printf(bio,"%2d s:%s\n",i,buf);
				X509_NAME_oneline(X509_get_issuer_name(
					sk_X509_value(sk,i)),buf,sizeof buf);
				BIO_printf(bio,"   i:%s\n",buf);
				if (c_showcerts)
					PEM_write_bio_X509(bio,sk_X509_value(sk,i));
				}
			}

		BIO_printf(bio,"---\n");
		peer=SSL_get_peer_certificate(s);
		if (peer != NULL)
			{
			BIO_printf(bio,"Server certificate\n");
			if (!(c_showcerts && got_a_chain)) /* Redundant if we showed the whole chain */
				PEM_write_bio_X509(bio,peer);
			X509_NAME_oneline(X509_get_subject_name(peer),
				buf,sizeof buf);
			BIO_printf(bio,"subject=%s\n",buf);
			X509_NAME_oneline(X509_get_issuer_name(peer),
				buf,sizeof buf);
			BIO_printf(bio,"issuer=%s\n",buf);
			}
		else
			BIO_printf(bio,"no peer certificate available\n");

		sk2=SSL_get_client_CA_list(s);
		if ((sk2 != NULL) && (sk_X509_NAME_num(sk2) > 0))
			{
			BIO_printf(bio,"---\nAcceptable client certificate CA names\n");
			for (i=0; i<sk_X509_NAME_num(sk2); i++)
				{
				xn=sk_X509_NAME_value(sk2,i);
				X509_NAME_oneline(xn,buf,sizeof(buf));
				BIO_write(bio,buf,strlen(buf));
				BIO_write(bio,"\n",1);
				}
			}
		else
			{
			BIO_printf(bio,"---\nNo client certificate CA names sent\n");
			}
		p=SSL_get_shared_ciphers(s,buf,sizeof buf);
		if (p != NULL)
			{
			/* This works only for SSL 2.  In later protocol
			 * versions, the client does not know what other
			 * ciphers (in addition to the one to be used
			 * in the current connection) the server supports. */

			BIO_printf(bio,"---\nCiphers common between both SSL endpoints:\n");
			j=i=0;
			while (*p)
				{
				if (*p == ':')
					{
					BIO_write(bio,space,15-j%25);
					i++;
					j=0;
					BIO_write(bio,((i%3)?" ":"\n"),1);
					}
				else
					{
					BIO_write(bio,p,1);
					j++;
					}
				p++;
				}
			BIO_write(bio,"\n",1);
			}

		ssl_print_sigalgs(bio, s);

		BIO_printf(bio,"---\nSSL handshake has read %ld bytes and written %ld bytes\n",
			BIO_number_read(SSL_get_rbio(s)),
			BIO_number_written(SSL_get_wbio(s)));
		}
	BIO_printf(bio,(SSL_cache_hit(s)?"---\nReused, ":"---\nNew, "));
	c=SSL_get_current_cipher(s);
	BIO_printf(bio,"%s, Cipher is %s\n",
		SSL_CIPHER_get_version(c),
		SSL_CIPHER_get_name(c));
	if (peer != NULL) {
		EVP_PKEY *pktmp;
		pktmp = X509_get_pubkey(peer);
		BIO_printf(bio,"Server public key is %d bit\n",
							 EVP_PKEY_bits(pktmp));
		EVP_PKEY_free(pktmp);
	}
	BIO_printf(bio, "Secure Renegotiation IS%s supported\n",
			SSL_get_secure_renegotiation_support(s) ? "" : " NOT");
#ifndef OPENSSL_NO_COMP
	comp=SSL_get_current_compression(s);
	expansion=SSL_get_current_expansion(s);
	BIO_printf(bio,"Compression: %s\n",
		comp ? SSL_COMP_get_name(comp) : "NONE");
	BIO_printf(bio,"Expansion: %s\n",
		expansion ? SSL_COMP_get_name(expansion) : "NONE");
#endif
 
#ifdef SSL_DEBUG
	{
	/* Print out local port of connection: useful for debugging */
	int sock;
	struct sockaddr_in ladd;
	socklen_t ladd_size = sizeof(ladd);
	sock = SSL_get_fd(s);
	getsockname(sock, (struct sockaddr *)&ladd, &ladd_size);
	BIO_printf(bio_c_out, "LOCAL PORT is %u\n", ntohs(ladd.sin_port));
	}
#endif

#if !defined(OPENSSL_NO_TLSEXT) && !defined(OPENSSL_NO_NEXTPROTONEG)
	if (next_proto.status != -1) {
		const unsigned char *proto;
		unsigned int proto_len;
		SSL_get0_next_proto_negotiated(s, &proto, &proto_len);
		BIO_printf(bio, "Next protocol: (%d) ", next_proto.status);
		BIO_write(bio, proto, proto_len);
		BIO_write(bio, "\n", 1);
	}
#endif

 	{
 	SRTP_PROTECTION_PROFILE *srtp_profile=SSL_get_selected_srtp_profile(s);
 
	if(srtp_profile)
		BIO_printf(bio,"SRTP Extension negotiated, profile=%s\n",
			   srtp_profile->name);
	}
 
	SSL_SESSION_print(bio,SSL_get_session(s));
	if (keymatexportlabel != NULL)
		{
		BIO_printf(bio, "Keying material exporter:\n");
		BIO_printf(bio, "    Label: '%s'\n", keymatexportlabel);
		BIO_printf(bio, "    Length: %i bytes\n", keymatexportlen);
		exportedkeymat = OPENSSL_malloc(keymatexportlen);
		if (exportedkeymat != NULL)
			{
			if (!SSL_export_keying_material(s, exportedkeymat,
						        keymatexportlen,
						        keymatexportlabel,
						        strlen(keymatexportlabel),
						        NULL, 0, 0))
				{
				BIO_printf(bio, "    Error\n");
				}
			else
				{
				BIO_printf(bio, "    Keying material: ");
				for (i=0; i<keymatexportlen; i++)
					BIO_printf(bio, "%02X",
						   exportedkeymat[i]);
				BIO_printf(bio, "\n");
				}
			OPENSSL_free(exportedkeymat);
			}
		}
	BIO_printf(bio,"---\n");
	if (peer != NULL)
		X509_free(peer);
	/* flush, or debugging output gets mixed with http response */
	(void)BIO_flush(bio);
	}