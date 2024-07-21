int ssl3_mac(SSL *ssl, unsigned char *md, int send)
	{
	SSL3_RECORD *rec;
	unsigned char *mac_sec,*seq;
	EVP_MD_CTX md_ctx;
	const EVP_MD *hash;
	unsigned char *p,rec_char;
	size_t md_size;
	int npad;

	if (send)
		{
		rec= &(ssl->s3->wrec);
		mac_sec= &(ssl->s3->write_mac_secret[0]);
		seq= &(ssl->s3->write_sequence[0]);
		hash=ssl->write_hash;
		}
	else
		{
		rec= &(ssl->s3->rrec);
		mac_sec= &(ssl->s3->read_mac_secret[0]);
		seq= &(ssl->s3->read_sequence[0]);
		hash=ssl->read_hash;
		}

	md_size=EVP_MD_size(hash);
	npad=(48/md_size)*md_size;

	if (!send &&
	    EVP_CIPHER_CTX_mode(ssl->enc_read_ctx) == EVP_CIPH_CBC_MODE &&
	    ssl3_cbc_record_digest_supported(hash))
		{
		/* This is a CBC-encrypted record. We must avoid leaking any
		 * timing-side channel information about how many blocks of
		 * data we are hashing because that gives an attacker a
		 * timing-oracle. */

		/* npad is, at most, 48 bytes and that's with MD5:
		 *   16 + 48 + 8 (sequence bytes) + 1 + 2 = 75.
		 *
		 * With SHA-1 (the largest hash speced for SSLv3) the hash size
		 * goes up 4, but npad goes down by 8, resulting in a smaller
		 * total size. */
		unsigned char header[75];
		unsigned j = 0;
		memcpy(header+j, mac_sec, md_size);
		j += md_size;
		memcpy(header+j, ssl3_pad_1, npad);
		j += npad;
		memcpy(header+j, seq, 8);
		j += 8;
		header[j++] = rec->type;
		header[j++] = rec->length >> 8;
		header[j++] = rec->length & 0xff;

		ssl3_cbc_digest_record(
			hash,
			md, &md_size,
			header, rec->input,
			rec->length + md_size, rec->orig_len,
			mac_sec, md_size,
			1 /* is SSLv3 */);
		}
	else
		{
		unsigned int md_size_u;
		/* Chop the digest off the end :-) */
		EVP_MD_CTX_init(&md_ctx);

		EVP_MD_CTX_copy_ex( &md_ctx,hash);
		EVP_DigestUpdate(&md_ctx,mac_sec,md_size);
		EVP_DigestUpdate(&md_ctx,ssl3_pad_1,npad);
		EVP_DigestUpdate(&md_ctx,seq,8);
		rec_char=rec->type;
		EVP_DigestUpdate(&md_ctx,&rec_char,1);
		p=md;
		s2n(rec->length,p);
		EVP_DigestUpdate(&md_ctx,md,2);
		EVP_DigestUpdate(&md_ctx,rec->input,rec->length);
		EVP_DigestFinal_ex( &md_ctx,md,NULL);

		EVP_MD_CTX_copy_ex( &md_ctx,hash);
		EVP_DigestUpdate(&md_ctx,mac_sec,md_size);
		EVP_DigestUpdate(&md_ctx,ssl3_pad_2,npad);
		EVP_DigestUpdate(&md_ctx,md,md_size);
		EVP_DigestFinal_ex( &md_ctx,md,&md_size_u);
		md_size = md_size_u;

		EVP_MD_CTX_cleanup(&md_ctx);
	}

	ssl3_record_sequence_update(seq);
	return(md_size);
	}