SPH_XCAT(HASH, _addbits_and_close)(void *cc,
	unsigned ub, unsigned n, void *dst, unsigned rnum)
{
	SPH_XCAT(sph_, SPH_XCAT(HASH, _context)) *sc;
	unsigned current, u;
#if !SPH_64
	sph_u32 low, high;
#endif

	sc = cc;
#if SPH_64
	current = (unsigned)sc->count & (SPH_BLEN - 1U);
#else
	current = (unsigned)sc->count_low & (SPH_BLEN - 1U);
#endif
#ifdef PW01
	sc->buf[current ++] = (0x100 | (ub & 0xFF)) >> (8 - n);
#else
	{
		unsigned z;

		z = 0x80 >> n;
		sc->buf[current ++] = ((ub & -z) | z) & 0xFF;
	}
#endif
	if (current > SPH_MAXPAD) {
		memset(sc->buf + current, 0, SPH_BLEN - current);
		RFUN(sc->buf, SPH_VAL);
		memset(sc->buf, 0, SPH_MAXPAD);
	} else {
		memset(sc->buf + current, 0, SPH_MAXPAD - current);
	}
#if defined BE64
#if defined PLW1
	sph_enc64be_aligned(sc->buf + SPH_MAXPAD,
		SPH_T64(sc->count << 3) + (sph_u64)n);
#elif defined PLW4
	memset(sc->buf + SPH_MAXPAD, 0, 2 * SPH_WLEN);
	sph_enc64be_aligned(sc->buf + SPH_MAXPAD + 2 * SPH_WLEN,
		sc->count >> 61);
	sph_enc64be_aligned(sc->buf + SPH_MAXPAD + 3 * SPH_WLEN,
		SPH_T64(sc->count << 3) + (sph_u64)n);
#else
	sph_enc64be_aligned(sc->buf + SPH_MAXPAD, sc->count >> 61);
	sph_enc64be_aligned(sc->buf + SPH_MAXPAD + SPH_WLEN,
		SPH_T64(sc->count << 3) + (sph_u64)n);
#endif
#elif defined LE64
#if defined PLW1
	sph_enc64le_aligned(sc->buf + SPH_MAXPAD,
		SPH_T64(sc->count << 3) + (sph_u64)n);
#elif defined PLW1
	sph_enc64le_aligned(sc->buf + SPH_MAXPAD,
		SPH_T64(sc->count << 3) + (sph_u64)n);
	sph_enc64le_aligned(sc->buf + SPH_MAXPAD + SPH_WLEN, sc->count >> 61);
	memset(sc->buf + SPH_MAXPAD + 2 * SPH_WLEN, 0, 2 * SPH_WLEN);
#else
	sph_enc64le_aligned(sc->buf + SPH_MAXPAD,
		SPH_T64(sc->count << 3) + (sph_u64)n);
	sph_enc64le_aligned(sc->buf + SPH_MAXPAD + SPH_WLEN, sc->count >> 61);
#endif
#else
#if SPH_64
#ifdef BE32
	sph_enc64be_aligned(sc->buf + SPH_MAXPAD,
		SPH_T64(sc->count << 3) + (sph_u64)n);
#else
	sph_enc64le_aligned(sc->buf + SPH_MAXPAD,
		SPH_T64(sc->count << 3) + (sph_u64)n);
#endif
#else
	low = sc->count_low;
	high = SPH_T32((sc->count_high << 3) | (low >> 29));
	low = SPH_T32(low << 3) + (sph_u32)n;
#ifdef BE32
	sph_enc32be(sc->buf + SPH_MAXPAD, high);
	sph_enc32be(sc->buf + SPH_MAXPAD + SPH_WLEN, low);
#else
	sph_enc32le(sc->buf + SPH_MAXPAD, low);
	sph_enc32le(sc->buf + SPH_MAXPAD + SPH_WLEN, high);
#endif
#endif
#endif
	RFUN(sc->buf, SPH_VAL);
#ifdef SPH_NO_OUTPUT
	(void)dst;
	(void)rnum;
	(void)u;
#else
	for (u = 0; u < rnum; u ++) {
#if defined BE64
		sph_enc64be((unsigned char *)dst + 8 * u, sc->val[u]);
#elif defined LE64
		sph_enc64le((unsigned char *)dst + 8 * u, sc->val[u]);
#elif defined BE32
		sph_enc32be((unsigned char *)dst + 4 * u, sc->val[u]);
#else
		sph_enc32le((unsigned char *)dst + 4 * u, sc->val[u]);
#endif
	}
#endif
}