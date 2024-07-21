int gnutls_global_init(void)
{
	int ret = 0, res;
	int level;
	const char* e;
	
	GNUTLS_STATIC_MUTEX_LOCK(global_init_mutex);

	_gnutls_init++;
	if (_gnutls_init > 1) {
		if (_gnutls_init == 2 && _gnutls_init_ret == 0) {
			/* some applications may close the urandom fd 
			 * before calling gnutls_global_init(). in that
			 * case reopen it */
			ret = _gnutls_rnd_check();
			if (ret < 0) {
				gnutls_assert();
				goto out;
			}
		}
		ret = _gnutls_init_ret;
		goto out;
	}

	_gnutls_switch_lib_state(LIB_STATE_INIT);

	e = getenv("GNUTLS_DEBUG_LEVEL");
	if (e != NULL) {
		level = atoi(e);
		gnutls_global_set_log_level(level);
		if (_gnutls_log_func == NULL)
			gnutls_global_set_log_function(default_log_func);
		_gnutls_debug_log("Enabled GnuTLS "VERSION" logging...\n");
	}

	bindtextdomain(PACKAGE, LOCALEDIR);

	res = gnutls_crypto_init();
	if (res != 0) {
		gnutls_assert();
		ret = GNUTLS_E_CRYPTO_INIT_FAILED;
		goto out;
	}

	/* initialize ASN.1 parser
	 */
	if (asn1_check_version(GNUTLS_MIN_LIBTASN1_VERSION) == NULL) {
		gnutls_assert();
		_gnutls_debug_log
		    ("Checking for libtasn1 failed: %s < %s\n",
		     asn1_check_version(NULL),
		     GNUTLS_MIN_LIBTASN1_VERSION);
		ret = GNUTLS_E_INCOMPATIBLE_LIBTASN1_LIBRARY;
		goto out;
	}

	_gnutls_pkix1_asn = ASN1_TYPE_EMPTY;
	res = asn1_array2tree(pkix_asn1_tab, &_gnutls_pkix1_asn, NULL);
	if (res != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(res);
		goto out;
	}

	res = asn1_array2tree(gnutls_asn1_tab, &_gnutls_gnutls_asn, NULL);
	if (res != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(res);
		goto out;
	}

	/* Initialize the random generator */
	ret = _gnutls_rnd_init();
	if (ret < 0) {
		gnutls_assert();
		goto out;
	}

	/* Initialize the default TLS extensions */
	ret = _gnutls_ext_init();
	if (ret < 0) {
		gnutls_assert();
		goto out;
	}

	ret = gnutls_mutex_init(&_gnutls_file_mutex);
	if (ret < 0) {
		gnutls_assert();
		goto out;
	}

	ret = gnutls_mutex_init(&_gnutls_pkcs11_mutex);
	if (ret < 0) {
		gnutls_assert();
		goto out;
	}

	ret = gnutls_system_global_init();
	if (ret < 0) {
		gnutls_assert();
		goto out;
	}

#ifdef ENABLE_FIPS140
	res = _gnutls_fips_mode_enabled();
	/* res == 1 -> fips140-2 mode enabled
	 * res == 2 -> only self checks performed - but no failure
	 * res == not in fips140 mode
	 */
	if (res != 0) {
		_gnutls_debug_log("FIPS140-2 mode: %d\n", res);
		_gnutls_priority_update_fips();

		/* first round of self checks, these are done on the
		 * nettle algorithms which are used internally */
		ret = _gnutls_fips_perform_self_checks1();
		if (res != 2) {
			if (ret < 0) {
				gnutls_assert();
				goto out;
			}
		}
	}
#endif

	_gnutls_register_accel_crypto();
	_gnutls_cryptodev_init();

#ifdef ENABLE_FIPS140
	/* These self tests are performed on the overriden algorithms
	 * (e.g., AESNI overriden AES). They are after _gnutls_register_accel_crypto()
	 * intentionally */
	if (res != 0) {
		ret = _gnutls_fips_perform_self_checks2();
		if (res != 2) {
			if (ret < 0) {
				gnutls_assert();
				goto out;
			}
		}
		_gnutls_fips_mode_reset_zombie();
	}
#endif
	_gnutls_switch_lib_state(LIB_STATE_OPERATIONAL);
	ret = 0;

      out:
	_gnutls_init_ret = ret;
	GNUTLS_STATIC_MUTEX_UNLOCK(global_init_mutex);
	return ret;
}