void jpc_qmfb_split_colres(jpc_fix_t *a, int numrows, int numcols,
  int stride, int parity)
{

	int bufsize = JPC_CEILDIVPOW2(numrows, 1);
	jpc_fix_t splitbuf[QMFB_SPLITBUFSIZE * JPC_QMFB_COLGRPSIZE];
	jpc_fix_t *buf = splitbuf;
	jpc_fix_t *srcptr;
	jpc_fix_t *dstptr;
	register jpc_fix_t *srcptr2;
	register jpc_fix_t *dstptr2;
	register int n;
	register int i;
	int m;
	int hstartcol;

	/* Get a buffer. */
	if (bufsize > QMFB_SPLITBUFSIZE) {
		if (!(buf = jas_alloc2(bufsize, sizeof(jpc_fix_t)))) {
			/* We have no choice but to commit suicide in this case. */
			abort();
		}
	}

	if (numrows >= 2) {
		hstartcol = (numrows + 1 - parity) >> 1;
		// ORIGINAL (WRONG): m = (parity) ? hstartcol : (numrows - hstartcol);
		m = numrows - hstartcol;

		/* Save the samples destined for the highpass channel. */
		n = m;
		dstptr = buf;
		srcptr = &a[(1 - parity) * stride];
		while (n-- > 0) {
			dstptr2 = dstptr;
			srcptr2 = srcptr;
			for (i = 0; i < numcols; ++i) {
				*dstptr2 = *srcptr2;
				++dstptr2;
				++srcptr2;
			}
			dstptr += numcols;
			srcptr += stride << 1;
		}
		/* Copy the appropriate samples into the lowpass channel. */
		dstptr = &a[(1 - parity) * stride];
		srcptr = &a[(2 - parity) * stride];
		n = numrows - m - (!parity);
		while (n-- > 0) {
			dstptr2 = dstptr;
			srcptr2 = srcptr;
			for (i = 0; i < numcols; ++i) {
				*dstptr2 = *srcptr2;
				++dstptr2;
				++srcptr2;
			}
			dstptr += stride;
			srcptr += stride << 1;
		}
		/* Copy the saved samples into the highpass channel. */
		dstptr = &a[hstartcol * stride];
		srcptr = buf;
		n = m;
		while (n-- > 0) {
			dstptr2 = dstptr;
			srcptr2 = srcptr;
			for (i = 0; i < numcols; ++i) {
				*dstptr2 = *srcptr2;
				++dstptr2;
				++srcptr2;
			}
			dstptr += stride;
			srcptr += numcols;
		}
	}

	/* If the split buffer was allocated on the heap, free this memory. */
	if (buf != splitbuf) {
		jas_free(buf);
	}

}