static pj_status_t avi_get_frame(pjmedia_port *this_port, 
			         pjmedia_frame *frame)
{
    struct avi_reader_port *fport = (struct avi_reader_port*)this_port;
    pj_status_t status = PJ_SUCCESS;
    pj_ssize_t size_read = 0, size_to_read = 0;

    pj_assert(fport->base.info.signature == SIGNATURE);

    /* We encountered end of file */
    if (fport->eof) {
	PJ_LOG(5,(THIS_FILE, "File port %.*s EOF",
		  (int)fport->base.info.name.slen,
		  fport->base.info.name.ptr));

	/* Call callback, if any */
	if (fport->cb2) {
	    pj_bool_t no_loop = (fport->options & PJMEDIA_AVI_FILE_NO_LOOP);

	    if (!fport->subscribed) {
	    	status = pjmedia_event_subscribe(NULL, &file_on_event,
	    				         fport, fport);
	    	fport->subscribed = (status == PJ_SUCCESS)? PJ_TRUE:
	    			    PJ_FALSE;
	    }

	    if (fport->subscribed && fport->eof != 2) {
	    	pjmedia_event event;

	    	if (no_loop) {
	    	    /* To prevent the callback from being called repeatedly */
	    	    fport->eof = 2;
	    	} else {
	    	    fport->eof = PJ_FALSE;
        	    pj_file_setpos(fport->fd, fport->start_data, PJ_SEEK_SET);
	    	}

	    	pjmedia_event_init(&event, PJMEDIA_EVENT_CALLBACK,
	                      	   NULL, fport);
	    	pjmedia_event_publish(NULL, fport, &event,
	                              PJMEDIA_EVENT_PUBLISH_POST_EVENT);
	    }
	    
	    /* Should not access player port after this since
	     * it might have been destroyed by the callback.
	     */
	    frame->type = PJMEDIA_FRAME_TYPE_NONE;
	    frame->size = 0;
	    
	    return (no_loop? PJ_EEOF: PJ_SUCCESS);

	} else if (fport->cb) {
	    status = (*fport->cb)(this_port, fport->base.port_data.pdata);
	}

	/* If callback returns non PJ_SUCCESS or 'no loop' is specified,
	 * return immediately (and don't try to access player port since
	 * it might have been destroyed by the callback).
	 */
	if ((status != PJ_SUCCESS) ||
            (fport->options & PJMEDIA_AVI_FILE_NO_LOOP)) 
        {
	    frame->type = PJMEDIA_FRAME_TYPE_NONE;
	    frame->size = 0;
	    return PJ_EEOF;
	}

        /* Rewind file */
	PJ_LOG(5,(THIS_FILE, "File port %.*s rewinding..",
		  (int)fport->base.info.name.slen,
		  fport->base.info.name.ptr));
	fport->eof = PJ_FALSE;
        pj_file_setpos(fport->fd, fport->start_data, PJ_SEEK_SET);
    }

    /* For PCMU/A audio stream, reduce frame size to half (temporarily). */
    if (fport->base.info.fmt.type == PJMEDIA_TYPE_AUDIO &&
	(fport->fmt_id == PJMEDIA_FORMAT_PCMA ||
	 fport->fmt_id == PJMEDIA_FORMAT_PCMU))
    {
	frame->size >>= 1;
    }

    /* Fill frame buffer. */
    size_to_read = frame->size;
    do {
        pjmedia_avi_subchunk ch = {0, 0};
        char *cid;
        unsigned stream_id;

        /* We need to read data from the file past the chunk boundary */
        if (fport->size_left > 0 && fport->size_left < size_to_read) {
            status = file_read3(fport->fd, frame->buf, fport->size_left,
                                fport->bits_per_sample, &size_read);
            if (status != PJ_SUCCESS)
                goto on_error2;
            size_to_read -= fport->size_left;
            fport->size_left = 0;
        }

        /* Read new chunk data */
        if (fport->size_left == 0) {
            pj_off_t pos;
            pj_file_getpos(fport->fd, &pos);

            /* Data is padded to the nearest WORD boundary */
            if (fport->pad) {
                status = pj_file_setpos(fport->fd, fport->pad, PJ_SEEK_CUR);
                fport->pad = 0;
            }

            status = file_read(fport->fd, &ch, sizeof(pjmedia_avi_subchunk));
            if (status != PJ_SUCCESS) {
                size_read = 0;
                goto on_error2;
            }

            cid = (char *)&ch.id;
            if (cid[0] >= '0' && cid[0] <= '9' &&
                cid[1] >= '0' && cid[1] <= '9') 
            {
                stream_id = (cid[0] - '0') * 10 + (cid[1] - '0');
            } else
                stream_id = 100;
            fport->pad = (pj_uint8_t)ch.len & 1;

            TRACE_((THIS_FILE, "Reading movi data at pos %u (%x), id: %.*s, "
                               "length: %u", (unsigned long)pos,
                               (unsigned long)pos, 4, cid, ch.len));

            /* We are only interested in data with our stream id */
            if (stream_id != fport->stream_id) {
                if (COMPARE_TAG(ch.id, PJMEDIA_AVI_LIST_TAG))
                    PJ_LOG(5, (THIS_FILE, "Unsupported LIST tag found in "
                                          "the movi data."));
                else if (COMPARE_TAG(ch.id, PJMEDIA_AVI_RIFF_TAG)) {
                    PJ_LOG(3, (THIS_FILE, "Unsupported format: multiple "
                           "AVIs in a single file."));
                    status = AVI_EOF;
                    goto on_error2;
                }

                status = pj_file_setpos(fport->fd, ch.len,
                                        PJ_SEEK_CUR);
                continue;
            }
            fport->size_left = ch.len;
        }

        frame->type = (fport->base.info.fmt.type == PJMEDIA_TYPE_VIDEO ?
                       PJMEDIA_FRAME_TYPE_VIDEO : PJMEDIA_FRAME_TYPE_AUDIO);

        if (frame->type == PJMEDIA_FRAME_TYPE_AUDIO) {
            if (size_to_read > fport->size_left)
                size_to_read = fport->size_left;
            status = file_read3(fport->fd, (char *)frame->buf + frame->size -
                                size_to_read, size_to_read,
                                fport->bits_per_sample, &size_read);
            if (status != PJ_SUCCESS)
                goto on_error2;
            fport->size_left -= size_to_read;
        } else {
            pj_assert(frame->size >= ch.len);
            status = file_read3(fport->fd, frame->buf, ch.len,
                                0, &size_read);
            if (status != PJ_SUCCESS)
                goto on_error2;
            frame->size = ch.len;
            fport->size_left = 0;
        }

        break;

    } while(1);
    frame->timestamp.u64 = fport->next_ts.u64;
    if (frame->type == PJMEDIA_FRAME_TYPE_AUDIO) {

	/* Decode PCMU/A frame */
	if (fport->fmt_id == PJMEDIA_FORMAT_PCMA ||
	    fport->fmt_id == PJMEDIA_FORMAT_PCMU)
	{
	    unsigned i;
	    pj_uint16_t *dst;
	    pj_uint8_t *src;

	    dst = (pj_uint16_t*)frame->buf + frame->size - 1;
	    src = (pj_uint8_t*)frame->buf + frame->size - 1;

	    if (fport->fmt_id == PJMEDIA_FORMAT_PCMU) {
		for (i = 0; i < frame->size; ++i) {
		    *dst-- = (pj_uint16_t) pjmedia_ulaw2linear(*src--);
		}
	    } else {
		for (i = 0; i < frame->size; ++i) {
		    *dst-- = (pj_uint16_t) pjmedia_alaw2linear(*src--);
		}
	    }

	    /* Return back the frame size */
	    frame->size <<= 1;
	}

	if (fport->usec_per_frame) {
	    fport->next_ts.u64 += (fport->usec_per_frame *
				   fport->base.info.fmt.det.aud.clock_rate /
				   1000000);
	} else {
	    fport->next_ts.u64 += (frame->size *
				   fport->base.info.fmt.det.aud.clock_rate /
				   (fport->base.info.fmt.det.aud.avg_bps / 8));
	}
    } else {
	if (fport->usec_per_frame) {
	    fport->next_ts.u64 += (fport->usec_per_frame * VIDEO_CLOCK_RATE /
				   1000000);
	} else {
	    fport->next_ts.u64 += (frame->size * VIDEO_CLOCK_RATE /
				   (fport->base.info.fmt.det.vid.avg_bps / 8));
	}
    }

    return PJ_SUCCESS;

on_error2:
    if (status == AVI_EOF) {
        fport->eof = PJ_TRUE;

        size_to_read -= size_read;
        if (size_to_read == (pj_ssize_t)frame->size) {
            /* Frame is empty */
 	    frame->type = PJMEDIA_FRAME_TYPE_NONE;
	    frame->size = 0;
	    return PJ_EEOF;           
        }
        pj_bzero((char *)frame->buf + frame->size - size_to_read,
                 size_to_read);

        return PJ_SUCCESS;
    }

    return status;
}