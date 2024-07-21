int dane_state_init(dane_state_t* s, unsigned int flags)
{
	struct ub_ctx* ctx;
	int ret;

	*s = calloc(1, sizeof(struct dane_state_st));
	if (*s == NULL)
		return gnutls_assert_val(DANE_E_MEMORY_ERROR);

	ctx = ub_ctx_create();
	if(!ctx) {
                gnutls_assert();
		ret = DANE_E_INITIALIZATION_ERROR;
		goto cleanup;
	}
	ub_ctx_debugout(ctx, stderr);

	if (!(flags & DANE_F_IGNORE_LOCAL_RESOLVER)) {
		if( (ret=ub_ctx_resolvconf(ctx, NULL)) != 0) {
		        gnutls_assert();
			ret = DANE_E_INITIALIZATION_ERROR;
			goto cleanup;
		}

		if( (ret=ub_ctx_hosts(ctx, NULL)) != 0) {
		        gnutls_assert();
			ret = DANE_E_INITIALIZATION_ERROR;
			goto cleanup;
		}
	}

	/* read public keys for DNSSEC verification */
	if( (ret=ub_ctx_add_ta_file(ctx, (char*)UNBOUND_ROOT_KEY_FILE)) != 0) {
	        gnutls_assert();
		ret = DANE_E_INITIALIZATION_ERROR;
		goto cleanup;
	}

	(*s)->ctx = ctx;
	(*s)->flags = flags;
	
	return DANE_E_SUCCESS;
cleanup:

	if (ctx)
		ub_ctx_delete(ctx);
	free(*s);
	
	return ret;
}