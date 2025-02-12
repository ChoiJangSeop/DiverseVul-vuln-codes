static BIGNUM *srp_Calc_k(BIGNUM *N, BIGNUM *g)
	{
	/* k = SHA1(N | PAD(g)) -- tls-srp draft 8 */

	unsigned char digest[SHA_DIGEST_LENGTH];
	unsigned char *tmp;
	EVP_MD_CTX ctxt;
	int longg ;
	int longN = BN_num_bytes(N);

	if ((tmp = OPENSSL_malloc(longN)) == NULL)
		return NULL;
	BN_bn2bin(N,tmp) ;

	EVP_MD_CTX_init(&ctxt);
	EVP_DigestInit_ex(&ctxt, EVP_sha1(), NULL);
	EVP_DigestUpdate(&ctxt, tmp, longN);

	memset(tmp, 0, longN);
	longg = BN_bn2bin(g,tmp) ;
        /* use the zeros behind to pad on left */
	EVP_DigestUpdate(&ctxt, tmp + longg, longN-longg);
	EVP_DigestUpdate(&ctxt, tmp, longg);
	OPENSSL_free(tmp);

	EVP_DigestFinal_ex(&ctxt, digest, NULL);
	EVP_MD_CTX_cleanup(&ctxt);
	return BN_bin2bn(digest, sizeof(digest), NULL);	
	}