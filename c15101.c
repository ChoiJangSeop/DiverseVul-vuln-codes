SPH_XCAT(sph_, HASH)(void *cc, const void *data, size_t len)
#endif
{
	SPH_XCAT(sph_, SPH_XCAT(HASH, _context)) *sc;
	unsigned current;

	sc = cc;
#if SPH_64
	current = (unsigned)sc->count & (SPH_BLEN - 1U);
#else
	current = (unsigned)sc->count_low & (SPH_BLEN - 1U);
#endif
	while (len > 0) {
		unsigned clen;
#if !SPH_64
		sph_u32 clow, clow2;
#endif

		clen = SPH_BLEN - current;
		if (clen > len)
			clen = len;
		memcpy(sc->buf + current, data, clen);
		data = (const unsigned char *)data + clen;
		current += clen;
		len -= clen;
		if (current == SPH_BLEN) {
			RFUN(sc->buf, SPH_VAL);
			current = 0;
		}
#if SPH_64
		sc->count += clen;
#else
		clow = sc->count_low;
		clow2 = SPH_T32(clow + clen);
		sc->count_low = clow2;
		if (clow2 < clow)
			sc->count_high ++;
#endif
	}
}