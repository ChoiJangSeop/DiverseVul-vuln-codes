unpack_Z_stream(int fd_in, int fd_out)
{
	IF_DESKTOP(long long total_written = 0;)
	IF_DESKTOP(long long) int retval = -1;
	unsigned char *stackp;
	long code;
	int finchar;
	long oldcode;
	long incode;
	int inbits;
	int posbits;
	int outpos;
	int insize;
	int bitmask;
	long free_ent;
	long maxcode;
	long maxmaxcode;
	int n_bits;
	int rsize = 0;
	unsigned char *inbuf; /* were eating insane amounts of stack - */
	unsigned char *outbuf; /* bad for some embedded targets */
	unsigned char *htab;
	unsigned short *codetab;

	/* Hmm, these were statics - why?! */
	/* user settable max # bits/code */
	int maxbits; /* = BITS; */
	/* block compress mode -C compatible with 2.0 */
	int block_mode; /* = BLOCK_MODE; */

	inbuf = xzalloc(IBUFSIZ + 64);
	outbuf = xzalloc(OBUFSIZ + 2048);
	htab = xzalloc(HSIZE);  /* wsn't zeroed out before, maybe can xmalloc? */
	codetab = xzalloc(HSIZE * sizeof(codetab[0]));

	insize = 0;

	/* xread isn't good here, we have to return - caller may want
	 * to do some cleanup (e.g. delete incomplete unpacked file etc) */
	if (full_read(fd_in, inbuf, 1) != 1) {
		bb_error_msg("short read");
		goto err;
	}

	maxbits = inbuf[0] & BIT_MASK;
	block_mode = inbuf[0] & BLOCK_MODE;
	maxmaxcode = MAXCODE(maxbits);

	if (maxbits > BITS) {
		bb_error_msg("compressed with %d bits, can only handle "
				BITS_STR" bits", maxbits);
		goto err;
	}

	n_bits = INIT_BITS;
	maxcode = MAXCODE(INIT_BITS) - 1;
	bitmask = (1 << INIT_BITS) - 1;
	oldcode = -1;
	finchar = 0;
	outpos = 0;
	posbits = 0 << 3;

	free_ent = ((block_mode) ? FIRST : 256);

	/* As above, initialize the first 256 entries in the table. */
	/*clear_tab_prefixof(); - done by xzalloc */

	for (code = 255; code >= 0; --code) {
		tab_suffixof(code) = (unsigned char) code;
	}

	do {
 resetbuf:
		{
			int i;
			int e;
			int o;

			o = posbits >> 3;
			e = insize - o;

			for (i = 0; i < e; ++i)
				inbuf[i] = inbuf[i + o];

			insize = e;
			posbits = 0;
		}

		if (insize < (int) (IBUFSIZ + 64) - IBUFSIZ) {
			rsize = safe_read(fd_in, inbuf + insize, IBUFSIZ);
//error check??
			insize += rsize;
		}

		inbits = ((rsize > 0) ? (insize - insize % n_bits) << 3 :
				  (insize << 3) - (n_bits - 1));

		while (inbits > posbits) {
			if (free_ent > maxcode) {
				posbits =
					((posbits - 1) +
					 ((n_bits << 3) -
					  (posbits - 1 + (n_bits << 3)) % (n_bits << 3)));
				++n_bits;
				if (n_bits == maxbits) {
					maxcode = maxmaxcode;
				} else {
					maxcode = MAXCODE(n_bits) - 1;
				}
				bitmask = (1 << n_bits) - 1;
				goto resetbuf;
			}
			{
				unsigned char *p = &inbuf[posbits >> 3];

				code = ((((long) (p[0])) | ((long) (p[1]) << 8) |
				         ((long) (p[2]) << 16)) >> (posbits & 0x7)) & bitmask;
			}
			posbits += n_bits;


			if (oldcode == -1) {
				oldcode = code;
				finchar = (int) oldcode;
				outbuf[outpos++] = (unsigned char) finchar;
				continue;
			}

			if (code == CLEAR && block_mode) {
				clear_tab_prefixof();
				free_ent = FIRST - 1;
				posbits =
					((posbits - 1) +
					 ((n_bits << 3) -
					  (posbits - 1 + (n_bits << 3)) % (n_bits << 3)));
				n_bits = INIT_BITS;
				maxcode = MAXCODE(INIT_BITS) - 1;
				bitmask = (1 << INIT_BITS) - 1;
				goto resetbuf;
			}

			incode = code;
			stackp = de_stack;

			/* Special case for KwKwK string. */
			if (code >= free_ent) {
				if (code > free_ent) {
					unsigned char *p;

					posbits -= n_bits;
					p = &inbuf[posbits >> 3];

					bb_error_msg
						("insize:%d posbits:%d inbuf:%02X %02X %02X %02X %02X (%d)",
						 insize, posbits, p[-1], p[0], p[1], p[2], p[3],
						 (posbits & 07));
					bb_error_msg("corrupted data");
					goto err;
				}

				*--stackp = (unsigned char) finchar;
				code = oldcode;
			}

			/* Generate output characters in reverse order */
			while ((long) code >= (long) 256) {
				*--stackp = tab_suffixof(code);
				code = tab_prefixof(code);
			}

			finchar = tab_suffixof(code);
			*--stackp = (unsigned char) finchar;

			/* And put them out in forward order */
			{
				int i;

				i = de_stack - stackp;
				if (outpos + i >= OBUFSIZ) {
					do {
						if (i > OBUFSIZ - outpos) {
							i = OBUFSIZ - outpos;
						}

						if (i > 0) {
							memcpy(outbuf + outpos, stackp, i);
							outpos += i;
						}

						if (outpos >= OBUFSIZ) {
							full_write(fd_out, outbuf, outpos);
//error check??
							IF_DESKTOP(total_written += outpos;)
							outpos = 0;
						}
						stackp += i;
						i = de_stack - stackp;
					} while (i > 0);
				} else {
					memcpy(outbuf + outpos, stackp, i);
					outpos += i;
				}
			}

			/* Generate the new entry. */
			code = free_ent;
			if (code < maxmaxcode) {
				tab_prefixof(code) = (unsigned short) oldcode;
				tab_suffixof(code) = (unsigned char) finchar;
				free_ent = code + 1;
			}

			/* Remember previous code.  */
			oldcode = incode;
		}

	} while (rsize > 0);

	if (outpos > 0) {
		full_write(fd_out, outbuf, outpos);
//error check??
		IF_DESKTOP(total_written += outpos;)
	}

	retval = IF_DESKTOP(total_written) + 0;
 err:
	free(inbuf);
	free(outbuf);
	free(htab);
	free(codetab);
	return retval;
}