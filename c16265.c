static void bnprint(FILE *out, const char *name, const BIGNUM *b)
	{
	unsigned char *tmp;
	int len;
	len = BN_num_bytes(b);
	tmp = OPENSSL_malloc(len);
	BN_bn2bin(b, tmp);
	fprintf(out, "%s\n", name);
	hexprint(out, tmp, len);
	OPENSSL_free(tmp);
	}