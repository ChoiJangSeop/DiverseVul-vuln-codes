int MAIN(int argc, char **argv)
	{
	int off=0, clr = 0;
	SSL *con=NULL,*con2=NULL;
	X509_STORE *store = NULL;
	int s,k,width,state=0;
	char *cbuf=NULL,*sbuf=NULL,*mbuf=NULL;
	int cbuf_len,cbuf_off;
	int sbuf_len,sbuf_off;
	fd_set readfds,writefds;
	short port=PORT;
	int full_log=1;
	char *host=SSL_HOST_NAME;
	char *cert_file=NULL,*key_file=NULL;
	int cert_format = FORMAT_PEM, key_format = FORMAT_PEM;
	char *passarg = NULL, *pass = NULL;
	X509 *cert = NULL;
	EVP_PKEY *key = NULL;
	char *CApath=NULL,*CAfile=NULL,*cipher=NULL;
	int reconnect=0,badop=0,verify=SSL_VERIFY_NONE,bugs=0;
	int crlf=0;
	int write_tty,read_tty,write_ssl,read_ssl,tty_on,ssl_pending;
	SSL_CTX *ctx=NULL;
	int ret=1,in_init=1,i,nbio_test=0;
	int starttls_proto = PROTO_OFF;
	int prexit = 0, vflags = 0;
	SSL_METHOD *meth=NULL;
#ifdef sock_type
#undef sock_type
#endif
	int sock_type=SOCK_STREAM;
	BIO *sbio;
	char *inrand=NULL;
	int mbuf_len=0;
	struct timeval timeout, *timeoutp;
#ifndef OPENSSL_NO_ENGINE
	char *engine_id=NULL;
	char *ssl_client_engine_id=NULL;
	ENGINE *ssl_client_engine=NULL;
#endif
	ENGINE *e=NULL;
#if defined(OPENSSL_SYS_WINDOWS) || defined(OPENSSL_SYS_MSDOS) || defined(OPENSSL_SYS_NETWARE)
	struct timeval tv;
#endif

#ifndef OPENSSL_NO_TLSEXT
	char *servername = NULL; 
        tlsextctx tlsextcbp = 
        {NULL,0};
#endif
	char *sess_in = NULL;
	char *sess_out = NULL;
	struct sockaddr peer;
	int peerlen = sizeof(peer);
	int enable_timeouts = 0 ;
	long socket_mtu = 0;
#ifndef OPENSSL_NO_JPAKE
	char *jpake_secret = NULL;
#endif

	meth=SSLv23_client_method();

	apps_startup();
	c_Pause=0;
	c_quiet=0;
	c_ign_eof=0;
	c_debug=0;
	c_msg=0;
	c_showcerts=0;

	if (bio_err == NULL)
		bio_err=BIO_new_fp(stderr,BIO_NOCLOSE);

	if (!load_config(bio_err, NULL))
		goto end;

	if (	((cbuf=OPENSSL_malloc(BUFSIZZ)) == NULL) ||
		((sbuf=OPENSSL_malloc(BUFSIZZ)) == NULL) ||
		((mbuf=OPENSSL_malloc(BUFSIZZ)) == NULL))
		{
		BIO_printf(bio_err,"out of memory\n");
		goto end;
		}

	verify_depth=0;
	verify_error=X509_V_OK;
#ifdef FIONBIO
	c_nbio=0;
#endif

	argc--;
	argv++;
	while (argc >= 1)
		{
		if	(strcmp(*argv,"-host") == 0)
			{
			if (--argc < 1) goto bad;
			host= *(++argv);
			}
		else if	(strcmp(*argv,"-port") == 0)
			{
			if (--argc < 1) goto bad;
			port=atoi(*(++argv));
			if (port == 0) goto bad;
			}
		else if (strcmp(*argv,"-connect") == 0)
			{
			if (--argc < 1) goto bad;
			if (!extract_host_port(*(++argv),&host,NULL,&port))
				goto bad;
			}
		else if	(strcmp(*argv,"-verify") == 0)
			{
			verify=SSL_VERIFY_PEER;
			if (--argc < 1) goto bad;
			verify_depth=atoi(*(++argv));
			BIO_printf(bio_err,"verify depth is %d\n",verify_depth);
			}
		else if	(strcmp(*argv,"-cert") == 0)
			{
			if (--argc < 1) goto bad;
			cert_file= *(++argv);
			}
		else if	(strcmp(*argv,"-sess_out") == 0)
			{
			if (--argc < 1) goto bad;
			sess_out = *(++argv);
			}
		else if	(strcmp(*argv,"-sess_in") == 0)
			{
			if (--argc < 1) goto bad;
			sess_in = *(++argv);
			}
		else if	(strcmp(*argv,"-certform") == 0)
			{
			if (--argc < 1) goto bad;
			cert_format = str2fmt(*(++argv));
			}
		else if	(strcmp(*argv,"-crl_check") == 0)
			vflags |= X509_V_FLAG_CRL_CHECK;
		else if	(strcmp(*argv,"-crl_check_all") == 0)
			vflags |= X509_V_FLAG_CRL_CHECK|X509_V_FLAG_CRL_CHECK_ALL;
		else if	(strcmp(*argv,"-prexit") == 0)
			prexit=1;
		else if	(strcmp(*argv,"-crlf") == 0)
			crlf=1;
		else if	(strcmp(*argv,"-quiet") == 0)
			{
			c_quiet=1;
			c_ign_eof=1;
			}
		else if	(strcmp(*argv,"-ign_eof") == 0)
			c_ign_eof=1;
		else if	(strcmp(*argv,"-no_ign_eof") == 0)
			c_ign_eof=0;
		else if	(strcmp(*argv,"-pause") == 0)
			c_Pause=1;
		else if	(strcmp(*argv,"-debug") == 0)
			c_debug=1;
#ifndef OPENSSL_NO_TLSEXT
		else if	(strcmp(*argv,"-tlsextdebug") == 0)
			c_tlsextdebug=1;
		else if	(strcmp(*argv,"-status") == 0)
			c_status_req=1;
#endif
#ifdef WATT32
		else if (strcmp(*argv,"-wdebug") == 0)
			dbug_init();
#endif
		else if	(strcmp(*argv,"-msg") == 0)
			c_msg=1;
		else if	(strcmp(*argv,"-showcerts") == 0)
			c_showcerts=1;
		else if	(strcmp(*argv,"-nbio_test") == 0)
			nbio_test=1;
		else if	(strcmp(*argv,"-state") == 0)
			state=1;
#ifndef OPENSSL_NO_SSL2
		else if	(strcmp(*argv,"-ssl2") == 0)
			meth=SSLv2_client_method();
#endif
#ifndef OPENSSL_NO_SSL3
		else if	(strcmp(*argv,"-ssl3") == 0)
			meth=SSLv3_client_method();
#endif
#ifndef OPENSSL_NO_TLS1
		else if	(strcmp(*argv,"-tls1") == 0)
			meth=TLSv1_client_method();
#endif
#ifndef OPENSSL_NO_DTLS1
		else if	(strcmp(*argv,"-dtls1") == 0)
			{
			meth=DTLSv1_client_method();
			sock_type=SOCK_DGRAM;
			}
		else if (strcmp(*argv,"-timeout") == 0)
			enable_timeouts=1;
		else if (strcmp(*argv,"-mtu") == 0)
			{
			if (--argc < 1) goto bad;
			socket_mtu = atol(*(++argv));
			}
#endif
		else if (strcmp(*argv,"-bugs") == 0)
			bugs=1;
		else if	(strcmp(*argv,"-keyform") == 0)
			{
			if (--argc < 1) goto bad;
			key_format = str2fmt(*(++argv));
			}
		else if	(strcmp(*argv,"-pass") == 0)
			{
			if (--argc < 1) goto bad;
			passarg = *(++argv);
			}
		else if	(strcmp(*argv,"-key") == 0)
			{
			if (--argc < 1) goto bad;
			key_file= *(++argv);
			}
		else if	(strcmp(*argv,"-reconnect") == 0)
			{
			reconnect=5;
			}
		else if	(strcmp(*argv,"-CApath") == 0)
			{
			if (--argc < 1) goto bad;
			CApath= *(++argv);
			}
		else if	(strcmp(*argv,"-CAfile") == 0)
			{
			if (--argc < 1) goto bad;
			CAfile= *(++argv);
			}
		else if (strcmp(*argv,"-no_tls1") == 0)
			off|=SSL_OP_NO_TLSv1;
		else if (strcmp(*argv,"-no_ssl3") == 0)
			off|=SSL_OP_NO_SSLv3;
		else if (strcmp(*argv,"-no_ssl2") == 0)
			off|=SSL_OP_NO_SSLv2;
#ifndef OPENSSL_NO_TLSEXT
		else if	(strcmp(*argv,"-no_ticket") == 0)
			{ off|=SSL_OP_NO_TICKET; }
#endif
		else if (strcmp(*argv,"-serverpref") == 0)
			off|=SSL_OP_CIPHER_SERVER_PREFERENCE;
		else if (strcmp(*argv,"-legacy_renegotiation") == 0)
			off|=SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION;
		else if	(strcmp(*argv,"-legacy_server_connect") == 0)
			{ off|=SSL_OP_LEGACY_SERVER_CONNECT; }
		else if	(strcmp(*argv,"-no_legacy_server_connect") == 0)
			{ clr|=SSL_OP_LEGACY_SERVER_CONNECT; }
		else if	(strcmp(*argv,"-cipher") == 0)
			{
			if (--argc < 1) goto bad;
			cipher= *(++argv);
			}
#ifdef FIONBIO
		else if (strcmp(*argv,"-nbio") == 0)
			{ c_nbio=1; }
#endif
		else if	(strcmp(*argv,"-starttls") == 0)
			{
			if (--argc < 1) goto bad;
			++argv;
			if (strcmp(*argv,"smtp") == 0)
				starttls_proto = PROTO_SMTP;
			else if (strcmp(*argv,"pop3") == 0)
				starttls_proto = PROTO_POP3;
			else if (strcmp(*argv,"imap") == 0)
				starttls_proto = PROTO_IMAP;
			else if (strcmp(*argv,"ftp") == 0)
				starttls_proto = PROTO_FTP;
			else if (strcmp(*argv, "xmpp") == 0)
				starttls_proto = PROTO_XMPP;
			else
				goto bad;
			}
#ifndef OPENSSL_NO_ENGINE
		else if	(strcmp(*argv,"-engine") == 0)
			{
			if (--argc < 1) goto bad;
			engine_id = *(++argv);
			}
		else if	(strcmp(*argv,"-ssl_client_engine") == 0)
			{
			if (--argc < 1) goto bad;
			ssl_client_engine_id = *(++argv);
			}
#endif
		else if (strcmp(*argv,"-rand") == 0)
			{
			if (--argc < 1) goto bad;
			inrand= *(++argv);
			}
#ifndef OPENSSL_NO_TLSEXT
		else if (strcmp(*argv,"-servername") == 0)
			{
			if (--argc < 1) goto bad;
			servername= *(++argv);
			/* meth=TLSv1_client_method(); */
			}
#endif
#ifndef OPENSSL_NO_JPAKE
		else if (strcmp(*argv,"-jpake") == 0)
			{
			if (--argc < 1) goto bad;
			jpake_secret = *++argv;
			}
#endif
		else
			{
			BIO_printf(bio_err,"unknown option %s\n",*argv);
			badop=1;
			break;
			}
		argc--;
		argv++;
		}
	if (badop)
		{
bad:
		sc_usage();
		goto end;
		}

	OpenSSL_add_ssl_algorithms();
	SSL_load_error_strings();

#ifndef OPENSSL_NO_ENGINE
        e = setup_engine(bio_err, engine_id, 1);
	if (ssl_client_engine_id)
		{
		ssl_client_engine = ENGINE_by_id(ssl_client_engine_id);
		if (!ssl_client_engine)
			{
			BIO_printf(bio_err,
					"Error getting client auth engine\n");
			goto end;
			}
		}
#endif
	if (!app_passwd(bio_err, passarg, NULL, &pass, NULL))
		{
		BIO_printf(bio_err, "Error getting password\n");
		goto end;
		}

	if (key_file == NULL)
		key_file = cert_file;


	if (key_file)

		{

		key = load_key(bio_err, key_file, key_format, 0, pass, e,
			       "client certificate private key file");
		if (!key)
			{
			ERR_print_errors(bio_err);
			goto end;
			}

		}

	if (cert_file)

		{
		cert = load_cert(bio_err,cert_file,cert_format,
				NULL, e, "client certificate file");

		if (!cert)
			{
			ERR_print_errors(bio_err);
			goto end;
			}
		}

	if (!app_RAND_load_file(NULL, bio_err, 1) && inrand == NULL
		&& !RAND_status())
		{
		BIO_printf(bio_err,"warning, not much extra random data, consider using the -rand option\n");
		}
	if (inrand != NULL)
		BIO_printf(bio_err,"%ld semi-random bytes loaded\n",
			app_RAND_load_files(inrand));

	if (bio_c_out == NULL)
		{
		if (c_quiet && !c_debug && !c_msg)
			{
			bio_c_out=BIO_new(BIO_s_null());
			}
		else
			{
			if (bio_c_out == NULL)
				bio_c_out=BIO_new_fp(stdout,BIO_NOCLOSE);
			}
		}

	ctx=SSL_CTX_new(meth);
	if (ctx == NULL)
		{
		ERR_print_errors(bio_err);
		goto end;
		}

#ifndef OPENSSL_NO_ENGINE
	if (ssl_client_engine)
		{
		if (!SSL_CTX_set_client_cert_engine(ctx, ssl_client_engine))
			{
			BIO_puts(bio_err, "Error setting client auth engine\n");
			ERR_print_errors(bio_err);
			ENGINE_free(ssl_client_engine);
			goto end;
			}
		ENGINE_free(ssl_client_engine);
		}
#endif

	if (bugs)
		SSL_CTX_set_options(ctx,SSL_OP_ALL|off);
	else
		SSL_CTX_set_options(ctx,off);

	if (clr)
		SSL_CTX_clear_options(ctx, clr);
	/* DTLS: partial reads end up discarding unread UDP bytes :-( 
	 * Setting read ahead solves this problem.
	 */
	if (sock_type == SOCK_DGRAM) SSL_CTX_set_read_ahead(ctx, 1);

	if (state) SSL_CTX_set_info_callback(ctx,apps_ssl_info_callback);
	if (cipher != NULL)
		if(!SSL_CTX_set_cipher_list(ctx,cipher)) {
		BIO_printf(bio_err,"error setting cipher list\n");
		ERR_print_errors(bio_err);
		goto end;
	}
#if 0
	else
		SSL_CTX_set_cipher_list(ctx,getenv("SSL_CIPHER"));
#endif

	SSL_CTX_set_verify(ctx,verify,verify_callback);
	if (!set_cert_key_stuff(ctx,cert,key))
		goto end;

	if ((!SSL_CTX_load_verify_locations(ctx,CAfile,CApath)) ||
		(!SSL_CTX_set_default_verify_paths(ctx)))
		{
		/* BIO_printf(bio_err,"error setting default verify locations\n"); */
		ERR_print_errors(bio_err);
		/* goto end; */
		}

	store = SSL_CTX_get_cert_store(ctx);
	X509_STORE_set_flags(store, vflags);
#ifndef OPENSSL_NO_TLSEXT
	if (servername != NULL)
		{
		tlsextcbp.biodebug = bio_err;
		SSL_CTX_set_tlsext_servername_callback(ctx, ssl_servername_cb);
		SSL_CTX_set_tlsext_servername_arg(ctx, &tlsextcbp);
		}
#endif

	con=SSL_new(ctx);
	if (sess_in)
		{
		SSL_SESSION *sess;
		BIO *stmp = BIO_new_file(sess_in, "r");
		if (!stmp)
			{
			BIO_printf(bio_err, "Can't open session file %s\n",
						sess_in);
			ERR_print_errors(bio_err);
			goto end;
			}
		sess = PEM_read_bio_SSL_SESSION(stmp, NULL, 0, NULL);
		BIO_free(stmp);
		if (!sess)
			{
			BIO_printf(bio_err, "Can't open session file %s\n",
						sess_in);
			ERR_print_errors(bio_err);
			goto end;
			}
		SSL_set_session(con, sess);
		SSL_SESSION_free(sess);
		}
#ifndef OPENSSL_NO_TLSEXT
	if (servername != NULL)
		{
		if (!SSL_set_tlsext_host_name(con,servername))
			{
			BIO_printf(bio_err,"Unable to set TLS servername extension.\n");
			ERR_print_errors(bio_err);
			goto end;
			}
		}
#endif

#ifndef OPENSSL_NO_KRB5
	if (con  &&  (con->kssl_ctx = kssl_ctx_new()) != NULL)
                {
                kssl_ctx_setstring(con->kssl_ctx, KSSL_SERVER, host);
		}
#endif	/* OPENSSL_NO_KRB5  */
/*	SSL_set_cipher_list(con,"RC4-MD5"); */

re_start:

	if (init_client(&s,host,port,sock_type) == 0)
		{
		BIO_printf(bio_err,"connect:errno=%d\n",get_last_socket_error());
		SHUTDOWN(s);
		goto end;
		}
	BIO_printf(bio_c_out,"CONNECTED(%08X)\n",s);

#ifdef FIONBIO
	if (c_nbio)
		{
		unsigned long l=1;
		BIO_printf(bio_c_out,"turning on non blocking io\n");
		if (BIO_socket_ioctl(s,FIONBIO,&l) < 0)
			{
			ERR_print_errors(bio_err);
			goto end;
			}
		}
#endif                                              
	if (c_Pause & 0x01) con->debug=1;

	if ( SSL_version(con) == DTLS1_VERSION)
		{

		sbio=BIO_new_dgram(s,BIO_NOCLOSE);
		if (getsockname(s, &peer, (void *)&peerlen) < 0)
			{
			BIO_printf(bio_err, "getsockname:errno=%d\n",
				get_last_socket_error());
			SHUTDOWN(s);
			goto end;
			}

		(void)BIO_ctrl_set_connected(sbio, 1, &peer);

		if ( enable_timeouts)
			{
			timeout.tv_sec = 0;
			timeout.tv_usec = DGRAM_RCV_TIMEOUT;
			BIO_ctrl(sbio, BIO_CTRL_DGRAM_SET_RECV_TIMEOUT, 0, &timeout);
			
			timeout.tv_sec = 0;
			timeout.tv_usec = DGRAM_SND_TIMEOUT;
			BIO_ctrl(sbio, BIO_CTRL_DGRAM_SET_SEND_TIMEOUT, 0, &timeout);
			}

		if (socket_mtu > 28)
			{
			SSL_set_options(con, SSL_OP_NO_QUERY_MTU);
			SSL_set_mtu(con, socket_mtu - 28);
			}
		else
			/* want to do MTU discovery */
			BIO_ctrl(sbio, BIO_CTRL_DGRAM_MTU_DISCOVER, 0, NULL);
		}
	else
		sbio=BIO_new_socket(s,BIO_NOCLOSE);

	if (nbio_test)
		{
		BIO *test;

		test=BIO_new(BIO_f_nbio_test());
		sbio=BIO_push(test,sbio);
		}

	if (c_debug)
		{
		con->debug=1;
		BIO_set_callback(sbio,bio_dump_callback);
		BIO_set_callback_arg(sbio,(char *)bio_c_out);
		}
	if (c_msg)
		{
		SSL_set_msg_callback(con, msg_cb);
		SSL_set_msg_callback_arg(con, bio_c_out);
		}
#ifndef OPENSSL_NO_TLSEXT
	if (c_tlsextdebug)
		{
		SSL_set_tlsext_debug_callback(con, tlsext_cb);
		SSL_set_tlsext_debug_arg(con, bio_c_out);
		}
	if (c_status_req)
		{
		SSL_set_tlsext_status_type(con, TLSEXT_STATUSTYPE_ocsp);
		SSL_CTX_set_tlsext_status_cb(ctx, ocsp_resp_cb);
		SSL_CTX_set_tlsext_status_arg(ctx, bio_c_out);
#if 0
{
STACK_OF(OCSP_RESPID) *ids = sk_OCSP_RESPID_new_null();
OCSP_RESPID *id = OCSP_RESPID_new();
id->value.byKey = ASN1_OCTET_STRING_new();
id->type = V_OCSP_RESPID_KEY;
ASN1_STRING_set(id->value.byKey, "Hello World", -1);
sk_OCSP_RESPID_push(ids, id);
SSL_set_tlsext_status_ids(con, ids);
}
#endif
		}
#endif
#ifndef OPENSSL_NO_JPAKE
	if (jpake_secret)
		jpake_client_auth(bio_c_out, sbio, jpake_secret);
#endif

	SSL_set_bio(con,sbio,sbio);
	SSL_set_connect_state(con);

	/* ok, lets connect */
	width=SSL_get_fd(con)+1;

	read_tty=1;
	write_tty=0;
	tty_on=0;
	read_ssl=1;
	write_ssl=1;
	
	cbuf_len=0;
	cbuf_off=0;
	sbuf_len=0;
	sbuf_off=0;

	/* This is an ugly hack that does a lot of assumptions */
	/* We do have to handle multi-line responses which may come
 	   in a single packet or not. We therefore have to use
	   BIO_gets() which does need a buffering BIO. So during
	   the initial chitchat we do push a buffering BIO into the
	   chain that is removed again later on to not disturb the
	   rest of the s_client operation. */
	if (starttls_proto == PROTO_SMTP)
		{
		int foundit=0;
		BIO *fbio = BIO_new(BIO_f_buffer());
		BIO_push(fbio, sbio);
		/* wait for multi-line response to end from SMTP */
		do
			{
			mbuf_len = BIO_gets(fbio,mbuf,BUFSIZZ);
			}
		while (mbuf_len>3 && mbuf[3]=='-');
		/* STARTTLS command requires EHLO... */
		BIO_printf(fbio,"EHLO openssl.client.net\r\n");
		(void)BIO_flush(fbio);
		/* wait for multi-line response to end EHLO SMTP response */
		do
			{
			mbuf_len = BIO_gets(fbio,mbuf,BUFSIZZ);
			if (strstr(mbuf,"STARTTLS"))
				foundit=1;
			}
		while (mbuf_len>3 && mbuf[3]=='-');
		(void)BIO_flush(fbio);
		BIO_pop(fbio);
		BIO_free(fbio);
		if (!foundit)
			BIO_printf(bio_err,
				   "didn't found starttls in server response,"
				   " try anyway...\n");
		BIO_printf(sbio,"STARTTLS\r\n");
		BIO_read(sbio,sbuf,BUFSIZZ);
		}
	else if (starttls_proto == PROTO_POP3)
		{
		BIO_read(sbio,mbuf,BUFSIZZ);
		BIO_printf(sbio,"STLS\r\n");
		BIO_read(sbio,sbuf,BUFSIZZ);
		}
	else if (starttls_proto == PROTO_IMAP)
		{
		int foundit=0;
		BIO *fbio = BIO_new(BIO_f_buffer());
		BIO_push(fbio, sbio);
		BIO_gets(fbio,mbuf,BUFSIZZ);
		/* STARTTLS command requires CAPABILITY... */
		BIO_printf(fbio,". CAPABILITY\r\n");
		(void)BIO_flush(fbio);
		/* wait for multi-line CAPABILITY response */
		do
			{
			mbuf_len = BIO_gets(fbio,mbuf,BUFSIZZ);
			if (strstr(mbuf,"STARTTLS"))
				foundit=1;
			}
		while (mbuf_len>3 && mbuf[0]!='.');
		(void)BIO_flush(fbio);
		BIO_pop(fbio);
		BIO_free(fbio);
		if (!foundit)
			BIO_printf(bio_err,
				   "didn't found STARTTLS in server response,"
				   " try anyway...\n");
		BIO_printf(sbio,". STARTTLS\r\n");
		BIO_read(sbio,sbuf,BUFSIZZ);
		}
	else if (starttls_proto == PROTO_FTP)
		{
		BIO *fbio = BIO_new(BIO_f_buffer());
		BIO_push(fbio, sbio);
		/* wait for multi-line response to end from FTP */
		do
			{
			mbuf_len = BIO_gets(fbio,mbuf,BUFSIZZ);
			}
		while (mbuf_len>3 && mbuf[3]=='-');
		(void)BIO_flush(fbio);
		BIO_pop(fbio);
		BIO_free(fbio);
		BIO_printf(sbio,"AUTH TLS\r\n");
		BIO_read(sbio,sbuf,BUFSIZZ);
		}
	if (starttls_proto == PROTO_XMPP)
		{
		int seen = 0;
		BIO_printf(sbio,"<stream:stream "
		    "xmlns:stream='http://etherx.jabber.org/streams' "
		    "xmlns='jabber:client' to='%s' version='1.0'>", host);
		seen = BIO_read(sbio,mbuf,BUFSIZZ);
		mbuf[seen] = 0;
		while (!strstr(mbuf, "<starttls xmlns='urn:ietf:params:xml:ns:xmpp-tls'"))
			{
			if (strstr(mbuf, "/stream:features>"))
				goto shut;
			seen = BIO_read(sbio,mbuf,BUFSIZZ);
			mbuf[seen] = 0;
			}
		BIO_printf(sbio, "<starttls xmlns='urn:ietf:params:xml:ns:xmpp-tls'/>");
		seen = BIO_read(sbio,sbuf,BUFSIZZ);
		sbuf[seen] = 0;
		if (!strstr(sbuf, "<proceed"))
			goto shut;
		mbuf[0] = 0;
		}

	for (;;)
		{
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);

		if ((SSL_version(con) == DTLS1_VERSION) &&
			DTLSv1_get_timeout(con, &timeout))
			timeoutp = &timeout;
		else
			timeoutp = NULL;

		if (SSL_in_init(con) && !SSL_total_renegotiations(con))
			{
			in_init=1;
			tty_on=0;
			}
		else
			{
			tty_on=1;
			if (in_init)
				{
				in_init=0;
				if (sess_out)
					{
					BIO *stmp = BIO_new_file(sess_out, "w");
					if (stmp)
						{
						PEM_write_bio_SSL_SESSION(stmp, SSL_get_session(con));
						BIO_free(stmp);
						}
					else 
						BIO_printf(bio_err, "Error writing session file %s\n", sess_out);
					}
				print_stuff(bio_c_out,con,full_log);
				if (full_log > 0) full_log--;

				if (starttls_proto)
					{
					BIO_printf(bio_err,"%s",mbuf);
					/* We don't need to know any more */
					starttls_proto = PROTO_OFF;
					}

				if (reconnect)
					{
					reconnect--;
					BIO_printf(bio_c_out,"drop connection and then reconnect\n");
					SSL_shutdown(con);
					SSL_set_connect_state(con);
					SHUTDOWN(SSL_get_fd(con));
					goto re_start;
					}
				}
			}

		ssl_pending = read_ssl && SSL_pending(con);

		if (!ssl_pending)
			{
#if !defined(OPENSSL_SYS_WINDOWS) && !defined(OPENSSL_SYS_MSDOS) && !defined(OPENSSL_SYS_NETWARE)
			if (tty_on)
				{
				if (read_tty)  FD_SET(fileno(stdin),&readfds);
				if (write_tty) FD_SET(fileno(stdout),&writefds);
				}
			if (read_ssl)
				FD_SET(SSL_get_fd(con),&readfds);
			if (write_ssl)
				FD_SET(SSL_get_fd(con),&writefds);
#else
			if(!tty_on || !write_tty) {
				if (read_ssl)
					FD_SET(SSL_get_fd(con),&readfds);
				if (write_ssl)
					FD_SET(SSL_get_fd(con),&writefds);
			}
#endif
/*			printf("mode tty(%d %d%d) ssl(%d%d)\n",
				tty_on,read_tty,write_tty,read_ssl,write_ssl);*/

			/* Note: under VMS with SOCKETSHR the second parameter
			 * is currently of type (int *) whereas under other
			 * systems it is (void *) if you don't have a cast it
			 * will choke the compiler: if you do have a cast then
			 * you can either go for (int *) or (void *).
			 */
#if defined(OPENSSL_SYS_WINDOWS) || defined(OPENSSL_SYS_MSDOS)
                        /* Under Windows/DOS we make the assumption that we can
			 * always write to the tty: therefore if we need to
			 * write to the tty we just fall through. Otherwise
			 * we timeout the select every second and see if there
			 * are any keypresses. Note: this is a hack, in a proper
			 * Windows application we wouldn't do this.
			 */
			i=0;
			if(!write_tty) {
				if(read_tty) {
					tv.tv_sec = 1;
					tv.tv_usec = 0;
					i=select(width,(void *)&readfds,(void *)&writefds,
						 NULL,&tv);
#if defined(OPENSSL_SYS_WINCE) || defined(OPENSSL_SYS_MSDOS)
					if(!i && (!_kbhit() || !read_tty) ) continue;
#else
					if(!i && (!((_kbhit()) || (WAIT_OBJECT_0 == WaitForSingleObject(GetStdHandle(STD_INPUT_HANDLE), 0))) || !read_tty) ) continue;
#endif
				} else 	i=select(width,(void *)&readfds,(void *)&writefds,
					 NULL,timeoutp);
			}
#elif defined(OPENSSL_SYS_NETWARE)
			if(!write_tty) {
				if(read_tty) {
					tv.tv_sec = 1;
					tv.tv_usec = 0;
					i=select(width,(void *)&readfds,(void *)&writefds,
						NULL,&tv);
				} else 	i=select(width,(void *)&readfds,(void *)&writefds,
					NULL,timeoutp);
			}
#else
			i=select(width,(void *)&readfds,(void *)&writefds,
				 NULL,timeoutp);
#endif
			if ( i < 0)
				{
				BIO_printf(bio_err,"bad select %d\n",
				get_last_socket_error());
				goto shut;
				/* goto end; */
				}
			}

		if ((SSL_version(con) == DTLS1_VERSION) && DTLSv1_handle_timeout(con) > 0)
			{
			BIO_printf(bio_err,"TIMEOUT occured\n");
			}

		if (!ssl_pending && FD_ISSET(SSL_get_fd(con),&writefds))
			{
			k=SSL_write(con,&(cbuf[cbuf_off]),
				(unsigned int)cbuf_len);
			switch (SSL_get_error(con,k))
				{
			case SSL_ERROR_NONE:
				cbuf_off+=k;
				cbuf_len-=k;
				if (k <= 0) goto end;
				/* we have done a  write(con,NULL,0); */
				if (cbuf_len <= 0)
					{
					read_tty=1;
					write_ssl=0;
					}
				else /* if (cbuf_len > 0) */
					{
					read_tty=0;
					write_ssl=1;
					}
				break;
			case SSL_ERROR_WANT_WRITE:
				BIO_printf(bio_c_out,"write W BLOCK\n");
				write_ssl=1;
				read_tty=0;
				break;
			case SSL_ERROR_WANT_READ:
				BIO_printf(bio_c_out,"write R BLOCK\n");
				write_tty=0;
				read_ssl=1;
				write_ssl=0;
				break;
			case SSL_ERROR_WANT_X509_LOOKUP:
				BIO_printf(bio_c_out,"write X BLOCK\n");
				break;
			case SSL_ERROR_ZERO_RETURN:
				if (cbuf_len != 0)
					{
					BIO_printf(bio_c_out,"shutdown\n");
					goto shut;
					}
				else
					{
					read_tty=1;
					write_ssl=0;
					break;
					}
				
			case SSL_ERROR_SYSCALL:
				if ((k != 0) || (cbuf_len != 0))
					{
					BIO_printf(bio_err,"write:errno=%d\n",
						get_last_socket_error());
					goto shut;
					}
				else
					{
					read_tty=1;
					write_ssl=0;
					}
				break;
			case SSL_ERROR_SSL:
				ERR_print_errors(bio_err);
				goto shut;
				}
			}
#if defined(OPENSSL_SYS_WINDOWS) || defined(OPENSSL_SYS_MSDOS) || defined(OPENSSL_SYS_NETWARE)
		/* Assume Windows/DOS can always write */
		else if (!ssl_pending && write_tty)
#else
		else if (!ssl_pending && FD_ISSET(fileno(stdout),&writefds))
#endif
			{
#ifdef CHARSET_EBCDIC
			ascii2ebcdic(&(sbuf[sbuf_off]),&(sbuf[sbuf_off]),sbuf_len);
#endif
			i=write(fileno(stdout),&(sbuf[sbuf_off]),sbuf_len);

			if (i <= 0)
				{
				BIO_printf(bio_c_out,"DONE\n");
				goto shut;
				/* goto end; */
				}

			sbuf_len-=i;;
			sbuf_off+=i;
			if (sbuf_len <= 0)
				{
				read_ssl=1;
				write_tty=0;
				}
			}
		else if (ssl_pending || FD_ISSET(SSL_get_fd(con),&readfds))
			{
#ifdef RENEG
{ static int iiii; if (++iiii == 52) { SSL_renegotiate(con); iiii=0; } }
#endif
#if 1
			k=SSL_read(con,sbuf,1024 /* BUFSIZZ */ );
#else
/* Demo for pending and peek :-) */
			k=SSL_read(con,sbuf,16);
{ char zbuf[10240]; 
printf("read=%d pending=%d peek=%d\n",k,SSL_pending(con),SSL_peek(con,zbuf,10240));
}
#endif

			switch (SSL_get_error(con,k))
				{
			case SSL_ERROR_NONE:
				if (k <= 0)
					goto end;
				sbuf_off=0;
				sbuf_len=k;

				read_ssl=0;
				write_tty=1;
				break;
			case SSL_ERROR_WANT_WRITE:
				BIO_printf(bio_c_out,"read W BLOCK\n");
				write_ssl=1;
				read_tty=0;
				break;
			case SSL_ERROR_WANT_READ:
				BIO_printf(bio_c_out,"read R BLOCK\n");
				write_tty=0;
				read_ssl=1;
				if ((read_tty == 0) && (write_ssl == 0))
					write_ssl=1;
				break;
			case SSL_ERROR_WANT_X509_LOOKUP:
				BIO_printf(bio_c_out,"read X BLOCK\n");
				break;
			case SSL_ERROR_SYSCALL:
				BIO_printf(bio_err,"read:errno=%d\n",get_last_socket_error());
				goto shut;
			case SSL_ERROR_ZERO_RETURN:
				BIO_printf(bio_c_out,"closed\n");
				goto shut;
			case SSL_ERROR_SSL:
				ERR_print_errors(bio_err);
				goto shut;
				/* break; */
				}
			}

#if defined(OPENSSL_SYS_WINDOWS) || defined(OPENSSL_SYS_MSDOS)
#if defined(OPENSSL_SYS_WINCE) || defined(OPENSSL_SYS_MSDOS)
		else if (_kbhit())
#else
		else if ((_kbhit()) || (WAIT_OBJECT_0 == WaitForSingleObject(GetStdHandle(STD_INPUT_HANDLE), 0)))
#endif
#elif defined (OPENSSL_SYS_NETWARE)
        else if (_kbhit())
#else
		else if (FD_ISSET(fileno(stdin),&readfds))
#endif
			{
			if (crlf)
				{
				int j, lf_num;

				i=read(fileno(stdin),cbuf,BUFSIZZ/2);
				lf_num = 0;
				/* both loops are skipped when i <= 0 */
				for (j = 0; j < i; j++)
					if (cbuf[j] == '\n')
						lf_num++;
				for (j = i-1; j >= 0; j--)
					{
					cbuf[j+lf_num] = cbuf[j];
					if (cbuf[j] == '\n')
						{
						lf_num--;
						i++;
						cbuf[j+lf_num] = '\r';
						}
					}
				assert(lf_num == 0);
				}
			else
				i=read(fileno(stdin),cbuf,BUFSIZZ);

			if ((!c_ign_eof) && ((i <= 0) || (cbuf[0] == 'Q')))
				{
				BIO_printf(bio_err,"DONE\n");
				goto shut;
				}

			if ((!c_ign_eof) && (cbuf[0] == 'R'))
				{
				BIO_printf(bio_err,"RENEGOTIATING\n");
				SSL_renegotiate(con);
				cbuf_len=0;
				}
			else
				{
				cbuf_len=i;
				cbuf_off=0;
#ifdef CHARSET_EBCDIC
				ebcdic2ascii(cbuf, cbuf, i);
#endif
				}

			write_ssl=1;
			read_tty=0;
			}
		}
shut:
	SSL_shutdown(con);
	SHUTDOWN(SSL_get_fd(con));
	ret=0;
end:
	if(prexit) print_stuff(bio_c_out,con,1);
	if (con != NULL) SSL_free(con);
	if (con2 != NULL) SSL_free(con2);
	if (ctx != NULL) SSL_CTX_free(ctx);
	if (cert)
		X509_free(cert);
	if (key)
		EVP_PKEY_free(key);
	if (pass)
		OPENSSL_free(pass);
	if (cbuf != NULL) { OPENSSL_cleanse(cbuf,BUFSIZZ); OPENSSL_free(cbuf); }
	if (sbuf != NULL) { OPENSSL_cleanse(sbuf,BUFSIZZ); OPENSSL_free(sbuf); }
	if (mbuf != NULL) { OPENSSL_cleanse(mbuf,BUFSIZZ); OPENSSL_free(mbuf); }
	if (bio_c_out != NULL)
		{
		BIO_free(bio_c_out);
		bio_c_out=NULL;
		}
	apps_shutdown();
	OPENSSL_EXIT(ret);
	}