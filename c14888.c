ImagingPcxDecode(Imaging im, ImagingCodecState state, UINT8* buf, Py_ssize_t bytes)
{
    UINT8 n;
    UINT8* ptr;

    if (strcmp(im->mode, "1") == 0 && state->xsize > state->bytes * 8) {
        state->errcode = IMAGING_CODEC_OVERRUN;
        return -1;
    } else if (strcmp(im->mode, "P") == 0 && state->xsize > state->bytes) {
        state->errcode = IMAGING_CODEC_OVERRUN;
        return -1;
    }

    ptr = buf;

    for (;;) {

	if (bytes < 1)
	    return ptr - buf;

	if ((*ptr & 0xC0) == 0xC0) {

	    /* Run */
	    if (bytes < 2)
		return ptr - buf;

	    n = ptr[0] & 0x3F;

	    while (n > 0) {
		if (state->x >= state->bytes) {
		    state->errcode = IMAGING_CODEC_OVERRUN;
		    break;
		}
		state->buffer[state->x++] = ptr[1];
		n--;
	    }

	    ptr += 2; bytes -= 2;

	} else {

	    /* Literal */
	    state->buffer[state->x++] = ptr[0];
	    ptr++; bytes--;

	}

	if (state->x >= state->bytes) {
        if (state->bytes % state->xsize && state->bytes > state->xsize) {
            int bands = state->bytes / state->xsize;
            int stride = state->bytes / bands;
            int i;
            for (i=1; i< bands; i++) {  // note -- skipping first band
                memmove(&state->buffer[i*state->xsize],
                        &state->buffer[i*stride],
                        state->xsize);
            }
        }
	    /* Got a full line, unpack it */
	    state->shuffle((UINT8*) im->image[state->y + state->yoff] +
			   state->xoff * im->pixelsize, state->buffer,
			   state->xsize);

	    state->x = 0;

	    if (++state->y >= state->ysize) {
		/* End of file (errcode = 0) */
		return -1;
	    }
	}

    }
}