PJ_DEF(pj_status_t) pjmedia_wav_playlist_create(pj_pool_t *pool,
						const pj_str_t *port_label,
						const pj_str_t file_list[],
						int file_count,
						unsigned ptime,
						unsigned options,
						pj_ssize_t buff_size,
						pjmedia_port **p_port)
{
    struct playlist_port *fport;
    pjmedia_audio_format_detail *afd;
    pj_off_t pos;
    pj_status_t status;
    int index;
    pj_bool_t has_wave_info = PJ_FALSE;
    pj_str_t tmp_port_label;
    char filename[PJ_MAXPATH];	/* filename for open operations.    */


    /* Check arguments. */
    PJ_ASSERT_RETURN(pool && file_list && file_count && p_port, PJ_EINVAL);

    /* Normalize port_label */
    if (port_label == NULL || port_label->slen == 0) {
	tmp_port_label = pj_str("WAV playlist");
	port_label = &tmp_port_label;
    }

    /* Be sure all files exist	*/
    for (index=0; index<file_count; index++) {

	PJ_ASSERT_RETURN(file_list[index].slen >= 0, PJ_ETOOSMALL);
	if (file_list[index].slen >= PJ_MAXPATH)
	    return PJ_ENAMETOOLONG;

	pj_memcpy(filename, file_list[index].ptr, file_list[index].slen);
	filename[file_list[index].slen] = '\0';

    	/* Check the file really exists. */
    	if (!pj_file_exists(filename)) {
	    PJ_LOG(4,(THIS_FILE,
		      "WAV playlist error: file '%s' not found",
	      	      filename));
	    return PJ_ENOTFOUND;
    	}
    }

    /* Normalize ptime */
    if (ptime == 0)
	ptime = 20;

    /* Create fport instance. */
    fport = create_file_list_port(pool, port_label);
    if (!fport) {
	return PJ_ENOMEM;
    }

    afd = pjmedia_format_get_audio_format_detail(&fport->base.info.fmt, 1);

    /* start with the first file. */
    fport->current_file = 0;
    fport->max_file = file_count;

    /* Create file descriptor list */
    fport->fd_list = (pj_oshandle_t*)
		     pj_pool_zalloc(pool, sizeof(pj_oshandle_t)*file_count);
    if (!fport->fd_list) {
	return PJ_ENOMEM;
    }

    /* Create file size list */
    fport->fsize_list = (pj_off_t*)
			pj_pool_alloc(pool, sizeof(pj_off_t)*file_count);
    if (!fport->fsize_list) {
	return PJ_ENOMEM;
    }

    /* Create start of WAVE data list */
    fport->start_data_list = (unsigned*)
			     pj_pool_alloc(pool, sizeof(unsigned)*file_count);
    if (!fport->start_data_list) {
	return PJ_ENOMEM;
    }

    /* Create data len list */
    fport->data_len_list = (unsigned*)
			     pj_pool_alloc(pool, sizeof(unsigned)*file_count);
    if (!fport->data_len_list) {
	return PJ_ENOMEM;
    }

    /* Create data left list */
    fport->data_left_list = (unsigned*)
			     pj_pool_alloc(pool, sizeof(unsigned)*file_count);
    if (!fport->data_left_list) {
	return PJ_ENOMEM;
    }

    /* Create file position list */
    fport->fpos_list = (pj_off_t*)
		       pj_pool_alloc(pool, sizeof(pj_off_t)*file_count);
    if (!fport->fpos_list) {
	return PJ_ENOMEM;
    }

    /* Create file buffer once for this operation.
     */
    if (buff_size < 1) buff_size = PJMEDIA_FILE_PORT_BUFSIZE;
    fport->bufsize = (pj_uint32_t)buff_size;


    /* Create buffer. */
    fport->buf = (char*) pj_pool_alloc(pool, fport->bufsize);
    if (!fport->buf) {
	return PJ_ENOMEM;
    }

    /* Initialize port */
    fport->options = options;
    fport->readpos = fport->buf;


    /* ok run this for all files to be sure all are good for playback. */
    for (index=file_count-1; index>=0; index--) {

	pjmedia_wave_hdr wavehdr;
	pj_ssize_t size_to_read, size_read;

	/* we end with the last one so we are good to go if still in function*/
	pj_memcpy(filename, file_list[index].ptr, file_list[index].slen);
	filename[file_list[index].slen] = '\0';

	/* Get the file size. */
	fport->current_file = index;
	fport->fsize_list[index] = pj_file_size(filename);
	
	/* Size must be more than WAVE header size */
	if (fport->fsize_list[index] <= sizeof(pjmedia_wave_hdr)) {
	    status = PJMEDIA_ENOTVALIDWAVE;
	    goto on_error;
	}
	
	/* Open file. */
	status = pj_file_open( pool, filename, PJ_O_RDONLY, 
			       &fport->fd_list[index]);
	if (status != PJ_SUCCESS)
	    goto on_error;
	
	/* Read the file header plus fmt header only. */
	size_read = size_to_read = sizeof(wavehdr) - 8;
	status = pj_file_read( fport->fd_list[index], &wavehdr, &size_read);
	if (status != PJ_SUCCESS) {
	    goto on_error;
	}

	if (size_read != size_to_read) {
	    status = PJMEDIA_ENOTVALIDWAVE;
	    goto on_error;
	}
	
	/* Normalize WAVE header fields values from little-endian to host
	 * byte order.
	 */
	pjmedia_wave_hdr_file_to_host(&wavehdr);
	
	/* Validate WAVE file. */
	if (wavehdr.riff_hdr.riff != PJMEDIA_RIFF_TAG ||
	    wavehdr.riff_hdr.wave != PJMEDIA_WAVE_TAG ||
	    wavehdr.fmt_hdr.fmt != PJMEDIA_FMT_TAG)
	{
	    TRACE_((THIS_FILE,
		"actual value|expected riff=%x|%x, wave=%x|%x fmt=%x|%x",
		wavehdr.riff_hdr.riff, PJMEDIA_RIFF_TAG,
		wavehdr.riff_hdr.wave, PJMEDIA_WAVE_TAG,
		wavehdr.fmt_hdr.fmt, PJMEDIA_FMT_TAG));
	    status = PJMEDIA_ENOTVALIDWAVE;
	    goto on_error;
	}
	
	/* Must be PCM with 16bits per sample */
	if (wavehdr.fmt_hdr.fmt_tag != 1 ||
	    wavehdr.fmt_hdr.bits_per_sample != 16)
	{
	    status = PJMEDIA_EWAVEUNSUPP;
	    goto on_error;
	}
	
	/* Block align must be 2*nchannels */
	if (wavehdr.fmt_hdr.block_align != 
		wavehdr.fmt_hdr.nchan * BYTES_PER_SAMPLE)
	{
	    status = PJMEDIA_EWAVEUNSUPP;
	    goto on_error;
	}
	
	/* If length of fmt_header is greater than 16, skip the remaining
	 * fmt header data.
	 */
	if (wavehdr.fmt_hdr.len > 16) {
	    size_to_read = wavehdr.fmt_hdr.len - 16;
	    status = pj_file_setpos(fport->fd_list[index], size_to_read, 
				    PJ_SEEK_CUR);
	    if (status != PJ_SUCCESS) {
		goto on_error;
	    }
	}
	
	/* Repeat reading the WAVE file until we have 'data' chunk */
	for (;;) {
	    pjmedia_wave_subchunk subchunk;
	    size_read = 8;
	    status = pj_file_read(fport->fd_list[index], &subchunk, 
				  &size_read);
	    if (status != PJ_SUCCESS || size_read != 8) {
		status = PJMEDIA_EWAVETOOSHORT;
		goto on_error;
	    }
	    
	    /* Normalize endianness */
	    PJMEDIA_WAVE_NORMALIZE_SUBCHUNK(&subchunk);
	    
	    /* Break if this is "data" chunk */
	    if (subchunk.id == PJMEDIA_DATA_TAG) {
		wavehdr.data_hdr.data = PJMEDIA_DATA_TAG;
		wavehdr.data_hdr.len = subchunk.len;
		break;
	    }
	    
	    /* Otherwise skip the chunk contents */
	    size_to_read = subchunk.len;
	    status = pj_file_setpos(fport->fd_list[index], size_to_read, 
				    PJ_SEEK_CUR);
	    if (status != PJ_SUCCESS) {
		goto on_error;
	    }
	}
	
	/* Current file position now points to start of data */
	status = pj_file_getpos(fport->fd_list[index], &pos);
	fport->start_data_list[index] = (unsigned)pos;
	fport->data_len_list[index] = wavehdr.data_hdr.len;
	fport->data_left_list[index] = wavehdr.data_hdr.len;
	
	/* Validate length. */
	if (wavehdr.data_hdr.len > fport->fsize_list[index] - 
				       fport->start_data_list[index]) 
	{
	    status = PJMEDIA_EWAVEUNSUPP;
	    goto on_error;
	}
	if (wavehdr.data_hdr.len < ptime * wavehdr.fmt_hdr.sample_rate *
				    wavehdr.fmt_hdr.nchan / 1000)
	{
	    status = PJMEDIA_EWAVETOOSHORT;
	    goto on_error;
	}
	
	/* It seems like we have a valid WAVE file. */
	
	/* Update port info if we don't have one, otherwise check
	 * that the WAV file has the same attributes as previous files. 
	 */
	if (!has_wave_info) {
	    afd->channel_count = wavehdr.fmt_hdr.nchan;
	    afd->clock_rate = wavehdr.fmt_hdr.sample_rate;
	    afd->bits_per_sample = wavehdr.fmt_hdr.bits_per_sample;
	    afd->frame_time_usec = ptime * 1000;
	    afd->avg_bps = afd->max_bps = afd->clock_rate *
					  afd->channel_count *
					  afd->bits_per_sample;

	    has_wave_info = PJ_TRUE;

	} else {

	    /* Check that this file has the same characteristics as the other
	     * files.
	     */
	    if (wavehdr.fmt_hdr.nchan != afd->channel_count ||
		wavehdr.fmt_hdr.sample_rate != afd->clock_rate ||
		wavehdr.fmt_hdr.bits_per_sample != afd->bits_per_sample)
	    {
		/* This file has different characteristics than the other 
		 * files. 
		 */
		PJ_LOG(4,(THIS_FILE,
		          "WAV playlist error: file '%s' has differrent number"
			  " of channels, sample rate, or bits per sample",
	      		  filename));
		status = PJMEDIA_EWAVEUNSUPP;
		goto on_error;
	    }

	}

	/* If file is shorter than buffer size, adjust buffer size to file
	 * size. Otherwise EOF callback will be called multiple times when
	 * file_fill_buffer() is called.
	 */
	if (wavehdr.data_hdr.len < (unsigned)buff_size)
	    buff_size = wavehdr.data_hdr.len;

	/* Create file buffer.
	 */
	fport->bufsize = (pj_uint32_t)buff_size;	
	
	/* Set initial position of the file. */
	fport->fpos_list[index] = fport->start_data_list[index];
    }

    /* Fill up the buffer. */
    status = file_fill_buffer(fport);
    if (status != PJ_SUCCESS) {
	goto on_error;
    }
    
    /* Done. */
    
    *p_port = &fport->base;
    
    PJ_LOG(4,(THIS_FILE,
	     "WAV playlist '%.*s' created: samp.rate=%d, ch=%d, bufsize=%uKB",
	     (int)port_label->slen,
	     port_label->ptr,
	     afd->clock_rate,
	     afd->channel_count,
	     fport->bufsize / 1000));
    
    return PJ_SUCCESS;

on_error:
    for (index=0; index<file_count; ++index) {
	if (fport->fd_list[index] != 0)
	    pj_file_close(fport->fd_list[index]);
    }

    return status;
}