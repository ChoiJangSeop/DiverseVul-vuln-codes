static int aesni_cbc_hmac_sha1_cipher(EVP_CIPHER_CTX *ctx, unsigned char *out,
		      const unsigned char *in, size_t len)
	{
	EVP_AES_HMAC_SHA1 *key = data(ctx);
	unsigned int l;
	size_t	plen = key->payload_length,
		iv = 0,		/* explicit IV in TLS 1.1 and later */
		sha_off = 0;
#if defined(STITCHED_CALL)
	size_t	aes_off = 0,
		blocks;

	sha_off = SHA_CBLOCK-key->md.num;
#endif

	if (len%AES_BLOCK_SIZE) return 0;

	if (ctx->encrypt) {
		if (plen==NO_PAYLOAD_LENGTH)
			plen = len;
		else if (len!=((plen+SHA_DIGEST_LENGTH+AES_BLOCK_SIZE)&-AES_BLOCK_SIZE))
			return 0;
		else if (key->aux.tls_ver >= TLS1_1_VERSION)
			iv = AES_BLOCK_SIZE;

#if defined(STITCHED_CALL)
		if (plen>(sha_off+iv) && (blocks=(plen-(sha_off+iv))/SHA_CBLOCK)) {
			SHA1_Update(&key->md,in+iv,sha_off);

			aesni_cbc_sha1_enc(in,out,blocks,&key->ks,
				ctx->iv,&key->md,in+iv+sha_off);
			blocks *= SHA_CBLOCK;
			aes_off += blocks;
			sha_off += blocks;
			key->md.Nh += blocks>>29;
			key->md.Nl += blocks<<=3;
			if (key->md.Nl<(unsigned int)blocks) key->md.Nh++;
		} else {
			sha_off = 0;
		}
#endif
		sha_off += iv;
		SHA1_Update(&key->md,in+sha_off,plen-sha_off);

		if (plen!=len)	{	/* "TLS" mode of operation */
			if (in!=out)
				memcpy(out+aes_off,in+aes_off,plen-aes_off);

			/* calculate HMAC and append it to payload */
			SHA1_Final(out+plen,&key->md);
			key->md = key->tail;
			SHA1_Update(&key->md,out+plen,SHA_DIGEST_LENGTH);
			SHA1_Final(out+plen,&key->md);

			/* pad the payload|hmac */
			plen += SHA_DIGEST_LENGTH;
			for (l=len-plen-1;plen<len;plen++) out[plen]=l;
			/* encrypt HMAC|padding at once */
			aesni_cbc_encrypt(out+aes_off,out+aes_off,len-aes_off,
					&key->ks,ctx->iv,1);
		} else {
			aesni_cbc_encrypt(in+aes_off,out+aes_off,len-aes_off,
					&key->ks,ctx->iv,1);
		}
	} else {
		unsigned char mac[SHA_DIGEST_LENGTH];

		/* decrypt HMAC|padding at once */
		aesni_cbc_encrypt(in,out,len,
				&key->ks,ctx->iv,0);

		if (plen) {	/* "TLS" mode of operation */
			/* figure out payload length */
			if (len<(size_t)(out[len-1]+1+SHA_DIGEST_LENGTH))
				return 0;

			len -= (out[len-1]+1+SHA_DIGEST_LENGTH);

			if ((key->aux.tls_aad[plen-4]<<8|key->aux.tls_aad[plen-3])
			    >= TLS1_1_VERSION) {
				len -= AES_BLOCK_SIZE;
				iv = AES_BLOCK_SIZE;
			}

			key->aux.tls_aad[plen-2] = len>>8;
			key->aux.tls_aad[plen-1] = len;

			/* calculate HMAC and verify it */
			key->md = key->head;
			SHA1_Update(&key->md,key->aux.tls_aad,plen);
			SHA1_Update(&key->md,out+iv,len);
			SHA1_Final(mac,&key->md);

			key->md = key->tail;
			SHA1_Update(&key->md,mac,SHA_DIGEST_LENGTH);
			SHA1_Final(mac,&key->md);

			if (memcmp(out+iv+len,mac,SHA_DIGEST_LENGTH))
				return 0;
		} else {
			SHA1_Update(&key->md,out,len);
		}
	}

	key->payload_length = NO_PAYLOAD_LENGTH;

	return 1;
	}