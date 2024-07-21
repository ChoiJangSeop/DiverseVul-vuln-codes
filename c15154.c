sph_enc16le(void *dst, unsigned val)
{
	((unsigned char *)dst)[0] = val;
	((unsigned char *)dst)[1] = val >> 8;
}