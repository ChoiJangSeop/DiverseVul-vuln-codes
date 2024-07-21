SPH_XCAT(sph_, HASH)(void *cc, const void *data, size_t len)
{
	SPH_XCAT(sph_, SPH_XCAT(HASH, _context)) *sc;
	unsigned current;
	size_t orig_len;
#if !SPH_64
	sph_u32 clow, clow2;
#endif

	if (len < (2 * SPH_BLEN)) {
		SPH_XCAT(HASH, _short)(cc, data, len);
		return;
	}
	sc = cc;
#if SPH_64
	current = (unsigned)sc->count & (SPH_BLEN - 1U);
#else
	current = (unsigned)sc->count_low & (SPH_BLEN - 1U);
#endif
	if (current > 0) {
		unsigned t;

		t = SPH_BLEN - current;
		SPH_XCAT(HASH, _short)(cc, data, t);
		data = (const unsigned char *)data + t;
		len -= t;
	}
#if !SPH_UNALIGNED
	if (((SPH_UPTR)data & (SPH_WLEN - 1U)) != 0) {
		SPH_XCAT(HASH, _short)(cc, data, len);
		return;
	}
#endif
	orig_len = len;
	while (len >= SPH_BLEN) {
		RFUN(data, SPH_VAL);
		len -= SPH_BLEN;
		data = (const unsigned char *)data + SPH_BLEN;
	}
	if (len > 0)
		memcpy(sc->buf, data, len);
#if SPH_64
	sc->count += (sph_u64)orig_len;
#else
	clow = sc->count_low;
	clow2 = SPH_T32(clow + orig_len);
	sc->count_low = clow2;
	if (clow2 < clow)
		sc->count_high ++;
	/*
	 * This code handles the improbable situation where "size_t" is
	 * greater than 32 bits, and yet we do not have a 64-bit type.
	 */
	orig_len >>= 12;
	orig_len >>= 10;
	orig_len >>= 10;
	sc->count_high += orig_len;
#endif
}