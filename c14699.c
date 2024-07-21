PJ_DEF(pj_status_t) pjmedia_wav_player_port_create( pj_pool_t *pool,
						     const char *filename,
						     unsigned ptime,
						     unsigned options,
						     pj_ssize_t buff_size,
						     pjmedia_port **p_port )
{
    pjmedia_wave_hdr wave_hdr;
    pj_ssize_t size_to_read, size_read;
    struct file_reader_port *fport;
    pjmedia_audio_format_detail *ad;
    pj_off_t pos;
    pj_str_t name;
    unsigned samples_per_frame;
    pj_status_t status = PJ_SUCCESS;


    /* Check arguments. */
    PJ_ASSERT_RETURN(pool && filename && p_port, PJ_EINVAL);

    /* Check the file really exists. */
    if (!pj_file_exists(filename)) {
	return PJ_ENOTFOUND;
    }

    /* Normalize ptime */
    if (ptime == 0)
	ptime = 20;

    /* Normalize buff_size */
    if (buff_size < 1) buff_size = PJMEDIA_FILE_PORT_BUFSIZE;


    /* Create fport instance. */
    fport = create_file_port(pool);
    if (!fport) {
	return PJ_ENOMEM;
    }


    /* Get the file size. */
    fport->fsize = pj_file_size(filename);

    /* Size must be more than WAVE header size */
    if (fport->fsize <= sizeof(pjmedia_wave_hdr)) {
	return PJMEDIA_ENOTVALIDWAVE;
    }

    /* Open file. */
    status = pj_file_open( pool, filename, PJ_O_RDONLY, &fport->fd);
    if (status != PJ_SUCCESS)
	return status;

    /* Read the file header plus fmt header only. */
    size_read = size_to_read = sizeof(wave_hdr) - 8;
    status = pj_file_read( fport->fd, &wave_hdr, &size_read);
    if (status != PJ_SUCCESS) {
	pj_file_close(fport->fd);
	return status;
    }
    if (size_read != size_to_read) {
	pj_file_close(fport->fd);
	return PJMEDIA_ENOTVALIDWAVE;
    }

    /* Normalize WAVE header fields values from little-endian to host
     * byte order.
     */
    pjmedia_wave_hdr_file_to_host(&wave_hdr);
    
    /* Validate WAVE file. */
    if (wave_hdr.riff_hdr.riff != PJMEDIA_RIFF_TAG ||
	wave_hdr.riff_hdr.wave != PJMEDIA_WAVE_TAG ||
	wave_hdr.fmt_hdr.fmt != PJMEDIA_FMT_TAG)
    {
	pj_file_close(fport->fd);
	TRACE_((THIS_FILE, 
		"actual value|expected riff=%x|%x, wave=%x|%x fmt=%x|%x",
		wave_hdr.riff_hdr.riff, PJMEDIA_RIFF_TAG,
		wave_hdr.riff_hdr.wave, PJMEDIA_WAVE_TAG,
		wave_hdr.fmt_hdr.fmt, PJMEDIA_FMT_TAG));
	return PJMEDIA_ENOTVALIDWAVE;
    }

    /* Validate format and its attributes (i.e: bits per sample, block align) */
    switch (wave_hdr.fmt_hdr.fmt_tag) {
    case PJMEDIA_WAVE_FMT_TAG_PCM:
	if (wave_hdr.fmt_hdr.bits_per_sample != 16 || 
	    wave_hdr.fmt_hdr.block_align != 2 * wave_hdr.fmt_hdr.nchan)
	    status = PJMEDIA_EWAVEUNSUPP;
	break;

    case PJMEDIA_WAVE_FMT_TAG_ALAW:
    case PJMEDIA_WAVE_FMT_TAG_ULAW:
	if (wave_hdr.fmt_hdr.bits_per_sample != 8 ||
	    wave_hdr.fmt_hdr.block_align != wave_hdr.fmt_hdr.nchan)
	    status = PJMEDIA_ENOTVALIDWAVE;
	break;

    default:
	status = PJMEDIA_EWAVEUNSUPP;
	break;
    }

    if (status != PJ_SUCCESS) {
	pj_file_close(fport->fd);
	return status;
    }

    fport->fmt_tag = (pjmedia_wave_fmt_tag)wave_hdr.fmt_hdr.fmt_tag;
    fport->bytes_per_sample = (pj_uint16_t) 
			      (wave_hdr.fmt_hdr.bits_per_sample / 8);

    /* If length of fmt_header is greater than 16, skip the remaining
     * fmt header data.
     */
    if (wave_hdr.fmt_hdr.len > 16) {
	size_to_read = wave_hdr.fmt_hdr.len - 16;
	status = pj_file_setpos(fport->fd, size_to_read, PJ_SEEK_CUR);
	if (status != PJ_SUCCESS) {
	    pj_file_close(fport->fd);
	    return status;
	}
    }

    /* Repeat reading the WAVE file until we have 'data' chunk */
    for (;;) {
	pjmedia_wave_subchunk subchunk;
	size_read = 8;
	status = pj_file_read(fport->fd, &subchunk, &size_read);
	if (status != PJ_SUCCESS || size_read != 8) {
	    pj_file_close(fport->fd);
	    return PJMEDIA_EWAVETOOSHORT;
	}

	/* Normalize endianness */
	PJMEDIA_WAVE_NORMALIZE_SUBCHUNK(&subchunk);

	/* Break if this is "data" chunk */
	if (subchunk.id == PJMEDIA_DATA_TAG) {
	    wave_hdr.data_hdr.data = PJMEDIA_DATA_TAG;
	    wave_hdr.data_hdr.len = subchunk.len;
	    break;
	}

	/* Otherwise skip the chunk contents */
	size_to_read = subchunk.len;
	status = pj_file_setpos(fport->fd, size_to_read, PJ_SEEK_CUR);
	if (status != PJ_SUCCESS) {
	    pj_file_close(fport->fd);
	    return status;
	}
    }

    /* Current file position now points to start of data */
    status = pj_file_getpos(fport->fd, &pos);
    fport->start_data = (unsigned)pos;
    fport->data_len = wave_hdr.data_hdr.len;
    fport->data_left = wave_hdr.data_hdr.len;

    /* Validate length. */
    if (wave_hdr.data_hdr.len > fport->fsize - fport->start_data) {
    	/* Actual data length may be shorter than declared. We should still
    	 * try to play whatever data is there instead of immediately returning
    	 * error.
    	 */
    	wave_hdr.data_hdr.len = (pj_uint32_t)fport->fsize - fport->start_data;
	// pj_file_close(fport->fd);
	// return PJMEDIA_EWAVEUNSUPP;
    }
    if (wave_hdr.data_hdr.len < ptime * wave_hdr.fmt_hdr.sample_rate *
				wave_hdr.fmt_hdr.nchan / 1000)
    {
	pj_file_close(fport->fd);
	return PJMEDIA_EWAVETOOSHORT;
    }

    /* It seems like we have a valid WAVE file. */

    /* Initialize */
    fport->options = options;

    /* Update port info. */
    ad = pjmedia_format_get_audio_format_detail(&fport->base.info.fmt, 1);
    pj_strdup2(pool, &name, filename);
    samples_per_frame = ptime * wave_hdr.fmt_hdr.sample_rate *
		        wave_hdr.fmt_hdr.nchan / 1000;
    pjmedia_port_info_init(&fport->base.info, &name, SIGNATURE,
			   wave_hdr.fmt_hdr.sample_rate,
			   wave_hdr.fmt_hdr.nchan,
			   BITS_PER_SAMPLE,
			   samples_per_frame);

    /* If file is shorter than buffer size, adjust buffer size to file
     * size. Otherwise EOF callback will be called multiple times when
     * fill_buffer() is called.
     */
    if (wave_hdr.data_hdr.len < (unsigned)buff_size)
	buff_size = wave_hdr.data_hdr.len;

    /* Create file buffer.
     */
    fport->bufsize = (pj_uint32_t)buff_size;


    /* samples_per_frame must be smaller than bufsize (because get_frame()
     * doesn't handle this case).
     */
    if (samples_per_frame * fport->bytes_per_sample >= fport->bufsize) {
	pj_file_close(fport->fd);
	return PJ_EINVAL;
    }

    /* Create buffer. */
    fport->buf = (char*) pj_pool_alloc(pool, fport->bufsize);
    if (!fport->buf) {
	pj_file_close(fport->fd);
	return PJ_ENOMEM;
    }
 
    fport->readpos = fport->buf;

    /* Set initial position of the file. */
    fport->fpos = fport->start_data;

    /* Fill up the buffer. */
    status = fill_buffer(fport);
    if (status != PJ_SUCCESS) {
	pj_file_close(fport->fd);
	return status;
    }

    /* Done. */

    *p_port = &fport->base;


    PJ_LOG(4,(THIS_FILE, 
	      "File player '%.*s' created: samp.rate=%d, ch=%d, bufsize=%uKB, "
	      "filesize=%luKB",
	      (int)fport->base.info.name.slen,
	      fport->base.info.name.ptr,
	      ad->clock_rate,
	      ad->channel_count,
	      fport->bufsize / 1000,
	      (unsigned long)(fport->fsize / 1000)));

    return PJ_SUCCESS;
}