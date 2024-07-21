u32 gf_base16_encode(u8 *_in, u32 inSize, u8 *_out, u32 outSize)
{
	u32 i = 0;
	unsigned char *in = (unsigned char *)_in;
	unsigned char *out = (unsigned char *)_out;

	if (outSize < (inSize * 2)+1) return 0;

	for (i=0; i<inSize; i++) {
		out[2*i] = base_16[((in[i] & 0xf0) >> 4)];
		out[2*i+1] = base_16[(in[i] & 0x0f)];
	}
	out[(inSize * 2)] = 0;

	return inSize * 2;
}