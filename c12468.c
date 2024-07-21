static int decode_avp(struct l2tp_avp_t *avp, const struct l2tp_attr_t *RV,
		      const char *secret, size_t secret_len)
{
	MD5_CTX md5_ctx;
	uint8_t md5[MD5_DIGEST_LENGTH];
	uint8_t p1[MD5_DIGEST_LENGTH];
	uint8_t *prev_block = NULL;
	uint16_t attr_len;
	uint16_t orig_attr_len;
	uint16_t bytes_left;
	uint16_t blocks_left;
	uint16_t last_block_len;

	if (avp->length < sizeof(struct l2tp_avp_t) + 2) {
		/* Hidden AVPs must contain at least two bytes
		   for storing original attribute length */
		log_warn("l2tp: incorrect hidden avp received (type %hu):"
			 " length too small (%hu bytes)\n",
			 ntohs(avp->type), avp->length);
		return -1;
	}
	attr_len = avp->length - sizeof(struct l2tp_avp_t);

	/* Decode first block */
	MD5_Init(&md5_ctx);
	MD5_Update(&md5_ctx, &avp->type, sizeof(avp->type));
	MD5_Update(&md5_ctx, secret, secret_len);
	MD5_Update(&md5_ctx, RV->val.octets, RV->length);
	MD5_Final(p1, &md5_ctx);

	if (attr_len <= MD5_DIGEST_LENGTH) {
		memxor(avp->val, p1, attr_len);
		return 0;
	}

	memxor(p1, avp->val, MD5_DIGEST_LENGTH);
	orig_attr_len = ntohs(*(uint16_t *)p1);

	if (orig_attr_len <= MD5_DIGEST_LENGTH - 2) {
		/* Enough bytes decoded already, no need to decode padding */
		memcpy(avp->val, p1, MD5_DIGEST_LENGTH);
		return 0;
	}

	if (orig_attr_len > attr_len - 2) {
		log_warn("l2tp: incorrect hidden avp received (type %hu):"
			 " original attribute length too big (ciphered"
			 " attribute length: %hu bytes, advertised original"
			 " attribute length: %hu bytes)\n",
			 ntohs(avp->type), attr_len, orig_attr_len);
		return -1;
	}

	/* Decode remaining blocks. Start from the last block as
	   preceding blocks must be kept hidden for computing MD5s */
	bytes_left = orig_attr_len + 2 - MD5_DIGEST_LENGTH;
	last_block_len = bytes_left % MD5_DIGEST_LENGTH;
	blocks_left = bytes_left / MD5_DIGEST_LENGTH;
	if (last_block_len) {
		prev_block = avp->val + blocks_left * MD5_DIGEST_LENGTH;
		MD5_Init(&md5_ctx);
		MD5_Update(&md5_ctx, secret, secret_len);
		MD5_Update(&md5_ctx, prev_block, MD5_DIGEST_LENGTH);
		MD5_Final(md5, &md5_ctx);
		memxor(prev_block + MD5_DIGEST_LENGTH, md5, last_block_len);
		prev_block -= MD5_DIGEST_LENGTH;
	} else
		prev_block = avp->val + (blocks_left - 1) * MD5_DIGEST_LENGTH;

	while (prev_block >= avp->val) {
		MD5_Init(&md5_ctx);
		MD5_Update(&md5_ctx, secret, secret_len);
		MD5_Update(&md5_ctx, prev_block, MD5_DIGEST_LENGTH);
		MD5_Final(md5, &md5_ctx);
		memxor(prev_block + MD5_DIGEST_LENGTH, md5, MD5_DIGEST_LENGTH);
		prev_block -= MD5_DIGEST_LENGTH;
	}
	memcpy(avp->val, p1, MD5_DIGEST_LENGTH);

	return 0;
}