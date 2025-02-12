void jpc_qmfb_join_col(jpc_fix_t *a, int numrows, int stride,
  int parity)
{

	int bufsize = JPC_CEILDIVPOW2(numrows, 1);
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

	hstartcol = (numrows + 1 - parity) >> 1;

	/* Save the samples from the lowpass channel. */
	n = hstartcol;
	srcptr = &a[0];
	dstptr = buf;
	while (n-- > 0) {
		*dstptr = *srcptr;
		srcptr += stride;
		++dstptr;
	}
	/* Copy the samples from the highpass channel into place. */
	srcptr = &a[hstartcol * stride];
	dstptr = &a[(1 - parity) * stride];
	n = numrows - hstartcol;
	while (n-- > 0) {
		*dstptr = *srcptr;
		dstptr += 2 * stride;
		srcptr += stride;
	}
	/* Copy the samples from the lowpass channel into place. */
	srcptr = buf;
	dstptr = &a[parity * stride];
	n = hstartcol;
	while (n-- > 0) {
		*dstptr = *srcptr;
		dstptr += 2 * stride;
		++srcptr;
	}

	/* If the join buffer was allocated on the heap, free this memory. */
	if (buf != joinbuf) {
		jas_free(buf);
	}

}