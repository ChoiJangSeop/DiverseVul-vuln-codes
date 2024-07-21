PHP_HASH_API void PHP_HAVAL160Final(unsigned char *digest, PHP_HAVAL_CTX * context)
{
	unsigned char bits[10];
	unsigned int index, padLen;

	/* Version, Passes, and Digest Length */
	bits[0] =	(PHP_HASH_HAVAL_VERSION & 0x07) |
				((context->passes & 0x07) << 3) |
				((context->output & 0x03) << 6);
	bits[1] = (context->output >> 2);

	/* Save number of bits */
	Encode(bits + 2, context->count, 8);

	/* Pad out to 118 mod 128.
	 */
	index = (unsigned int) ((context->count[0] >> 3) & 0x3f);
	padLen = (index < 118) ? (118 - index) : (246 - index);
	PHP_HAVALUpdate(context, PADDING, padLen);

	/* Append version, passes, digest length, and message length */
	PHP_HAVALUpdate(context, bits, 10);

	/* Store state in digest */
	context->state[4] += ((context->state[7] & 0xFE000000) |
						  (context->state[6] & 0x01F80000) |
						  (context->state[5] & 0x0007F000)) >> 12;

	context->state[3] += ((context->state[7] & 0x01F80000) |
						  (context->state[6] & 0x0007F000) |
						  (context->state[5] & 0x00000FC0)) >> 6;

	context->state[2] +=  (context->state[7] & 0x0007F000) |
						  (context->state[6] & 0x00000FC0) |
						  (context->state[5] & 0x0000003F);

	context->state[1] += ROTR((context->state[7] & 0x00000FC0) |
							  (context->state[6] & 0x0000003F) |
							  (context->state[5] & 0xFE000000), 25);

	context->state[0] += ROTR((context->state[7] & 0x0000003F) |
							  (context->state[6] & 0xFE000000) |
							  (context->state[5] & 0x01F80000), 19);

	Encode(digest, context->state, 20);

	/* Zeroize sensitive information.
	 */
	memset((unsigned char*) context, 0, sizeof(*context));
}