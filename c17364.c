static void zlib_stateful_free_ex_data(void *obj, void *item,
	CRYPTO_EX_DATA *ad, int ind,long argl, void *argp)
	{
	struct zlib_state *state = (struct zlib_state *)item;
	inflateEnd(&state->istream);
	deflateEnd(&state->ostream);
	OPENSSL_free(state);
	}