void jpc_qmfb_join_row(jpc_fix_t *a, int numcols, int parity)
{

	int bufsize = JPC_CEILDIVPOW2(numcols, 1);
	jpc_fix_t joinbuf[QMFB_JOINBUFSIZE];
	jpc_fix_t *buf = joinbuf;
	register jpc_fix_t *srcptr;
	register jpc_fix_t *dstptr;
	register int n;
	int hstartcol;

	/* Allocate memory for the join buffer from the heap. */
	if (bufsize > QMFB_JOINBUFSIZE) {
		if (!(buf = jas_malloc(bufsize * sizeof(jpc_fix_t)))) {
			/* We have no choice but to commit suicide. */
			abort();
		}
	}

	hstartcol = (numcols + 1 - parity) >> 1;

	/* Save the samples from the lowpass channel. */
	n = hstartcol;
	srcptr = &a[0];
	dstptr = buf;
	while (n-- > 0) {
		*dstptr = *srcptr;
		++srcptr;
		++dstptr;
	}
	/* Copy the samples from the highpass channel into place. */
	srcptr = &a[hstartcol];
	dstptr = &a[1 - parity];
	n = numcols - hstartcol;
	while (n-- > 0) {
		*dstptr = *srcptr;
		dstptr += 2;
		++srcptr;
	}
	/* Copy the samples from the lowpass channel into place. */
	srcptr = buf;
	dstptr = &a[parity];
	n = hstartcol;
	while (n-- > 0) {
		*dstptr = *srcptr;
		dstptr += 2;
		++srcptr;
	}

	/* If the join buffer was allocated on the heap, free this memory. */
	if (buf != joinbuf) {
		jas_free(buf);
	}

}