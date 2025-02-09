BIGNUM *SRP_Calc_u(BIGNUM *A, BIGNUM *B, BIGNUM *N)
	{
	/* k = SHA1(PAD(A) || PAD(B) ) -- tls-srp draft 8 */

	BIGNUM *u;	
	unsigned char cu[SHA_DIGEST_LENGTH];
	unsigned char *cAB;
	EVP_MD_CTX ctxt;
	int longN;  
	if ((A == NULL) ||(B == NULL) || (N == NULL))
		return NULL;

	longN= BN_num_bytes(N);

	if ((cAB = OPENSSL_malloc(2*longN)) == NULL) 
		return NULL;

	memset(cAB, 0, longN);

	EVP_MD_CTX_init(&ctxt);
	EVP_DigestInit_ex(&ctxt, EVP_sha1(), NULL);
	EVP_DigestUpdate(&ctxt, cAB + BN_bn2bin(A,cAB+longN), longN);
	EVP_DigestUpdate(&ctxt, cAB + BN_bn2bin(B,cAB+longN), longN);
	OPENSSL_free(cAB);
	EVP_DigestFinal_ex(&ctxt, cu, NULL);
	EVP_MD_CTX_cleanup(&ctxt);

	if (!(u = BN_bin2bn(cu, sizeof(cu), NULL)))
		return NULL;
	if (!BN_is_zero(u))
		return u;
	BN_free(u);
	return NULL;
}