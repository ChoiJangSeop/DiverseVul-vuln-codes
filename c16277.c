dtls1_hm_fragment_free(hm_fragment *frag)
	{
	if (frag->fragment) OPENSSL_free(frag->fragment);
	if (frag->reassembly) OPENSSL_free(frag->reassembly);
	OPENSSL_free(frag);
	}