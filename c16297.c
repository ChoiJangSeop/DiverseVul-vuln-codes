int MAIN(int argc, char *argv[])
	{
	X509_VERIFY_PARAM *vpm = NULL;
	int badarg = 0;
	short port=PORT;
	char *CApath=NULL,*CAfile=NULL;
	unsigned char *context = NULL;
	char *dhfile = NULL;
#ifndef OPENSSL_NO_ECDH
	char *named_curve = NULL;
#endif
	int badop=0,bugs=0;
	int ret=1;
	int off=0;
	int no_tmp_rsa=0,no_dhe=0,no_ecdhe=0,nocert=0;
	int state=0;
	const SSL_METHOD *meth=NULL;
	int socket_type=SOCK_STREAM;
	ENGINE *e=NULL;
	char *inrand=NULL;
	int s_cert_format = FORMAT_PEM, s_key_format = FORMAT_PEM;
	char *passarg = NULL, *pass = NULL;
	char *dpassarg = NULL, *dpass = NULL;
	int s_dcert_format = FORMAT_PEM, s_dkey_format = FORMAT_PEM;
	X509 *s_cert = NULL, *s_dcert = NULL;
	EVP_PKEY *s_key = NULL, *s_dkey = NULL;
	int no_cache = 0, ext_cache = 0;
#ifndef OPENSSL_NO_TLSEXT
	EVP_PKEY *s_key2 = NULL;
	X509 *s_cert2 = NULL;
#endif
#ifndef OPENSSL_NO_TLSEXT
        tlsextctx tlsextcbp = {NULL, NULL, SSL_TLSEXT_ERR_ALERT_WARNING};
#endif
#ifndef OPENSSL_NO_PSK
	/* by default do not send a PSK identity hint */
	static char *psk_identity_hint=NULL;
#endif
#if !defined(OPENSSL_NO_SSL2) && !defined(OPENSSL_NO_SSL3)
	meth=SSLv23_server_method();
#elif !defined(OPENSSL_NO_SSL3)
	meth=SSLv3_server_method();
#elif !defined(OPENSSL_NO_SSL2)
	meth=SSLv2_server_method();
#endif

	local_argc=argc;
	local_argv=argv;

	apps_startup();
#ifdef MONOLITH
	s_server_init();
#endif

	if (bio_err == NULL)
		bio_err=BIO_new_fp(stderr,BIO_NOCLOSE);

	if (!load_config(bio_err, NULL))
		goto end;

	verify_depth=0;
#ifdef FIONBIO
	s_nbio=0;
#endif
	s_nbio_test=0;

	argc--;
	argv++;

	while (argc >= 1)
		{
		if	((strcmp(*argv,"-port") == 0) ||
			 (strcmp(*argv,"-accept") == 0))
			{
			if (--argc < 1) goto bad;
			if (!extract_port(*(++argv),&port))
				goto bad;
			}
		else if	(strcmp(*argv,"-verify") == 0)
			{
			s_server_verify=SSL_VERIFY_PEER|SSL_VERIFY_CLIENT_ONCE;
			if (--argc < 1) goto bad;
			verify_depth=atoi(*(++argv));
			BIO_printf(bio_err,"verify depth is %d\n",verify_depth);
			}
		else if	(strcmp(*argv,"-Verify") == 0)
			{
			s_server_verify=SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT|
				SSL_VERIFY_CLIENT_ONCE;
			if (--argc < 1) goto bad;
			verify_depth=atoi(*(++argv));
			BIO_printf(bio_err,"verify depth is %d, must return a certificate\n",verify_depth);
			}
		else if	(strcmp(*argv,"-context") == 0)
			{
			if (--argc < 1) goto bad;
			context= (unsigned char *)*(++argv);
			}
		else if	(strcmp(*argv,"-cert") == 0)
			{
			if (--argc < 1) goto bad;
			s_cert_file= *(++argv);
			}
		else if	(strcmp(*argv,"-certform") == 0)
			{
			if (--argc < 1) goto bad;
			s_cert_format = str2fmt(*(++argv));
			}
		else if	(strcmp(*argv,"-key") == 0)
			{
			if (--argc < 1) goto bad;
			s_key_file= *(++argv);
			}
		else if	(strcmp(*argv,"-keyform") == 0)
			{
			if (--argc < 1) goto bad;
			s_key_format = str2fmt(*(++argv));
			}
		else if	(strcmp(*argv,"-pass") == 0)
			{
			if (--argc < 1) goto bad;
			passarg = *(++argv);
			}
		else if	(strcmp(*argv,"-dhparam") == 0)
			{
			if (--argc < 1) goto bad;
			dhfile = *(++argv);
			}
#ifndef OPENSSL_NO_ECDH		
		else if	(strcmp(*argv,"-named_curve") == 0)
			{
			if (--argc < 1) goto bad;
			named_curve = *(++argv);
			}
#endif
		else if	(strcmp(*argv,"-dcertform") == 0)
			{
			if (--argc < 1) goto bad;
			s_dcert_format = str2fmt(*(++argv));
			}
		else if	(strcmp(*argv,"-dcert") == 0)
			{
			if (--argc < 1) goto bad;
			s_dcert_file= *(++argv);
			}
		else if	(strcmp(*argv,"-dkeyform") == 0)
			{
			if (--argc < 1) goto bad;
			s_dkey_format = str2fmt(*(++argv));
			}
		else if	(strcmp(*argv,"-dpass") == 0)
			{
			if (--argc < 1) goto bad;
			dpassarg = *(++argv);
			}
		else if	(strcmp(*argv,"-dkey") == 0)
			{
			if (--argc < 1) goto bad;
			s_dkey_file= *(++argv);
			}
		else if (strcmp(*argv,"-nocert") == 0)
			{
			nocert=1;
			}
		else if	(strcmp(*argv,"-CApath") == 0)
			{
			if (--argc < 1) goto bad;
			CApath= *(++argv);
			}
		else if (strcmp(*argv,"-no_cache") == 0)
			no_cache = 1;
		else if (strcmp(*argv,"-ext_cache") == 0)
			ext_cache = 1;
		else if (args_verify(&argv, &argc, &badarg, bio_err, &vpm))
			{
			if (badarg)
				goto bad;
			continue;
			}
		else if (strcmp(*argv,"-verify_return_error") == 0)
			verify_return_error = 1;
		else if	(strcmp(*argv,"-serverpref") == 0)
			{ off|=SSL_OP_CIPHER_SERVER_PREFERENCE; }
		else if (strcmp(*argv,"-legacy_renegotiation") == 0)
			off|=SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION;
		else if	(strcmp(*argv,"-cipher") == 0)
			{
			if (--argc < 1) goto bad;
			cipher= *(++argv);
			}
		else if	(strcmp(*argv,"-CAfile") == 0)
			{
			if (--argc < 1) goto bad;
			CAfile= *(++argv);
			}
#ifdef FIONBIO	
		else if	(strcmp(*argv,"-nbio") == 0)
			{ s_nbio=1; }
#endif
		else if	(strcmp(*argv,"-nbio_test") == 0)
			{
#ifdef FIONBIO	
			s_nbio=1;
#endif
			s_nbio_test=1;
			}
		else if	(strcmp(*argv,"-debug") == 0)
			{ s_debug=1; }
#ifndef OPENSSL_NO_TLSEXT
		else if	(strcmp(*argv,"-tlsextdebug") == 0)
			s_tlsextdebug=1;
		else if	(strcmp(*argv,"-status") == 0)
			s_tlsextstatus=1;
		else if	(strcmp(*argv,"-status_verbose") == 0)
			{
			s_tlsextstatus=1;
			tlscstatp.verbose = 1;
			}
		else if (!strcmp(*argv, "-status_timeout"))
			{
			s_tlsextstatus=1;
                        if (--argc < 1) goto bad;
			tlscstatp.timeout = atoi(*(++argv));
			}
		else if (!strcmp(*argv, "-status_url"))
			{
			s_tlsextstatus=1;
                        if (--argc < 1) goto bad;
			if (!OCSP_parse_url(*(++argv),
					&tlscstatp.host,
					&tlscstatp.port,
					&tlscstatp.path,
					&tlscstatp.use_ssl))
				{
				BIO_printf(bio_err, "Error parsing URL\n");
				goto bad;
				}
			}
#endif
		else if	(strcmp(*argv,"-msg") == 0)
			{ s_msg=1; }
		else if	(strcmp(*argv,"-hack") == 0)
			{ hack=1; }
		else if	(strcmp(*argv,"-state") == 0)
			{ state=1; }
		else if	(strcmp(*argv,"-crlf") == 0)
			{ s_crlf=1; }
		else if	(strcmp(*argv,"-quiet") == 0)
			{ s_quiet=1; }
		else if	(strcmp(*argv,"-bugs") == 0)
			{ bugs=1; }
		else if	(strcmp(*argv,"-no_tmp_rsa") == 0)
			{ no_tmp_rsa=1; }
		else if	(strcmp(*argv,"-no_dhe") == 0)
			{ no_dhe=1; }
		else if	(strcmp(*argv,"-no_ecdhe") == 0)
			{ no_ecdhe=1; }
#ifndef OPENSSL_NO_PSK
                else if (strcmp(*argv,"-psk_hint") == 0)
			{
                        if (--argc < 1) goto bad;
                        psk_identity_hint= *(++argv);
                        }
                else if (strcmp(*argv,"-psk") == 0)
			{
			size_t i;

			if (--argc < 1) goto bad;
			psk_key=*(++argv);
			for (i=0; i<strlen(psk_key); i++)
				{
				if (isxdigit((int)psk_key[i]))
					continue;
				BIO_printf(bio_err,"Not a hex number '%s'\n",*argv);
				goto bad;
				}
			}
#endif
		else if	(strcmp(*argv,"-www") == 0)
			{ www=1; }
		else if	(strcmp(*argv,"-WWW") == 0)
			{ www=2; }
		else if	(strcmp(*argv,"-HTTP") == 0)
			{ www=3; }
		else if	(strcmp(*argv,"-no_ssl2") == 0)
			{ off|=SSL_OP_NO_SSLv2; }
		else if	(strcmp(*argv,"-no_ssl3") == 0)
			{ off|=SSL_OP_NO_SSLv3; }
		else if	(strcmp(*argv,"-no_tls1_1") == 0)
			{ off|=SSL_OP_NO_TLSv1_1; }
		else if	(strcmp(*argv,"-no_tls1") == 0)
			{ off|=SSL_OP_NO_TLSv1; }
		else if	(strcmp(*argv,"-no_comp") == 0)
			{ off|=SSL_OP_NO_COMPRESSION; }
#ifndef OPENSSL_NO_TLSEXT
		else if	(strcmp(*argv,"-no_ticket") == 0)
			{ off|=SSL_OP_NO_TICKET; }
#endif
#ifndef OPENSSL_NO_SSL2
		else if	(strcmp(*argv,"-ssl2") == 0)
			{ meth=SSLv2_server_method(); }
#endif
#ifndef OPENSSL_NO_SSL3
		else if	(strcmp(*argv,"-ssl3") == 0)
			{ meth=SSLv3_server_method(); }
#endif
#ifndef OPENSSL_NO_TLS1
		else if	(strcmp(*argv,"-tls1_1") == 0)
			{ meth=TLSv1_1_server_method(); }
		else if	(strcmp(*argv,"-tls1") == 0)
			{ meth=TLSv1_server_method(); }
#endif
#ifndef OPENSSL_NO_DTLS1
		else if	(strcmp(*argv,"-dtls1") == 0)
			{ 
			meth=DTLSv1_server_method();
			socket_type = SOCK_DGRAM;
			}
		else if (strcmp(*argv,"-timeout") == 0)
			enable_timeouts = 1;
		else if (strcmp(*argv,"-mtu") == 0)
			{
			if (--argc < 1) goto bad;
			socket_mtu = atol(*(++argv));
			}
		else if (strcmp(*argv, "-chain") == 0)
			cert_chain = 1;
#endif
		else if (strcmp(*argv, "-id_prefix") == 0)
			{
			if (--argc < 1) goto bad;
			session_id_prefix = *(++argv);
			}
#ifndef OPENSSL_NO_ENGINE
		else if (strcmp(*argv,"-engine") == 0)
			{
			if (--argc < 1) goto bad;
			engine_id= *(++argv);
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
			tlsextcbp.servername= *(++argv);
			}
		else if (strcmp(*argv,"-servername_fatal") == 0)
			{ tlsextcbp.extension_error = SSL_TLSEXT_ERR_ALERT_FATAL; }
		else if	(strcmp(*argv,"-cert2") == 0)
			{
			if (--argc < 1) goto bad;
			s_cert_file2= *(++argv);
			}
		else if	(strcmp(*argv,"-key2") == 0)
			{
			if (--argc < 1) goto bad;
			s_key_file2= *(++argv);
			}
			
#endif
#if !defined(OPENSSL_NO_JPAKE) && !defined(OPENSSL_NO_PSK)
		else if (strcmp(*argv,"-jpake") == 0)
			{
			if (--argc < 1) goto bad;
			jpake_secret = *(++argv);
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
		sv_usage();
		goto end;
		}

#if !defined(OPENSSL_NO_JPAKE) && !defined(OPENSSL_NO_PSK)
	if (jpake_secret)
		{
		if (psk_key)
			{
			BIO_printf(bio_err,
				   "Can't use JPAKE and PSK together\n");
			goto end;
			}
		psk_identity = "JPAKE";
		if (cipher)
			{
			BIO_printf(bio_err, "JPAKE sets cipher to PSK\n");
			goto end;
			}
		cipher = "PSK";
		}

#endif

	SSL_load_error_strings();
	OpenSSL_add_ssl_algorithms();

#ifndef OPENSSL_NO_ENGINE
        e = setup_engine(bio_err, engine_id, 1);
#endif

	if (!app_passwd(bio_err, passarg, dpassarg, &pass, &dpass))
		{
		BIO_printf(bio_err, "Error getting password\n");
		goto end;
		}


	if (s_key_file == NULL)
		s_key_file = s_cert_file;
#ifndef OPENSSL_NO_TLSEXT
	if (s_key_file2 == NULL)
		s_key_file2 = s_cert_file2;
#endif

	if (nocert == 0)
		{
		s_key = load_key(bio_err, s_key_file, s_key_format, 0, pass, e,
		       "server certificate private key file");
		if (!s_key)
			{
			ERR_print_errors(bio_err);
			goto end;
			}

		s_cert = load_cert(bio_err,s_cert_file,s_cert_format,
			NULL, e, "server certificate file");

		if (!s_cert)
			{
			ERR_print_errors(bio_err);
			goto end;
			}

#ifndef OPENSSL_NO_TLSEXT
		if (tlsextcbp.servername) 
			{
			s_key2 = load_key(bio_err, s_key_file2, s_key_format, 0, pass, e,
				"second server certificate private key file");
			if (!s_key2)
				{
				ERR_print_errors(bio_err);
				goto end;
				}
			
			s_cert2 = load_cert(bio_err,s_cert_file2,s_cert_format,
				NULL, e, "second server certificate file");
			
			if (!s_cert2)
				{
				ERR_print_errors(bio_err);
				goto end;
				}
			}
#endif
		}


	if (s_dcert_file)
		{

		if (s_dkey_file == NULL)
			s_dkey_file = s_dcert_file;

		s_dkey = load_key(bio_err, s_dkey_file, s_dkey_format,
				0, dpass, e,
			       "second certificate private key file");
		if (!s_dkey)
			{
			ERR_print_errors(bio_err);
			goto end;
			}

		s_dcert = load_cert(bio_err,s_dcert_file,s_dcert_format,
				NULL, e, "second server certificate file");

		if (!s_dcert)
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

	if (bio_s_out == NULL)
		{
		if (s_quiet && !s_debug && !s_msg)
			{
			bio_s_out=BIO_new(BIO_s_null());
			}
		else
			{
			if (bio_s_out == NULL)
				bio_s_out=BIO_new_fp(stdout,BIO_NOCLOSE);
			}
		}

#if !defined(OPENSSL_NO_RSA) || !defined(OPENSSL_NO_DSA) || !defined(OPENSSL_NO_ECDSA)
	if (nocert)
#endif
		{
		s_cert_file=NULL;
		s_key_file=NULL;
		s_dcert_file=NULL;
		s_dkey_file=NULL;
#ifndef OPENSSL_NO_TLSEXT
		s_cert_file2=NULL;
		s_key_file2=NULL;
#endif
		}

	ctx=SSL_CTX_new(meth);
	if (ctx == NULL)
		{
		ERR_print_errors(bio_err);
		goto end;
		}
	if (session_id_prefix)
		{
		if(strlen(session_id_prefix) >= 32)
			BIO_printf(bio_err,
"warning: id_prefix is too long, only one new session will be possible\n");
		else if(strlen(session_id_prefix) >= 16)
			BIO_printf(bio_err,
"warning: id_prefix is too long if you use SSLv2\n");
		if(!SSL_CTX_set_generate_session_id(ctx, generate_session_id))
			{
			BIO_printf(bio_err,"error setting 'id_prefix'\n");
			ERR_print_errors(bio_err);
			goto end;
			}
		BIO_printf(bio_err,"id_prefix '%s' set.\n", session_id_prefix);
		}
	SSL_CTX_set_quiet_shutdown(ctx,1);
	if (bugs) SSL_CTX_set_options(ctx,SSL_OP_ALL);
	if (hack) SSL_CTX_set_options(ctx,SSL_OP_NETSCAPE_DEMO_CIPHER_CHANGE_BUG);
	SSL_CTX_set_options(ctx,off);
	/* DTLS: partial reads end up discarding unread UDP bytes :-( 
	 * Setting read ahead solves this problem.
	 */
	if (socket_type == SOCK_DGRAM) SSL_CTX_set_read_ahead(ctx, 1);

	if (state) SSL_CTX_set_info_callback(ctx,apps_ssl_info_callback);
	if (no_cache)
		SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_OFF);
	else if (ext_cache)
		init_session_cache_ctx(ctx);
	else
		SSL_CTX_sess_set_cache_size(ctx,128);

#if 0
	if (cipher == NULL) cipher=getenv("SSL_CIPHER");
#endif

#if 0
	if (s_cert_file == NULL)
		{
		BIO_printf(bio_err,"You must specify a certificate file for the server to use\n");
		goto end;
		}
#endif

	if ((!SSL_CTX_load_verify_locations(ctx,CAfile,CApath)) ||
		(!SSL_CTX_set_default_verify_paths(ctx)))
		{
		/* BIO_printf(bio_err,"X509_load_verify_locations\n"); */
		ERR_print_errors(bio_err);
		/* goto end; */
		}
	if (vpm)
		SSL_CTX_set1_param(ctx, vpm);

#ifndef OPENSSL_NO_TLSEXT
	if (s_cert2)
		{
		ctx2=SSL_CTX_new(meth);
		if (ctx2 == NULL)
			{
			ERR_print_errors(bio_err);
			goto end;
			}
		}
	
	if (ctx2)
		{
		BIO_printf(bio_s_out,"Setting secondary ctx parameters\n");

		if (session_id_prefix)
			{
			if(strlen(session_id_prefix) >= 32)
				BIO_printf(bio_err,
					"warning: id_prefix is too long, only one new session will be possible\n");
			else if(strlen(session_id_prefix) >= 16)
				BIO_printf(bio_err,
					"warning: id_prefix is too long if you use SSLv2\n");
			if(!SSL_CTX_set_generate_session_id(ctx2, generate_session_id))
				{
				BIO_printf(bio_err,"error setting 'id_prefix'\n");
				ERR_print_errors(bio_err);
				goto end;
				}
			BIO_printf(bio_err,"id_prefix '%s' set.\n", session_id_prefix);
			}
		SSL_CTX_set_quiet_shutdown(ctx2,1);
		if (bugs) SSL_CTX_set_options(ctx2,SSL_OP_ALL);
		if (hack) SSL_CTX_set_options(ctx2,SSL_OP_NETSCAPE_DEMO_CIPHER_CHANGE_BUG);
		SSL_CTX_set_options(ctx2,off);
		/* DTLS: partial reads end up discarding unread UDP bytes :-( 
		 * Setting read ahead solves this problem.
		 */
		if (socket_type == SOCK_DGRAM) SSL_CTX_set_read_ahead(ctx2, 1);

		if (state) SSL_CTX_set_info_callback(ctx2,apps_ssl_info_callback);

		if (no_cache)
			SSL_CTX_set_session_cache_mode(ctx2,SSL_SESS_CACHE_OFF);
		else if (ext_cache)
			init_session_cache_ctx(ctx2);
		else
			SSL_CTX_sess_set_cache_size(ctx2,128);

		if ((!SSL_CTX_load_verify_locations(ctx2,CAfile,CApath)) ||
			(!SSL_CTX_set_default_verify_paths(ctx2)))
			{
			ERR_print_errors(bio_err);
			}
		if (vpm)
			SSL_CTX_set1_param(ctx2, vpm);
		}
#endif 

#ifndef OPENSSL_NO_DH
	if (!no_dhe)
		{
		DH *dh=NULL;

		if (dhfile)
			dh = load_dh_param(dhfile);
		else if (s_cert_file)
			dh = load_dh_param(s_cert_file);

		if (dh != NULL)
			{
			BIO_printf(bio_s_out,"Setting temp DH parameters\n");
			}
		else
			{
			BIO_printf(bio_s_out,"Using default temp DH parameters\n");
			dh=get_dh512();
			}
		(void)BIO_flush(bio_s_out);

		SSL_CTX_set_tmp_dh(ctx,dh);
#ifndef OPENSSL_NO_TLSEXT
		if (ctx2)
			{
			if (!dhfile)
				{ 
				DH *dh2=load_dh_param(s_cert_file2);
				if (dh2 != NULL)
					{
					BIO_printf(bio_s_out,"Setting temp DH parameters\n");
					(void)BIO_flush(bio_s_out);

					DH_free(dh);
					dh = dh2;
					}
				}
			SSL_CTX_set_tmp_dh(ctx2,dh);
			}
#endif
		DH_free(dh);
		}
#endif

#ifndef OPENSSL_NO_ECDH
	if (!no_ecdhe)
		{
		EC_KEY *ecdh=NULL;

		if (named_curve)
			{
			int nid = OBJ_sn2nid(named_curve);

			if (nid == 0)
				{
				BIO_printf(bio_err, "unknown curve name (%s)\n", 
					named_curve);
				goto end;
				}
			ecdh = EC_KEY_new_by_curve_name(nid);
			if (ecdh == NULL)
				{
				BIO_printf(bio_err, "unable to create curve (%s)\n", 
					named_curve);
				goto end;
				}
			}

		if (ecdh != NULL)
			{
			BIO_printf(bio_s_out,"Setting temp ECDH parameters\n");
			}
		else
			{
			BIO_printf(bio_s_out,"Using default temp ECDH parameters\n");
			ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
			if (ecdh == NULL) 
				{
				BIO_printf(bio_err, "unable to create curve (nistp256)\n");
				goto end;
				}
			}
		(void)BIO_flush(bio_s_out);

		SSL_CTX_set_tmp_ecdh(ctx,ecdh);
#ifndef OPENSSL_NO_TLSEXT
		if (ctx2) 
			SSL_CTX_set_tmp_ecdh(ctx2,ecdh);
#endif
		EC_KEY_free(ecdh);
		}
#endif
	
	if (!set_cert_key_stuff(ctx,s_cert,s_key))
		goto end;
#ifndef OPENSSL_NO_TLSEXT
	if (ctx2 && !set_cert_key_stuff(ctx2,s_cert2,s_key2))
		goto end; 
#endif
	if (s_dcert != NULL)
		{
		if (!set_cert_key_stuff(ctx,s_dcert,s_dkey))
			goto end;
		}

#ifndef OPENSSL_NO_RSA
#if 1
	if (!no_tmp_rsa)
		{
		SSL_CTX_set_tmp_rsa_callback(ctx,tmp_rsa_cb);
#ifndef OPENSSL_NO_TLSEXT
		if (ctx2) 
			SSL_CTX_set_tmp_rsa_callback(ctx2,tmp_rsa_cb);
#endif		
		}
#else
	if (!no_tmp_rsa && SSL_CTX_need_tmp_RSA(ctx))
		{
		RSA *rsa;

		BIO_printf(bio_s_out,"Generating temp (512 bit) RSA key...");
		BIO_flush(bio_s_out);

		rsa=RSA_generate_key(512,RSA_F4,NULL);

		if (!SSL_CTX_set_tmp_rsa(ctx,rsa))
			{
			ERR_print_errors(bio_err);
			goto end;
			}
#ifndef OPENSSL_NO_TLSEXT
			if (ctx2)
				{
				if (!SSL_CTX_set_tmp_rsa(ctx2,rsa))
					{
					ERR_print_errors(bio_err);
					goto end;
					}
				}
#endif
		RSA_free(rsa);
		BIO_printf(bio_s_out,"\n");
		}
#endif
#endif

#ifndef OPENSSL_NO_PSK
#ifdef OPENSSL_NO_JPAKE
	if (psk_key != NULL)
#else
	if (psk_key != NULL || jpake_secret)
#endif
		{
		if (s_debug)
			BIO_printf(bio_s_out, "PSK key given or JPAKE in use, setting server callback\n");
		SSL_CTX_set_psk_server_callback(ctx, psk_server_cb);
		}

	if (!SSL_CTX_use_psk_identity_hint(ctx, psk_identity_hint))
		{
		BIO_printf(bio_err,"error setting PSK identity hint to context\n");
		ERR_print_errors(bio_err);
		goto end;
		}
#endif

	if (cipher != NULL)
		{
		if(!SSL_CTX_set_cipher_list(ctx,cipher))
			{
			BIO_printf(bio_err,"error setting cipher list\n");
			ERR_print_errors(bio_err);
			goto end;
			}
#ifndef OPENSSL_NO_TLSEXT
		if (ctx2 && !SSL_CTX_set_cipher_list(ctx2,cipher))
			{
			BIO_printf(bio_err,"error setting cipher list\n");
			ERR_print_errors(bio_err);
			goto end;
			}
#endif
		}
	SSL_CTX_set_verify(ctx,s_server_verify,verify_callback);
	SSL_CTX_set_session_id_context(ctx,(void*)&s_server_session_id_context,
		sizeof s_server_session_id_context);

	/* Set DTLS cookie generation and verification callbacks */
	SSL_CTX_set_cookie_generate_cb(ctx, generate_cookie_callback);
	SSL_CTX_set_cookie_verify_cb(ctx, verify_cookie_callback);

#ifndef OPENSSL_NO_TLSEXT
	if (ctx2)
		{
		SSL_CTX_set_verify(ctx2,s_server_verify,verify_callback);
		SSL_CTX_set_session_id_context(ctx2,(void*)&s_server_session_id_context,
			sizeof s_server_session_id_context);

		tlsextcbp.biodebug = bio_s_out;
		SSL_CTX_set_tlsext_servername_callback(ctx2, ssl_servername_cb);
		SSL_CTX_set_tlsext_servername_arg(ctx2, &tlsextcbp);
		SSL_CTX_set_tlsext_servername_callback(ctx, ssl_servername_cb);
		SSL_CTX_set_tlsext_servername_arg(ctx, &tlsextcbp);
		}
#endif

	if (CAfile != NULL)
		{
		SSL_CTX_set_client_CA_list(ctx,SSL_load_client_CA_file(CAfile));
#ifndef OPENSSL_NO_TLSEXT
		if (ctx2) 
			SSL_CTX_set_client_CA_list(ctx2,SSL_load_client_CA_file(CAfile));
#endif
		}

	BIO_printf(bio_s_out,"ACCEPT\n");
	(void)BIO_flush(bio_s_out);
	if (www)
		do_server(port,socket_type,&accept_socket,www_body, context);
	else
		do_server(port,socket_type,&accept_socket,sv_body, context);
	print_stats(bio_s_out,ctx);
	ret=0;
end:
	if (ctx != NULL) SSL_CTX_free(ctx);
	if (s_cert)
		X509_free(s_cert);
	if (s_dcert)
		X509_free(s_dcert);
	if (s_key)
		EVP_PKEY_free(s_key);
	if (s_dkey)
		EVP_PKEY_free(s_dkey);
	if (pass)
		OPENSSL_free(pass);
	if (dpass)
		OPENSSL_free(dpass);
	free_sessions();
#ifndef OPENSSL_NO_TLSEXT
	if (ctx2 != NULL) SSL_CTX_free(ctx2);
	if (s_cert2)
		X509_free(s_cert2);
	if (s_key2)
		EVP_PKEY_free(s_key2);
#endif
	if (bio_s_out != NULL)
		{
        BIO_free(bio_s_out);
		bio_s_out=NULL;
		}
	apps_shutdown();
	OPENSSL_EXIT(ret);
	}