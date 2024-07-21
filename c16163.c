int RSA_padding_check_PKCS1_OAEP(unsigned char *to, int tlen,
	const unsigned char *from, int flen, int num,
	const unsigned char *param, int plen)
	{
	int i, dblen, mlen = -1;
	const unsigned char *maskeddb;
	int lzero;
	unsigned char *db = NULL, seed[SHA_DIGEST_LENGTH], phash[SHA_DIGEST_LENGTH];
	unsigned char *padded_from;
	int bad = 0;

	if (--num < 2 * SHA_DIGEST_LENGTH + 1)
		/* 'num' is the length of the modulus, i.e. does not depend on the
		 * particular ciphertext. */
		goto decoding_err;

	lzero = num - flen;
	if (lzero < 0)
		{
		/* signalling this error immediately after detection might allow
		 * for side-channel attacks (e.g. timing if 'plen' is huge
		 * -- cf. James H. Manger, "A Chosen Ciphertext Attack on RSA Optimal
		 * Asymmetric Encryption Padding (OAEP) [...]", CRYPTO 2001),
		 * so we use a 'bad' flag */
		bad = 1;
		lzero = 0;
		flen = num; /* don't overflow the memcpy to padded_from */
		}

	dblen = num - SHA_DIGEST_LENGTH;
	db = OPENSSL_malloc(dblen + num);
	if (db == NULL)
		{
		RSAerr(RSA_F_RSA_PADDING_CHECK_PKCS1_OAEP, ERR_R_MALLOC_FAILURE);
		return -1;
		}

	/* Always do this zero-padding copy (even when lzero == 0)
	 * to avoid leaking timing info about the value of lzero. */
	padded_from = db + dblen;
	memset(padded_from, 0, lzero);
	memcpy(padded_from + lzero, from, flen);

	maskeddb = padded_from + SHA_DIGEST_LENGTH;

	MGF1(seed, SHA_DIGEST_LENGTH, maskeddb, dblen);
	for (i = 0; i < SHA_DIGEST_LENGTH; i++)
		seed[i] ^= padded_from[i];
  
	MGF1(db, dblen, seed, SHA_DIGEST_LENGTH);
	for (i = 0; i < dblen; i++)
		db[i] ^= maskeddb[i];

	EVP_Digest((void *)param, plen, phash, NULL, EVP_sha1(), NULL);

	if (memcmp(db, phash, SHA_DIGEST_LENGTH) != 0 || bad)
		goto decoding_err;
	else
		{
		for (i = SHA_DIGEST_LENGTH; i < dblen; i++)
			if (db[i] != 0x00)
				break;
		if (i == dblen || db[i] != 0x01)
			goto decoding_err;
		else
			{
			/* everything looks OK */

			mlen = dblen - ++i;
			if (tlen < mlen)
				{
				RSAerr(RSA_F_RSA_PADDING_CHECK_PKCS1_OAEP, RSA_R_DATA_TOO_LARGE);
				mlen = -1;
				}
			else
				memcpy(to, db + i, mlen);
			}
		}
	OPENSSL_free(db);
	return mlen;

decoding_err:
	/* to avoid chosen ciphertext attacks, the error message should not reveal
	 * which kind of decoding error happened */
	RSAerr(RSA_F_RSA_PADDING_CHECK_PKCS1_OAEP, RSA_R_OAEP_DECODING_ERROR);
	if (db != NULL) OPENSSL_free(db);
	return -1;
	}