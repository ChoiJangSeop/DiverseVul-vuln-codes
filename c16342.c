dtls1_hm_fragment_new(unsigned long frag_len)
	{
	hm_fragment *frag = NULL;
	unsigned char *buf = NULL;

	frag = (hm_fragment *)OPENSSL_malloc(sizeof(hm_fragment));
	if ( frag == NULL)
		return NULL;

	if (frag_len)
		{
		buf = (unsigned char *)OPENSSL_malloc(frag_len);
		if ( buf == NULL)
			{
			OPENSSL_free(frag);
			return NULL;
			}
		}

	/* zero length fragment gets zero frag->fragment */
	frag->fragment = buf;

	return frag;
	}