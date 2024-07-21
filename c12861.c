static GF_Err isoffin_setup(GF_Filter *filter, ISOMReader *read)
{
	char szURL[2048];
	char *tmp, *src;
	GF_Err e;
	const GF_PropertyValue *prop;
	if (!read) return GF_SERVICE_ERROR;

	if (read->pid) {
		prop = gf_filter_pid_get_property(read->pid, GF_PROP_PID_FILEPATH);
		assert(prop);
		src = prop->value.string;
	} else {
		src = read->src;
	}
	if (!src)  return GF_SERVICE_ERROR;

	read->src_crc = gf_crc_32(src, (u32) strlen(src));

	strcpy(szURL, src);
	tmp = gf_file_ext_start(szURL);
	if (tmp) {
		Bool truncate = GF_TRUE;
		tmp = strchr(tmp, '#');
		if (!tmp && read->pid) {
			prop = gf_filter_pid_get_property(read->pid, GF_PROP_PID_URL);
			if (prop && prop->value.string) {
				tmp = gf_file_ext_start(prop->value.string);
				if (tmp) tmp = strchr(tmp, '#');
				truncate = GF_FALSE;
			}
		}
		if (tmp) {
			if (!strnicmp(tmp, "#audio", 6)) {
				read->play_only_first_media = GF_ISOM_MEDIA_AUDIO;
			} else if (!strnicmp(tmp, "#video", 6)) {
				read->play_only_first_media = GF_ISOM_MEDIA_VISUAL;
			} else if (!strnicmp(tmp, "#auxv", 5)) {
				read->play_only_first_media = GF_ISOM_MEDIA_AUXV;
			} else if (!strnicmp(tmp, "#pict", 5)) {
				read->play_only_first_media = GF_ISOM_MEDIA_PICT;
			} else if (!strnicmp(tmp, "#text", 5)) {
				read->play_only_first_media = GF_ISOM_MEDIA_TEXT;
			} else if (!strnicmp(tmp, "#trackID=", 9)) {
				read->play_only_track_id = atoi(tmp+9);
			} else if (!strnicmp(tmp, "#ID=", 4)) {
				read->play_only_track_id = atoi(tmp+4);
			} else {
				read->play_only_track_id = atoi(tmp+1);
			}
			if (truncate) tmp[0] = 0;
		}
	}

	if (! isor_is_local(szURL)) {
		return GF_NOT_SUPPORTED;
	}
	read->start_range = read->end_range = 0;
	prop = gf_filter_pid_get_property(read->pid, GF_PROP_PID_FILE_RANGE);
	if (prop) {
		read->start_range = prop->value.lfrac.num;
		read->end_range = prop->value.lfrac.den;
	}

	read->missing_bytes = 0;
	e = gf_isom_open_progressive(szURL, read->start_range, read->end_range, read->sigfrag, &read->mov, &read->missing_bytes);

	if (e == GF_ISOM_INCOMPLETE_FILE) {
		read->moov_not_loaded = 1;
		return GF_OK;
	}

	read->input_loaded = GF_TRUE;
	//if missing bytes is set, file is incomplete, check if cache is complete
	if (read->missing_bytes) {
		read->input_loaded = GF_FALSE;
		prop = gf_filter_pid_get_property(read->pid, GF_PROP_PID_FILE_CACHED);
		if (prop && prop->value.boolean)
			read->input_loaded = GF_TRUE;
	}

	if (e != GF_OK) {
		GF_LOG(GF_LOG_ERROR, GF_LOG_CONTAINER, ("[IsoMedia] error while opening %s, error=%s\n", szURL,gf_error_to_string(e)));
		gf_filter_setup_failure(filter, e);
		return e;
	}
	read->frag_type = gf_isom_is_fragmented(read->mov) ? 1 : 0;
    if (!read->frag_type && read->sigfrag) {
        e = GF_BAD_PARAM;
        GF_LOG(GF_LOG_ERROR, GF_LOG_CONTAINER, ("[IsoMedia] sigfrag requested but file %s is not fragmented\n", szURL));
        gf_filter_setup_failure(filter, e);
        return e;
    }
    
	read->timescale = gf_isom_get_timescale(read->mov);
	if (!read->input_loaded && read->frag_type)
		read->refresh_fragmented = GF_TRUE;

	if (read->strtxt)
		gf_isom_text_set_streaming_mode(read->mov, GF_TRUE);

	return isor_declare_objects(read);
}