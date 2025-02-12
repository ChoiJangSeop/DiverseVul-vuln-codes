ImagingSgiRleDecode(Imaging im, ImagingCodecState state, UINT8 *buf, Py_ssize_t bytes) {
    UINT8 *ptr;
    SGISTATE *c;
    int err = 0;
    int status;

    /* size check */
    if (im->xsize > INT_MAX / im->bands || im->ysize > INT_MAX / im->bands) {
        state->errcode = IMAGING_CODEC_MEMORY;
        return -1;
    }

    /* Get all data from File descriptor */
    c = (SGISTATE *)state->context;
    _imaging_seek_pyFd(state->fd, 0L, SEEK_END);
    c->bufsize = _imaging_tell_pyFd(state->fd);
    c->bufsize -= SGI_HEADER_SIZE;

    c->tablen = im->bands * im->ysize;
    /* below, we populate the starttab and lentab into the bufsize,
       each with 4 bytes per element of tablen
       Check here before we allocate any memory
    */
    if (c->bufsize < 8 * c->tablen) {
        state->errcode = IMAGING_CODEC_OVERRUN;
        return -1;
    }

    ptr = malloc(sizeof(UINT8) * c->bufsize);
    if (!ptr) {
        state->errcode = IMAGING_CODEC_MEMORY;
        return -1;
    }
    _imaging_seek_pyFd(state->fd, SGI_HEADER_SIZE, SEEK_SET);
    _imaging_read_pyFd(state->fd, (char *)ptr, c->bufsize);

    /* decoder initialization */
    state->count = 0;
    state->y = 0;
    if (state->ystep < 0) {
        state->y = im->ysize - 1;
    } else {
        state->ystep = 1;
    }

    /* Allocate memory for RLE tables and rows */
    free(state->buffer);
    state->buffer = NULL;
    /* malloc overflow check above */
    state->buffer = calloc(im->xsize * im->bands, sizeof(UINT8) * 2);
    c->starttab = calloc(c->tablen, sizeof(UINT32));
    c->lengthtab = calloc(c->tablen, sizeof(UINT32));
    if (!state->buffer || !c->starttab || !c->lengthtab) {
        err = IMAGING_CODEC_MEMORY;
        goto sgi_finish_decode;
    }
    /* populate offsets table */
    for (c->tabindex = 0, c->bufindex = 0; c->tabindex < c->tablen;
         c->tabindex++, c->bufindex += 4) {
        read4B(&c->starttab[c->tabindex], &ptr[c->bufindex]);
    }
    /* populate lengths table */
    for (c->tabindex = 0, c->bufindex = c->tablen * sizeof(UINT32);
         c->tabindex < c->tablen;
         c->tabindex++, c->bufindex += 4) {
        read4B(&c->lengthtab[c->tabindex], &ptr[c->bufindex]);
    }

    state->count += c->tablen * sizeof(UINT32) * 2;

    /* read compressed rows */
    for (c->rowno = 0; c->rowno < im->ysize; c->rowno++, state->y += state->ystep) {
        for (c->channo = 0; c->channo < im->bands; c->channo++) {
            c->rleoffset = c->starttab[c->rowno + c->channo * im->ysize];
            c->rlelength = c->lengthtab[c->rowno + c->channo * im->ysize];
            c->rleoffset -= SGI_HEADER_SIZE;

            if (c->rleoffset + c->rlelength > c->bufsize) {
                state->errcode = IMAGING_CODEC_OVERRUN;
                goto sgi_finish_decode;
            }

            /* row decompression */
            if (c->bpc == 1) {
                status = expandrow(
                    &state->buffer[c->channo],
                    &ptr[c->rleoffset],
                    c->rlelength,
                    im->bands,
                    im->xsize);
            } else {
                status = expandrow2(
                    &state->buffer[c->channo * 2],
                    &ptr[c->rleoffset],
                    c->rlelength,
                    im->bands,
                    im->xsize);
            }
            if (status == -1) {
                state->errcode = IMAGING_CODEC_OVERRUN;
                goto sgi_finish_decode;
            } else if (status == 1) {
                goto sgi_finish_decode;
            }

            state->count += c->rlelength;
        }

        /* store decompressed data in image */
        state->shuffle((UINT8 *)im->image[state->y], state->buffer, im->xsize);
    }

    c->bufsize++;

sgi_finish_decode:;

    free(c->starttab);
    free(c->lengthtab);
    free(ptr);
    if (err != 0) {
        state->errcode = err;
        return -1;
    }
    return state->count - c->bufsize;
}