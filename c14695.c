pjmedia_avi_player_create_streams(pj_pool_t *pool,
                                  const char *filename,
				  unsigned options,
				  pjmedia_avi_streams **p_streams)
{
    pjmedia_avi_hdr avi_hdr;
    struct avi_reader_port *fport[PJMEDIA_AVI_MAX_NUM_STREAMS];
    pj_off_t pos;
    unsigned i, nstr = 0;
    pj_status_t status = PJ_SUCCESS;

    /* Check arguments. */
    PJ_ASSERT_RETURN(pool && filename && p_streams, PJ_EINVAL);

    /* Check the file really exists. */
    if (!pj_file_exists(filename)) {
	return PJ_ENOTFOUND;
    }

    /* Create fport instance. */
    fport[0] = create_avi_port(pool);
    if (!fport[0]) {
	return PJ_ENOMEM;
    }

    /* Get the file size. */
    fport[0]->fsize = pj_file_size(filename);

    /* Size must be more than AVI header size */
    if (fport[0]->fsize <= sizeof(riff_hdr_t) + sizeof(avih_hdr_t) + 
                           sizeof(strl_hdr_t))
    {
	return PJMEDIA_EINVALIMEDIATYPE;
    }

    /* Open file. */
    status = pj_file_open(pool, filename, PJ_O_RDONLY, &fport[0]->fd);
    if (status != PJ_SUCCESS)
	return status;

    /* Read the RIFF + AVIH header. */
    status = file_read(fport[0]->fd, &avi_hdr,
                       sizeof(riff_hdr_t) + sizeof(avih_hdr_t));
    if (status != PJ_SUCCESS)
        goto on_error;

    /* Validate AVI file. */
    if (!COMPARE_TAG(avi_hdr.riff_hdr.riff, PJMEDIA_AVI_RIFF_TAG) ||
	!COMPARE_TAG(avi_hdr.riff_hdr.avi, PJMEDIA_AVI_AVI_TAG) ||
        !COMPARE_TAG(avi_hdr.avih_hdr.list_tag, PJMEDIA_AVI_LIST_TAG) ||
        !COMPARE_TAG(avi_hdr.avih_hdr.hdrl_tag, PJMEDIA_AVI_HDRL_TAG) ||
        !COMPARE_TAG(avi_hdr.avih_hdr.avih, PJMEDIA_AVI_AVIH_TAG))
    {
	status = PJMEDIA_EINVALIMEDIATYPE;
        goto on_error;
    }

    PJ_LOG(5, (THIS_FILE, "The AVI file has %d streams.",
               avi_hdr.avih_hdr.num_streams));

    /* Unsupported AVI format. */
    if (avi_hdr.avih_hdr.num_streams > PJMEDIA_AVI_MAX_NUM_STREAMS) {
        status = PJMEDIA_EAVIUNSUPP;
        goto on_error;
    }

    /** 
     * TODO: Possibly unsupported AVI format.
     * If you encounter this warning, verify whether the avi player
     * is working properly.
     */
    if (avi_hdr.avih_hdr.flags & AVIF_MUSTUSEINDEX ||
        avi_hdr.avih_hdr.pad > 1)
    {
        PJ_LOG(3, (THIS_FILE, "Warning!!! Possibly unsupported AVI format: "
                   "flags:%d, pad:%d", avi_hdr.avih_hdr.flags, 
                   avi_hdr.avih_hdr.pad));
    }

    /* Read the headers of each stream. */
    for (i = 0; i < avi_hdr.avih_hdr.num_streams; i++) {
        pj_size_t elem = 0;
        pj_ssize_t size_to_read;

        /* Read strl header */
        status = file_read(fport[0]->fd, &avi_hdr.strl_hdr[i],
                           sizeof(strl_hdr_t));
        if (status != PJ_SUCCESS)
            goto on_error;
        
        elem = COMPARE_TAG(avi_hdr.strl_hdr[i].data_type, 
                           PJMEDIA_AVI_VIDS_TAG) ? 
               sizeof(strf_video_hdr_t) :
               COMPARE_TAG(avi_hdr.strl_hdr[i].data_type, 
                           PJMEDIA_AVI_AUDS_TAG) ?
               sizeof(strf_audio_hdr_t) : 0;

        /* Read strf header */
        status = file_read2(fport[0]->fd, &avi_hdr.strf_hdr[i],
                            elem, 0);
        if (status != PJ_SUCCESS)
            goto on_error;

        /* Normalize the endian */
        if (elem == sizeof(strf_video_hdr_t))
            data_to_host2(&avi_hdr.strf_hdr[i],
                          sizeof(strf_video_hdr_sizes)/
                          sizeof(strf_video_hdr_sizes[0]),
                          strf_video_hdr_sizes);
        else if (elem == sizeof(strf_audio_hdr_t))
            data_to_host2(&avi_hdr.strf_hdr[i],
                          sizeof(strf_audio_hdr_sizes)/
                          sizeof(strf_audio_hdr_sizes[0]),
                          strf_audio_hdr_sizes);

        /* Skip the remainder of the header */
        size_to_read = avi_hdr.strl_hdr[i].list_sz - (sizeof(strl_hdr_t) -
                       8) - elem;
	status = pj_file_setpos(fport[0]->fd, size_to_read, PJ_SEEK_CUR);
	if (status != PJ_SUCCESS) {
            goto on_error;
	}
    }

    /* Finish reading the AVIH header */
    status = pj_file_setpos(fport[0]->fd, avi_hdr.avih_hdr.list_sz +
                            sizeof(riff_hdr_t) + 8, PJ_SEEK_SET);
    if (status != PJ_SUCCESS) {
        goto on_error;
    }

    /* Skip any JUNK or LIST INFO until we get MOVI tag */
    do {
        pjmedia_avi_subchunk ch;
        int read = 0;

        status = file_read(fport[0]->fd, &ch, sizeof(pjmedia_avi_subchunk));
        if (status != PJ_SUCCESS) {
            goto on_error;
        }

        if (COMPARE_TAG(ch.id, PJMEDIA_AVI_LIST_TAG))
        {
            read = 4;
            status = file_read(fport[0]->fd, &ch, read);
            if (COMPARE_TAG(ch.id, PJMEDIA_AVI_MOVI_TAG))
                break;
        }

        status = pj_file_setpos(fport[0]->fd, ch.len-read, PJ_SEEK_CUR);
        if (status != PJ_SUCCESS) {
            goto on_error;
        }
    } while(1);

    status = pj_file_getpos(fport[0]->fd, &pos);
    if (status != PJ_SUCCESS)
        goto on_error;

    for (i = 0, nstr = 0; i < avi_hdr.avih_hdr.num_streams; i++) {
	pjmedia_format_id fmt_id;

        /* Skip non-audio, non-video, or disabled streams) */
        if ((!COMPARE_TAG(avi_hdr.strl_hdr[i].data_type, 
                          PJMEDIA_AVI_VIDS_TAG) &&
             !COMPARE_TAG(avi_hdr.strl_hdr[i].data_type, 
                          PJMEDIA_AVI_AUDS_TAG)) ||
            avi_hdr.strl_hdr[i].flags & AVISF_DISABLED)
        {
            continue;
        }

        if (COMPARE_TAG(avi_hdr.strl_hdr[i].data_type, 
                        PJMEDIA_AVI_VIDS_TAG))
        {
            int j;

            if (avi_hdr.strl_hdr[i].flags & AVISF_VIDEO_PALCHANGES) {
                PJ_LOG(4, (THIS_FILE, "Unsupported video stream"));
                continue;
            }

            fmt_id = avi_hdr.strl_hdr[i].codec;
            for (j = sizeof(avi_fmts)/sizeof(avi_fmts[0])-1; j >= 0; j--) {
                /* Check supported video formats here */
                if (fmt_id == avi_fmts[j].fmt_id) {
                    if (avi_fmts[j].eff_fmt_id)
                        fmt_id = avi_fmts[j].eff_fmt_id;
                    break;
                }
            }
            
            if (j < 0) {
                PJ_LOG(4, (THIS_FILE, "Unsupported video stream"));
                continue;
            }
        } else {
            /* Check supported audio formats here */
	    strf_audio_hdr_t *hdr = (strf_audio_hdr_t*)
				    &avi_hdr.strf_hdr[i].strf_audio_hdr;
            if (hdr->fmt_tag == PJMEDIA_WAVE_FMT_TAG_PCM &&
		hdr->bits_per_sample == 16)
	    {
		fmt_id = PJMEDIA_FORMAT_PCM;
	    }
	    else if (hdr->fmt_tag == PJMEDIA_WAVE_FMT_TAG_ALAW)
	    {
		fmt_id = PJMEDIA_FORMAT_PCMA;
	    }
	    else if (hdr->fmt_tag == PJMEDIA_WAVE_FMT_TAG_ULAW)
	    {
		fmt_id = PJMEDIA_FORMAT_PCMU;
	    }
	    else
            {
                PJ_LOG(4, (THIS_FILE, "Unsupported audio stream"));
                continue;
            }
        }

        if (nstr > 0) {
            /* Create fport instance. */
            fport[nstr] = create_avi_port(pool);
            if (!fport[nstr]) {
	        status = PJ_ENOMEM;
                goto on_error;
            }

            /* Open file. */
            status = pj_file_open(pool, filename, PJ_O_RDONLY,
                                  &fport[nstr]->fd);
            if (status != PJ_SUCCESS)
                goto on_error;

            /* Set the file position */
            status = pj_file_setpos(fport[nstr]->fd, pos, PJ_SEEK_SET);
            if (status != PJ_SUCCESS) {
                goto on_error;
            }
        }

        fport[nstr]->stream_id = i;
        fport[nstr]->fmt_id = fmt_id;

        nstr++;
    }

    if (nstr == 0) {
        status = PJMEDIA_EAVIUNSUPP;
        goto on_error;
    }

    for (i = 0; i < nstr; i++) {
        strl_hdr_t *strl_hdr = &avi_hdr.strl_hdr[fport[i]->stream_id];

        /* Initialize */
        fport[i]->options = options;
        fport[i]->fsize = fport[0]->fsize;
        /* Current file position now points to start of data */
        fport[i]->start_data = pos;
        
        if (COMPARE_TAG(strl_hdr->data_type, PJMEDIA_AVI_VIDS_TAG)) {
            strf_video_hdr_t *strf_hdr =
                &avi_hdr.strf_hdr[fport[i]->stream_id].strf_video_hdr;
            const pjmedia_video_format_info *vfi;

            vfi = pjmedia_get_video_format_info(
                pjmedia_video_format_mgr_instance(),
                strl_hdr->codec);

            fport[i]->bits_per_sample = (vfi ? vfi->bpp : 0);
            fport[i]->usec_per_frame = avi_hdr.avih_hdr.usec_per_frame;
            pjmedia_format_init_video(&fport[i]->base.info.fmt,
                                      fport[i]->fmt_id,
                                      strf_hdr->biWidth,
                                      strf_hdr->biHeight,
                                      strl_hdr->rate,
                                      strl_hdr->scale);
#if 0
            /* The calculation below is wrong. strf_hdr->biSizeImage shows
             * uncompressed size. Looks like we need to go the ugly way to
             * get the bitrage:
             *    http://www.virtualdub.org/blog/pivot/entry.php?id=159
             */
            bps = strf_hdr->biSizeImage * 8 * strl_hdr->rate / strl_hdr->scale;
            if (bps==0) {
        	/* strf_hdr->biSizeImage may be zero for uncompressed RGB */
        	bps = strf_hdr->biWidth * strf_hdr->biHeight *
        		strf_hdr->biBitCount *
        		strl_hdr->rate / strl_hdr->scale;
            }
            fport[i]->base.info.fmt.det.vid.avg_bps = bps;
            fport[i]->base.info.fmt.det.vid.max_bps = bps;
#endif
        } else {
            strf_audio_hdr_t *strf_hdr =
                &avi_hdr.strf_hdr[fport[i]->stream_id].strf_audio_hdr;

            fport[i]->bits_per_sample = strf_hdr->bits_per_sample;
            fport[i]->usec_per_frame = avi_hdr.avih_hdr.usec_per_frame;
            pjmedia_format_init_audio(&fport[i]->base.info.fmt,
                                      fport[i]->fmt_id,
                                      strf_hdr->sample_rate,
                                      strf_hdr->nchannels,
                                      strf_hdr->bits_per_sample,
                                      20000 /* fport[i]->usec_per_frame */,
                                      strf_hdr->bytes_per_sec * 8,
                                      strf_hdr->bytes_per_sec * 8);

	    /* Set format to PCM (we will decode PCMA/U) */
	    if (fport[i]->fmt_id == PJMEDIA_FORMAT_PCMA ||
		fport[i]->fmt_id == PJMEDIA_FORMAT_PCMU)
	    {
		fport[i]->base.info.fmt.id = PJMEDIA_FORMAT_PCM;
		fport[i]->base.info.fmt.det.aud.bits_per_sample = 16;
	    }
	}

        pj_strdup2(pool, &fport[i]->base.info.name, filename);
    }

    /* Done. */
    *p_streams = pj_pool_alloc(pool, sizeof(pjmedia_avi_streams));
    (*p_streams)->num_streams = nstr;
    (*p_streams)->streams = pj_pool_calloc(pool, (*p_streams)->num_streams,
                                           sizeof(pjmedia_port *));
    for (i = 0; i < nstr; i++)
        (*p_streams)->streams[i] = &fport[i]->base;

    PJ_LOG(4,(THIS_FILE, 
	      "AVI file player '%.*s' created with "
	      "%d media ports",
	      (int)fport[0]->base.info.name.slen,
	      fport[0]->base.info.name.ptr,
              (*p_streams)->num_streams));

    return PJ_SUCCESS;

on_error:
    fport[0]->base.on_destroy(&fport[0]->base);
    for (i = 1; i < nstr; i++)
        fport[i]->base.on_destroy(&fport[i]->base);
    if (status == AVI_EOF)
        return PJMEDIA_EINVALIMEDIATYPE;
    return status;
}