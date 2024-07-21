int main(int argc, char *argv[])
	{
	char *CApath=NULL,*CAfile=NULL;
	int badop=0;
	int bio_pair=0;
	int force=0;
	int tls1=0,ssl2=0,ssl3=0,ret=1;
	int client_auth=0;
	int server_auth=0,i;
	struct app_verify_arg app_verify_arg =
		{ APP_CALLBACK_STRING, 0, 0, NULL, NULL };
	char *server_cert=TEST_SERVER_CERT;
	char *server_key=NULL;
	char *client_cert=TEST_CLIENT_CERT;
	char *client_key=NULL;
#ifndef OPENSSL_NO_ECDH
	char *named_curve = NULL;
#endif
	SSL_CTX *s_ctx=NULL;
	SSL_CTX *c_ctx=NULL;
	const SSL_METHOD *meth=NULL;
	SSL *c_ssl,*s_ssl;
	int number=1,reuse=0;
	long bytes=256L;
#ifndef OPENSSL_NO_DH
	DH *dh;
	int dhe1024 = 0, dhe1024dsa = 0;
#endif
#ifndef OPENSSL_NO_ECDH
	EC_KEY *ecdh = NULL;
#endif
	int no_dhe = 0;
	int no_ecdhe = 0;
	int no_psk = 0;
	int print_time = 0;
	clock_t s_time = 0, c_time = 0;
	int comp = 0;
#ifndef OPENSSL_NO_COMP
	COMP_METHOD *cm = NULL;
	STACK_OF(SSL_COMP) *ssl_comp_methods = NULL;
#endif
	int test_cipherlist = 0;

	verbose = 0;
	debug = 0;
	cipher = 0;

	bio_err=BIO_new_fp(stderr,BIO_NOCLOSE|BIO_FP_TEXT);	

	CRYPTO_set_locking_callback(lock_dbg_cb);

	/* enable memory leak checking unless explicitly disabled */
	if (!((getenv("OPENSSL_DEBUG_MEMORY") != NULL) && (0 == strcmp(getenv("OPENSSL_DEBUG_MEMORY"), "off"))))
		{
		CRYPTO_malloc_debug_init();
		CRYPTO_set_mem_debug_options(V_CRYPTO_MDEBUG_ALL);
		}
	else
		{
		/* OPENSSL_DEBUG_MEMORY=off */
		CRYPTO_set_mem_debug_functions(0, 0, 0, 0, 0);
		}
	CRYPTO_mem_ctrl(CRYPTO_MEM_CHECK_ON);

	RAND_seed(rnd_seed, sizeof rnd_seed);

	bio_stdout=BIO_new_fp(stdout,BIO_NOCLOSE|BIO_FP_TEXT);

	argc--;
	argv++;

	while (argc >= 1)
		{
		if	(strcmp(*argv,"-server_auth") == 0)
			server_auth=1;
		else if	(strcmp(*argv,"-client_auth") == 0)
			client_auth=1;
		else if (strcmp(*argv,"-proxy_auth") == 0)
			{
			if (--argc < 1) goto bad;
			app_verify_arg.proxy_auth= *(++argv);
			}
		else if (strcmp(*argv,"-proxy_cond") == 0)
			{
			if (--argc < 1) goto bad;
			app_verify_arg.proxy_cond= *(++argv);
			}
		else if	(strcmp(*argv,"-v") == 0)
			verbose=1;
		else if	(strcmp(*argv,"-d") == 0)
			debug=1;
		else if	(strcmp(*argv,"-reuse") == 0)
			reuse=1;
		else if	(strcmp(*argv,"-dhe1024") == 0)
			{
#ifndef OPENSSL_NO_DH
			dhe1024=1;
#else
			fprintf(stderr,"ignoring -dhe1024, since I'm compiled without DH\n");
#endif
			}
		else if	(strcmp(*argv,"-dhe1024dsa") == 0)
			{
#ifndef OPENSSL_NO_DH
			dhe1024dsa=1;
#else
			fprintf(stderr,"ignoring -dhe1024, since I'm compiled without DH\n");
#endif
			}
		else if	(strcmp(*argv,"-no_dhe") == 0)
			no_dhe=1;
		else if	(strcmp(*argv,"-no_ecdhe") == 0)
			no_ecdhe=1;
		else if (strcmp(*argv,"-psk") == 0)
			{
			if (--argc < 1) goto bad;
			psk_key=*(++argv);
#ifndef OPENSSL_NO_PSK
			if (strspn(psk_key, "abcdefABCDEF1234567890") != strlen(psk_key))
				{
				BIO_printf(bio_err,"Not a hex number '%s'\n",*argv);
				goto bad;
				}
#else
			no_psk=1;
#endif
			}
		else if	(strcmp(*argv,"-ssl2") == 0)
			ssl2=1;
		else if	(strcmp(*argv,"-tls1") == 0)
			tls1=1;
		else if	(strcmp(*argv,"-ssl3") == 0)
			ssl3=1;
		else if	(strncmp(*argv,"-num",4) == 0)
			{
			if (--argc < 1) goto bad;
			number= atoi(*(++argv));
			if (number == 0) number=1;
			}
		else if	(strcmp(*argv,"-bytes") == 0)
			{
			if (--argc < 1) goto bad;
			bytes= atol(*(++argv));
			if (bytes == 0L) bytes=1L;
			i=strlen(argv[0]);
			if (argv[0][i-1] == 'k') bytes*=1024L;
			if (argv[0][i-1] == 'm') bytes*=1024L*1024L;
			}
		else if	(strcmp(*argv,"-cert") == 0)
			{
			if (--argc < 1) goto bad;
			server_cert= *(++argv);
			}
		else if	(strcmp(*argv,"-s_cert") == 0)
			{
			if (--argc < 1) goto bad;
			server_cert= *(++argv);
			}
		else if	(strcmp(*argv,"-key") == 0)
			{
			if (--argc < 1) goto bad;
			server_key= *(++argv);
			}
		else if	(strcmp(*argv,"-s_key") == 0)
			{
			if (--argc < 1) goto bad;
			server_key= *(++argv);
			}
		else if	(strcmp(*argv,"-c_cert") == 0)
			{
			if (--argc < 1) goto bad;
			client_cert= *(++argv);
			}
		else if	(strcmp(*argv,"-c_key") == 0)
			{
			if (--argc < 1) goto bad;
			client_key= *(++argv);
			}
		else if	(strcmp(*argv,"-cipher") == 0)
			{
			if (--argc < 1) goto bad;
			cipher= *(++argv);
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
		else if	(strcmp(*argv,"-bio_pair") == 0)
			{
			bio_pair = 1;
			}
		else if	(strcmp(*argv,"-f") == 0)
			{
			force = 1;
			}
		else if	(strcmp(*argv,"-time") == 0)
			{
			print_time = 1;
			}
		else if	(strcmp(*argv,"-zlib") == 0)
			{
			comp = COMP_ZLIB;
			}
		else if	(strcmp(*argv,"-rle") == 0)
			{
			comp = COMP_RLE;
			}
		else if	(strcmp(*argv,"-named_curve") == 0)
			{
			if (--argc < 1) goto bad;
#ifndef OPENSSL_NO_ECDH		
			named_curve = *(++argv);
#else
			fprintf(stderr,"ignoring -named_curve, since I'm compiled without ECDH\n");
			++argv;
#endif
			}
		else if	(strcmp(*argv,"-app_verify") == 0)
			{
			app_verify_arg.app_verify = 1;
			}
		else if	(strcmp(*argv,"-proxy") == 0)
			{
			app_verify_arg.allow_proxy_certs = 1;
			}
		else if (strcmp(*argv,"-test_cipherlist") == 0)
			{
			test_cipherlist = 1;
			}
#ifndef OPENSSL_NO_NPN
		else if (strcmp(*argv,"-npn_client") == 0)
			{
			npn_client = 1;
			}
		else if (strcmp(*argv,"-npn_server") == 0)
			{
			npn_server = 1;
			}
		else if (strcmp(*argv,"-npn_server_reject") == 0)
			{
			npn_server_reject = 1;
			}
#endif
		else
			{
			fprintf(stderr,"unknown option %s\n",*argv);
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

	if (test_cipherlist == 1)
		{
		/* ensure that the cipher list are correctly sorted and exit */
		if (do_test_cipherlist() == 0)
			EXIT(1);
		ret = 0;
		goto end;
		}

	if (!ssl2 && !ssl3 && !tls1 && number > 1 && !reuse && !force)
		{
		fprintf(stderr, "This case cannot work.  Use -f to perform "
			"the test anyway (and\n-d to see what happens), "
			"or add one of -ssl2, -ssl3, -tls1, -reuse\n"
			"to avoid protocol mismatch.\n");
		EXIT(1);
		}

	if (print_time)
		{
		if (!bio_pair)
			{
			fprintf(stderr, "Using BIO pair (-bio_pair)\n");
			bio_pair = 1;
			}
		if (number < 50 && !force)
			fprintf(stderr, "Warning: For accurate timings, use more connections (e.g. -num 1000)\n");
		}

/*	if (cipher == NULL) cipher=getenv("SSL_CIPHER"); */

	SSL_library_init();
	SSL_load_error_strings();

#ifndef OPENSSL_NO_COMP
	if (comp == COMP_ZLIB) cm = COMP_zlib();
	if (comp == COMP_RLE) cm = COMP_rle();
	if (cm != NULL)
		{
		if (cm->type != NID_undef)
			{
			if (SSL_COMP_add_compression_method(comp, cm) != 0)
				{
				fprintf(stderr,
					"Failed to add compression method\n");
				ERR_print_errors_fp(stderr);
				}
			}
		else
			{
			fprintf(stderr,
				"Warning: %s compression not supported\n",
				(comp == COMP_RLE ? "rle" :
					(comp == COMP_ZLIB ? "zlib" :
						"unknown")));
			ERR_print_errors_fp(stderr);
			}
		}
	ssl_comp_methods = SSL_COMP_get_compression_methods();
	fprintf(stderr, "Available compression methods:\n");
	{
	int j, n = sk_SSL_COMP_num(ssl_comp_methods);
	if (n == 0)
		fprintf(stderr, "  NONE\n");
	else
		for (j = 0; j < n; j++)
			{
			SSL_COMP *c = sk_SSL_COMP_value(ssl_comp_methods, j);
			fprintf(stderr, "  %d: %s\n", c->id, c->name);
			}
	}
#endif

#if !defined(OPENSSL_NO_SSL2) && !defined(OPENSSL_NO_SSL3)
	if (ssl2)
		meth=SSLv2_method();
	else 
	if (tls1)
		meth=TLSv1_method();
	else
	if (ssl3)
		meth=SSLv3_method();
	else
		meth=SSLv23_method();
#else
#ifdef OPENSSL_NO_SSL2
	meth=SSLv3_method();
#else
	meth=SSLv2_method();
#endif
#endif

	c_ctx=SSL_CTX_new(meth);
	s_ctx=SSL_CTX_new(meth);
	if ((c_ctx == NULL) || (s_ctx == NULL))
		{
		ERR_print_errors(bio_err);
		goto end;
		}

	if (cipher != NULL)
		{
		SSL_CTX_set_cipher_list(c_ctx,cipher);
		SSL_CTX_set_cipher_list(s_ctx,cipher);
		}

#ifndef OPENSSL_NO_DH
	if (!no_dhe)
		{
		if (dhe1024dsa)
			{
			/* use SSL_OP_SINGLE_DH_USE to avoid small subgroup attacks */
			SSL_CTX_set_options(s_ctx, SSL_OP_SINGLE_DH_USE);
			dh=get_dh1024dsa();
			}
		else if (dhe1024)
			dh=get_dh1024();
		else
			dh=get_dh512();
		SSL_CTX_set_tmp_dh(s_ctx,dh);
		DH_free(dh);
		}
#else
	(void)no_dhe;
#endif

#ifndef OPENSSL_NO_ECDH
	if (!no_ecdhe)
		{
		int nid;

		if (named_curve != NULL)
			{
			nid = OBJ_sn2nid(named_curve);
			if (nid == 0)
			{
				BIO_printf(bio_err, "unknown curve name (%s)\n", named_curve);
				goto end;
				}
			}
		else
#ifdef OPENSSL_NO_EC2M
			nid = NID_X9_62_prime256v1;
#else
			nid = NID_sect163r2;
#endif

		ecdh = EC_KEY_new_by_curve_name(nid);
		if (ecdh == NULL)
			{
			BIO_printf(bio_err, "unable to create curve\n");
			goto end;
			}

		SSL_CTX_set_tmp_ecdh(s_ctx, ecdh);
		SSL_CTX_set_options(s_ctx, SSL_OP_SINGLE_ECDH_USE);
		EC_KEY_free(ecdh);
		}
#else
	(void)no_ecdhe;
#endif

#ifndef OPENSSL_NO_RSA
	SSL_CTX_set_tmp_rsa_callback(s_ctx,tmp_rsa_cb);
#endif

#ifdef TLSEXT_TYPE_opaque_prf_input
	SSL_CTX_set_tlsext_opaque_prf_input_callback(c_ctx, opaque_prf_input_cb);
	SSL_CTX_set_tlsext_opaque_prf_input_callback(s_ctx, opaque_prf_input_cb);
	SSL_CTX_set_tlsext_opaque_prf_input_callback_arg(c_ctx, &co1); /* or &co2 or NULL */
	SSL_CTX_set_tlsext_opaque_prf_input_callback_arg(s_ctx, &so1); /* or &so2 or NULL */
#endif

	if (!SSL_CTX_use_certificate_file(s_ctx,server_cert,SSL_FILETYPE_PEM))
		{
		ERR_print_errors(bio_err);
		}
	else if (!SSL_CTX_use_PrivateKey_file(s_ctx,
		(server_key?server_key:server_cert), SSL_FILETYPE_PEM))
		{
		ERR_print_errors(bio_err);
		goto end;
		}

	if (client_auth)
		{
		SSL_CTX_use_certificate_file(c_ctx,client_cert,
			SSL_FILETYPE_PEM);
		SSL_CTX_use_PrivateKey_file(c_ctx,
			(client_key?client_key:client_cert),
			SSL_FILETYPE_PEM);
		}

	if (	(!SSL_CTX_load_verify_locations(s_ctx,CAfile,CApath)) ||
		(!SSL_CTX_set_default_verify_paths(s_ctx)) ||
		(!SSL_CTX_load_verify_locations(c_ctx,CAfile,CApath)) ||
		(!SSL_CTX_set_default_verify_paths(c_ctx)))
		{
		/* fprintf(stderr,"SSL_load_verify_locations\n"); */
		ERR_print_errors(bio_err);
		/* goto end; */
		}

	if (client_auth)
		{
		BIO_printf(bio_err,"client authentication\n");
		SSL_CTX_set_verify(s_ctx,
			SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
			verify_callback);
		SSL_CTX_set_cert_verify_callback(s_ctx, app_verify_callback, &app_verify_arg);
		}
	if (server_auth)
		{
		BIO_printf(bio_err,"server authentication\n");
		SSL_CTX_set_verify(c_ctx,SSL_VERIFY_PEER,
			verify_callback);
		SSL_CTX_set_cert_verify_callback(c_ctx, app_verify_callback, &app_verify_arg);
		}
	
	{
		int session_id_context = 0;
		SSL_CTX_set_session_id_context(s_ctx, (void *)&session_id_context, sizeof session_id_context);
	}

	/* Use PSK only if PSK key is given */
	if (psk_key != NULL)
		{
		/* no_psk is used to avoid putting psk command to openssl tool */
		if (no_psk)
			{
			/* if PSK is not compiled in and psk key is
			 * given, do nothing and exit successfully */
			ret=0;
			goto end;
			}
#ifndef OPENSSL_NO_PSK
		SSL_CTX_set_psk_client_callback(c_ctx, psk_client_callback);
		SSL_CTX_set_psk_server_callback(s_ctx, psk_server_callback);
		if (debug)
			BIO_printf(bio_err,"setting PSK identity hint to s_ctx\n");
		if (!SSL_CTX_use_psk_identity_hint(s_ctx, "ctx server identity_hint"))
			{
			BIO_printf(bio_err,"error setting PSK identity hint to s_ctx\n");
			ERR_print_errors(bio_err);
			goto end;
			}
#endif
		}

#ifndef OPENSSL_NO_NPN
	if (npn_client)
		{
		SSL_CTX_set_next_proto_select_cb(c_ctx, cb_client_npn, NULL);
		}
	if (npn_server)
		{
		if (npn_server_reject)
			{
			BIO_printf(bio_err, "Can't have both -npn_server and -npn_server_reject\n");
			goto end;
			}
		SSL_CTX_set_next_protos_advertised_cb(s_ctx, cb_server_npn, NULL);
		}
	if (npn_server_reject)
		{
		SSL_CTX_set_next_protos_advertised_cb(s_ctx, cb_server_rejects_npn, NULL);
		}
#endif

	c_ssl=SSL_new(c_ctx);
	s_ssl=SSL_new(s_ctx);

#ifndef OPENSSL_NO_KRB5
	if (c_ssl  &&  c_ssl->kssl_ctx)
                {
                char	localhost[MAXHOSTNAMELEN+2];

		if (gethostname(localhost, sizeof localhost-1) == 0)
                        {
			localhost[sizeof localhost-1]='\0';
			if(strlen(localhost) == sizeof localhost-1)
				{
				BIO_printf(bio_err,"localhost name too long\n");
				goto end;
				}
			kssl_ctx_setstring(c_ssl->kssl_ctx, KSSL_SERVER,
                                localhost);
			}
		}
#endif    /* OPENSSL_NO_KRB5  */

	for (i=0; i<number; i++)
		{
		if (!reuse) SSL_set_session(c_ssl,NULL);
		if (bio_pair)
			ret=doit_biopair(s_ssl,c_ssl,bytes,&s_time,&c_time);
		else
			ret=doit(s_ssl,c_ssl,bytes);
		}

	if (!verbose)
		{
		print_details(c_ssl, "");
		}
	if ((number > 1) || (bytes > 1L))
		BIO_printf(bio_stdout, "%d handshakes of %ld bytes done\n",number,bytes);
	if (print_time)
		{
#ifdef CLOCKS_PER_SEC
		/* "To determine the time in seconds, the value returned
		 * by the clock function should be divided by the value
		 * of the macro CLOCKS_PER_SEC."
		 *                                       -- ISO/IEC 9899 */
		BIO_printf(bio_stdout, "Approximate total server time: %6.2f s\n"
			"Approximate total client time: %6.2f s\n",
			(double)s_time/CLOCKS_PER_SEC,
			(double)c_time/CLOCKS_PER_SEC);
#else
		/* "`CLOCKS_PER_SEC' undeclared (first use this function)"
		 *                            -- cc on NeXTstep/OpenStep */
		BIO_printf(bio_stdout,
			"Approximate total server time: %6.2f units\n"
			"Approximate total client time: %6.2f units\n",
			(double)s_time,
			(double)c_time);
#endif
		}

	SSL_free(s_ssl);
	SSL_free(c_ssl);

end:
	if (s_ctx != NULL) SSL_CTX_free(s_ctx);
	if (c_ctx != NULL) SSL_CTX_free(c_ctx);

	if (bio_stdout != NULL) BIO_free(bio_stdout);

#ifndef OPENSSL_NO_RSA
	free_tmp_rsa();
#endif
#ifndef OPENSSL_NO_ENGINE
	ENGINE_cleanup();
#endif
	CRYPTO_cleanup_all_ex_data();
	ERR_free_strings();
	ERR_remove_thread_state(NULL);
	EVP_cleanup();
	CRYPTO_mem_leaks(bio_err);
	if (bio_err != NULL) BIO_free(bio_err);
	EXIT(ret);
	return ret;
	}