u32 gf_base64_encode(const u8 *_in, u32 inSize, u8 *_out, u32 outSize)
{
	s32 padding;
	u32 i = 0, j = 0;
	unsigned char *in = (unsigned char *)_in;
	unsigned char *out = (unsigned char *)_out;

	if (outSize < (inSize * 4 / 3)) return 0;

	while (i < inSize) {
		padding = 3 - (inSize - i);
		if (padding == 2) {
			out[j] = base_64[in[i]>>2];
			out[j+1] = base_64[(in[i] & 0x03) << 4];
			out[j+2] = '=';
			out[j+3] = '=';
		} else if (padding == 1) {
			out[j] = base_64[in[i]>>2];
			out[j+1] = base_64[((in[i] & 0x03) << 4) | ((in[i+1] & 0xf0) >> 4)];
			out[j+2] = base_64[(in[i+1] & 0x0f) << 2];
			out[j+3] = '=';
		} else {
			out[j] = base_64[in[i]>>2];
			out[j+1] = base_64[((in[i] & 0x03) << 4) | ((in[i+1] & 0xf0) >> 4)];
			out[j+2] = base_64[((in[i+1] & 0x0f) << 2) | ((in[i+2] & 0xc0) >> 6)];
			out[j+3] = base_64[in[i+2] & 0x3f];
		}
		i += 3;
		j += 4;
	}
	return j;
}