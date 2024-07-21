static int bn2binpad(unsigned char *to, size_t tolen, BIGNUM *b)
	{
	size_t blen;
	blen = BN_num_bytes(b);
	/* If BIGNUM length greater than buffer, mask to get rightmost
	 * bytes. NB: modifies b but this doesn't matter for our purposes.
	 */
	if (blen > tolen)
		{
		BN_mask_bits(b, tolen << 3);
		/* Update length because mask operation might create leading
		 * zeroes.
		 */
		blen = BN_num_bytes(b);
		}
	/* If b length smaller than buffer pad with zeroes */
	if (blen < tolen)
		{
		memset(to, 0, tolen - blen);
		to += tolen - blen;
		}

	/* This call cannot fail */
	BN_bn2bin(b, to);
	return 1;
	}