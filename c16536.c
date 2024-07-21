static int www_body(char *hostname, int s, unsigned char *context)
	{
	char *buf=NULL;
	int ret=1;
	int i,j,k,dot;
	SSL *con;
	const SSL_CIPHER *c;
	BIO *io,*ssl_bio,*sbio;
#ifndef OPENSSL_NO_KRB5
	KSSL_CTX *kctx;
#endif

	buf=OPENSSL_malloc(bufsize);
	if (buf == NULL) return(0);
	io=BIO_new(BIO_f_buffer());
	ssl_bio=BIO_new(BIO_f_ssl());
	if ((io == NULL) || (ssl_bio == NULL)) goto err;

#ifdef FIONBIO	
	if (s_nbio)
		{
		unsigned long sl=1;

		if (!s_quiet)
			BIO_printf(bio_err,"turning on non blocking io\n");
		if (BIO_socket_ioctl(s,FIONBIO,&sl) < 0)
			ERR_print_errors(bio_err);
		}
#endif

	/* lets make the output buffer a reasonable size */
	if (!BIO_set_write_buffer_size(io,bufsize)) goto err;

	if ((con=SSL_new(ctx)) == NULL) goto err;
#ifndef OPENSSL_NO_TLSEXT
		if (s_tlsextdebug)
			{
			SSL_set_tlsext_debug_callback(con, tlsext_cb);
			SSL_set_tlsext_debug_arg(con, bio_s_out);
			}
#endif
#ifndef OPENSSL_NO_KRB5
	if ((kctx = kssl_ctx_new()) != NULL)
		{
		kssl_ctx_setstring(kctx, KSSL_SERVICE, KRB5SVC);
		kssl_ctx_setstring(kctx, KSSL_KEYTAB, KRB5KEYTAB);
		}
#endif	/* OPENSSL_NO_KRB5 */
	if(context) SSL_set_session_id_context(con, context,
					       strlen((char *)context));

	sbio=BIO_new_socket(s,BIO_NOCLOSE);
	if (s_nbio_test)
		{
		BIO *test;

		test=BIO_new(BIO_f_nbio_test());
		sbio=BIO_push(test,sbio);
		}
	SSL_set_bio(con,sbio,sbio);
	SSL_set_accept_state(con);

	/* SSL_set_fd(con,s); */
	BIO_set_ssl(ssl_bio,con,BIO_CLOSE);
	BIO_push(io,ssl_bio);
#ifdef CHARSET_EBCDIC
	io = BIO_push(BIO_new(BIO_f_ebcdic_filter()),io);
#endif

	if (s_debug)
		{
		SSL_set_debug(con, 1);
		BIO_set_callback(SSL_get_rbio(con),bio_dump_callback);
		BIO_set_callback_arg(SSL_get_rbio(con),(char *)bio_s_out);
		}
	if (s_msg)
		{
		SSL_set_msg_callback(con, msg_cb);
		SSL_set_msg_callback_arg(con, bio_s_out);
		}

	for (;;)
		{
		if (hack)
			{
			i=SSL_accept(con);
#ifndef OPENSSL_NO_SRP
			while (i <= 0 &&  SSL_get_error(con,i) == SSL_ERROR_WANT_X509_LOOKUP) 
		{
			BIO_printf(bio_s_out,"LOOKUP during accept %s\n",srp_callback_parm.login);
			srp_callback_parm.user = SRP_VBASE_get_by_user(srp_callback_parm.vb, srp_callback_parm.login); 
			if (srp_callback_parm.user) 
				BIO_printf(bio_s_out,"LOOKUP done %s\n",srp_callback_parm.user->info);
			else 
				BIO_printf(bio_s_out,"LOOKUP not successful\n");
			i=SSL_accept(con);
		}
#endif
			switch (SSL_get_error(con,i))
				{
			case SSL_ERROR_NONE:
				break;
			case SSL_ERROR_WANT_WRITE:
			case SSL_ERROR_WANT_READ:
			case SSL_ERROR_WANT_X509_LOOKUP:
				continue;
			case SSL_ERROR_SYSCALL:
			case SSL_ERROR_SSL:
			case SSL_ERROR_ZERO_RETURN:
				ret=1;
				goto err;
				/* break; */
				}

			SSL_renegotiate(con);
			SSL_write(con,NULL,0);
			}

		i=BIO_gets(io,buf,bufsize-1);
		if (i < 0) /* error */
			{
			if (!BIO_should_retry(io))
				{
				if (!s_quiet)
					ERR_print_errors(bio_err);
				goto err;
				}
			else
				{
				BIO_printf(bio_s_out,"read R BLOCK\n");
#if defined(OPENSSL_SYS_NETWARE)
            delay(1000);
#elif !defined(OPENSSL_SYS_MSDOS) && !defined(__DJGPP__)
				sleep(1);
#endif
				continue;
				}
			}
		else if (i == 0) /* end of input */
			{
			ret=1;
			goto end;
			}

		/* else we have data */
		if (	((www == 1) && (strncmp("GET ",buf,4) == 0)) ||
			((www == 2) && (strncmp("GET /stats ",buf,10) == 0)))
			{
			char *p;
			X509 *peer;
			STACK_OF(SSL_CIPHER) *sk;
			static const char *space="                          ";

			BIO_puts(io,"HTTP/1.0 200 ok\r\nContent-type: text/html\r\n\r\n");
			BIO_puts(io,"<HTML><BODY BGCOLOR=\"#ffffff\">\n");
			BIO_puts(io,"<pre>\n");
/*			BIO_puts(io,SSLeay_version(SSLEAY_VERSION));*/
			BIO_puts(io,"\n");
			for (i=0; i<local_argc; i++)
				{
				BIO_puts(io,local_argv[i]);
				BIO_write(io," ",1);
				}
			BIO_puts(io,"\n");

			BIO_printf(io,
				"Secure Renegotiation IS%s supported\n",
		      		SSL_get_secure_renegotiation_support(con) ?
							"" : " NOT");

			/* The following is evil and should not really
			 * be done */
			BIO_printf(io,"Ciphers supported in s_server binary\n");
			sk=SSL_get_ciphers(con);
			j=sk_SSL_CIPHER_num(sk);
			for (i=0; i<j; i++)
				{
				c=sk_SSL_CIPHER_value(sk,i);
				BIO_printf(io,"%-11s:%-25s",
					SSL_CIPHER_get_version(c),
					SSL_CIPHER_get_name(c));
				if ((((i+1)%2) == 0) && (i+1 != j))
					BIO_puts(io,"\n");
				}
			BIO_puts(io,"\n");
			p=SSL_get_shared_ciphers(con,buf,bufsize);
			if (p != NULL)
				{
				BIO_printf(io,"---\nCiphers common between both SSL end points:\n");
				j=i=0;
				while (*p)
					{
					if (*p == ':')
						{
						BIO_write(io,space,26-j);
						i++;
						j=0;
						BIO_write(io,((i%3)?" ":"\n"),1);
						}
					else
						{
						BIO_write(io,p,1);
						j++;
						}
					p++;
					}
				BIO_puts(io,"\n");
				}
			ssl_print_sigalgs(io, con);
			ssl_print_curves(io, con);
			BIO_printf(io,(SSL_cache_hit(con)
				?"---\nReused, "
				:"---\nNew, "));
			c=SSL_get_current_cipher(con);
			BIO_printf(io,"%s, Cipher is %s\n",
				SSL_CIPHER_get_version(c),
				SSL_CIPHER_get_name(c));
			SSL_SESSION_print(io,SSL_get_session(con));
			BIO_printf(io,"---\n");
			print_stats(io,SSL_get_SSL_CTX(con));
			BIO_printf(io,"---\n");
			peer=SSL_get_peer_certificate(con);
			if (peer != NULL)
				{
				BIO_printf(io,"Client certificate\n");
				X509_print(io,peer);
				PEM_write_bio_X509(io,peer);
				}
			else
				BIO_puts(io,"no client certificate available\n");
			BIO_puts(io,"</BODY></HTML>\r\n\r\n");
			break;
			}
		else if ((www == 2 || www == 3)
                         && (strncmp("GET /",buf,5) == 0))
			{
			BIO *file;
			char *p,*e;
			static const char *text="HTTP/1.0 200 ok\r\nContent-type: text/plain\r\n\r\n";

			/* skip the '/' */
			p= &(buf[5]);

			dot = 1;
			for (e=p; *e != '\0'; e++)
				{
				if (e[0] == ' ')
					break;

				switch (dot)
					{
				case 1:
					dot = (e[0] == '.') ? 2 : 0;
					break;
				case 2:
					dot = (e[0] == '.') ? 3 : 0;
					break;
				case 3:
					dot = (e[0] == '/') ? -1 : 0;
					break;
					}
				if (dot == 0)
					dot = (e[0] == '/') ? 1 : 0;
				}
			dot = (dot == 3) || (dot == -1); /* filename contains ".." component */

			if (*e == '\0')
				{
				BIO_puts(io,text);
				BIO_printf(io,"'%s' is an invalid file name\r\n",p);
				break;
				}
			*e='\0';

			if (dot)
				{
				BIO_puts(io,text);
				BIO_printf(io,"'%s' contains '..' reference\r\n",p);
				break;
				}

			if (*p == '/')
				{
				BIO_puts(io,text);
				BIO_printf(io,"'%s' is an invalid path\r\n",p);
				break;
				}

#if 0
			/* append if a directory lookup */
			if (e[-1] == '/')
				strcat(p,"index.html");
#endif

			/* if a directory, do the index thang */
			if (app_isdir(p)>0)
				{
#if 0 /* must check buffer size */
				strcat(p,"/index.html");
#else
				BIO_puts(io,text);
				BIO_printf(io,"'%s' is a directory\r\n",p);
				break;
#endif
				}

			if ((file=BIO_new_file(p,"r")) == NULL)
				{
				BIO_puts(io,text);
				BIO_printf(io,"Error opening '%s'\r\n",p);
				ERR_print_errors(io);
				break;
				}

			if (!s_quiet)
				BIO_printf(bio_err,"FILE:%s\n",p);

                        if (www == 2)
                                {
                                i=strlen(p);
                                if (	((i > 5) && (strcmp(&(p[i-5]),".html") == 0)) ||
                                        ((i > 4) && (strcmp(&(p[i-4]),".php") == 0)) ||
                                        ((i > 4) && (strcmp(&(p[i-4]),".htm") == 0)))
                                        BIO_puts(io,"HTTP/1.0 200 ok\r\nContent-type: text/html\r\n\r\n");
                                else
                                        BIO_puts(io,"HTTP/1.0 200 ok\r\nContent-type: text/plain\r\n\r\n");
                                }
			/* send the file */
			for (;;)
				{
				i=BIO_read(file,buf,bufsize);
				if (i <= 0) break;

#ifdef RENEG
				total_bytes+=i;
				fprintf(stderr,"%d\n",i);
				if (total_bytes > 3*1024)
					{
					total_bytes=0;
					fprintf(stderr,"RENEGOTIATE\n");
					SSL_renegotiate(con);
					}
#endif

				for (j=0; j<i; )
					{
#ifdef RENEG
{ static count=0; if (++count == 13) { SSL_renegotiate(con); } }
#endif
					k=BIO_write(io,&(buf[j]),i-j);
					if (k <= 0)
						{
						if (!BIO_should_retry(io))
							goto write_error;
						else
							{
							BIO_printf(bio_s_out,"rwrite W BLOCK\n");
							}
						}
					else
						{
						j+=k;
						}
					}
				}
write_error:
			BIO_free(file);
			break;
			}
		}

	for (;;)
		{
		i=(int)BIO_flush(io);
		if (i <= 0)
			{
			if (!BIO_should_retry(io))
				break;
			}
		else
			break;
		}
end:
#if 1
	/* make sure we re-use sessions */
	SSL_set_shutdown(con,SSL_SENT_SHUTDOWN|SSL_RECEIVED_SHUTDOWN);
#else
	/* This kills performance */
/*	SSL_shutdown(con); A shutdown gets sent in the
 *	BIO_free_all(io) procession */
#endif

err:

	if (ret >= 0)
		BIO_printf(bio_s_out,"ACCEPT\n");

	if (buf != NULL) OPENSSL_free(buf);
	if (io != NULL) BIO_free_all(io);
/*	if (ssl_bio != NULL) BIO_free(ssl_bio);*/
	return(ret);
	}