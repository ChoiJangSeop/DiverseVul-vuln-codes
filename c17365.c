COMP_METHOD *COMP_zlib(void)
	{
	COMP_METHOD *meth = &zlib_method_nozlib;

#ifdef ZLIB_SHARED
	if (!zlib_loaded)
		{
#if defined(OPENSSL_SYS_WINDOWS) || defined(OPENSSL_SYS_WIN32)
		zlib_dso = DSO_load(NULL, "ZLIB1", NULL, 0);
#else
		zlib_dso = DSO_load(NULL, "z", NULL, 0);
#endif
		if (zlib_dso != NULL)
			{
			p_compress
				= (compress_ft) DSO_bind_func(zlib_dso,
					"compress");
			p_inflateEnd
				= (inflateEnd_ft) DSO_bind_func(zlib_dso,
					"inflateEnd");
			p_inflate
				= (inflate_ft) DSO_bind_func(zlib_dso,
					"inflate");
			p_inflateInit_
				= (inflateInit__ft) DSO_bind_func(zlib_dso,
					"inflateInit_");
			p_deflateEnd
				= (deflateEnd_ft) DSO_bind_func(zlib_dso,
					"deflateEnd");
			p_deflate
				= (deflate_ft) DSO_bind_func(zlib_dso,
					"deflate");
			p_deflateInit_
				= (deflateInit__ft) DSO_bind_func(zlib_dso,
					"deflateInit_");
			p_zError
				= (zError__ft) DSO_bind_func(zlib_dso,
					"zError");

			if (p_compress && p_inflateEnd && p_inflate
				&& p_inflateInit_ && p_deflateEnd
				&& p_deflate && p_deflateInit_ && p_zError)
				zlib_loaded++;
			}
		}

#endif
#ifdef ZLIB_SHARED
	if (zlib_loaded)
#endif
#if defined(ZLIB) || defined(ZLIB_SHARED)
		{
		/* init zlib_stateful_ex_idx here so that in a multi-process
		 * application it's enough to intialize openssl before forking
		 * (idx will be inherited in all the children) */
		if (zlib_stateful_ex_idx == -1)
			{
			CRYPTO_w_lock(CRYPTO_LOCK_COMP);
			if (zlib_stateful_ex_idx == -1)
				zlib_stateful_ex_idx =
					CRYPTO_get_ex_new_index(CRYPTO_EX_INDEX_COMP,
						0,NULL,NULL,NULL,zlib_stateful_free_ex_data);
			CRYPTO_w_unlock(CRYPTO_LOCK_COMP);
			if (zlib_stateful_ex_idx == -1)
				goto err;
			}
		
		meth = &zlib_stateful_method;
		}
err:	
#endif

	return(meth);
	}