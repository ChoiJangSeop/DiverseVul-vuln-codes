PHP_HASH_API void PHP_HAVAL256Final(unsigned char *digest, PHP_HAVAL_CTX * context)
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
	Encode(digest, context->state, 32);

	/* Zeroize sensitive information.
	 */
	memset((unsigned char*) context, 0, sizeof(*context));
}