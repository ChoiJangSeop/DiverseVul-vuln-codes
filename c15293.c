sph_enc16be(void *dst, unsigned val)
{
	((unsigned char *)dst)[0] = (val >> 8);
	((unsigned char *)dst)[1] = val;
}