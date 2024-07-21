FLAC__bool EncoderSession_init_encoder(EncoderSession *e, encode_options_t options)
{
	const unsigned channels = e->info.channels;
	const unsigned bps = e->info.bits_per_sample - e->info.shift;
	const unsigned sample_rate = e->info.sample_rate;
	FLACDecoderData *flac_decoder_data = (e->format == FORMAT_FLAC || e->format == FORMAT_OGGFLAC)? &e->fmt.flac.client_data : 0;
	FLAC__StreamMetadata padding;
	FLAC__StreamMetadata **metadata = 0;
	static_metadata_t static_metadata;
	unsigned num_metadata = 0, ic;
	FLAC__StreamEncoderInitStatus init_status;
	const FLAC__bool is_cdda = (channels == 1 || channels == 2) && (bps == 16) && (sample_rate == 44100);
	char apodizations[2000];

	FLAC__ASSERT(sizeof(options.pictures)/sizeof(options.pictures[0]) <= 64);

	static_metadata_init(&static_metadata);

	e->replay_gain = options.replay_gain;

	apodizations[0] = '\0';

	if(e->replay_gain) {
		if(channels != 1 && channels != 2) {
			flac__utils_printf(stderr, 1, "%s: ERROR, number of channels (%u) must be 1 or 2 for --replay-gain\n", e->inbasefilename, channels);
			return false;
		}
		if(!grabbag__replaygain_is_valid_sample_frequency(sample_rate)) {
			flac__utils_printf(stderr, 1, "%s: ERROR, invalid sample rate (%u) for --replay-gain\n", e->inbasefilename, sample_rate);
			return false;
		}
		if(options.is_first_file) {
			if(!grabbag__replaygain_init(sample_rate)) {
				flac__utils_printf(stderr, 1, "%s: ERROR initializing ReplayGain stage\n", e->inbasefilename);
				return false;
			}
		}
	}

	if(!parse_cuesheet(&static_metadata.cuesheet, options.cuesheet_filename, e->inbasefilename, sample_rate, is_cdda, e->total_samples_to_encode, e->treat_warnings_as_errors))
		return false;

	if(!convert_to_seek_table_template(options.requested_seek_points, options.num_requested_seek_points, options.cued_seekpoints? static_metadata.cuesheet : 0, e)) {
		flac__utils_printf(stderr, 1, "%s: ERROR allocating memory for seek table\n", e->inbasefilename);
		static_metadata_clear(&static_metadata);
		return false;
	}

	/* build metadata */
	if(flac_decoder_data) {
		/*
		 * we're encoding from FLAC so we will use the FLAC file's
		 * metadata as the basis for the encoded file
		 */
		{
			unsigned i;
			/*
			 * first handle pictures: simple append any --pictures
			 * specified.
			 */
			for(i = 0; i < options.num_pictures; i++) {
				FLAC__StreamMetadata *pic = FLAC__metadata_object_clone(options.pictures[i]);
				if(0 == pic) {
					flac__utils_printf(stderr, 1, "%s: ERROR allocating memory for PICTURE block\n", e->inbasefilename);
					static_metadata_clear(&static_metadata);
					return false;
				}
				flac_decoder_data->metadata_blocks[flac_decoder_data->num_metadata_blocks++] = pic;
			}
		}
		{
			/*
			 * next handle vorbis comment: if any tags were specified
			 * or there is no existing vorbis comment, we create a
			 * new vorbis comment (discarding any existing one); else
			 * we keep the existing one.  also need to make sure to
			 * propagate any channel mask tag.
			 */
			/* @@@ change to append -T values from options.vorbis_comment if input has VC already? */
			size_t i, j;
			FLAC__bool vc_found = false;
			for(i = 0, j = 0; i < flac_decoder_data->num_metadata_blocks; i++) {
				if(flac_decoder_data->metadata_blocks[i]->type == FLAC__METADATA_TYPE_VORBIS_COMMENT)
					vc_found = true;
				if(flac_decoder_data->metadata_blocks[i]->type == FLAC__METADATA_TYPE_VORBIS_COMMENT && options.vorbis_comment->data.vorbis_comment.num_comments > 0) {
					(void) flac__utils_get_channel_mask_tag(flac_decoder_data->metadata_blocks[i], &e->info.channel_mask);
					flac__utils_printf(stderr, 1, "%s: WARNING, replacing tags from input FLAC file with those given on the command-line\n", e->inbasefilename);
					if(e->treat_warnings_as_errors) {
						static_metadata_clear(&static_metadata);
						return false;
					}
					FLAC__metadata_object_delete(flac_decoder_data->metadata_blocks[i]);
					flac_decoder_data->metadata_blocks[i] = 0;
				}
				else
					flac_decoder_data->metadata_blocks[j++] = flac_decoder_data->metadata_blocks[i];
			}
			flac_decoder_data->num_metadata_blocks = j;
			if((!vc_found || options.vorbis_comment->data.vorbis_comment.num_comments > 0) && flac_decoder_data->num_metadata_blocks < sizeof(flac_decoder_data->metadata_blocks)/sizeof(flac_decoder_data->metadata_blocks[0])) {
				/* prepend ours */
				FLAC__StreamMetadata *vc = FLAC__metadata_object_clone(options.vorbis_comment);
				if(0 == vc || (e->info.channel_mask && !flac__utils_set_channel_mask_tag(vc, e->info.channel_mask))) {
					flac__utils_printf(stderr, 1, "%s: ERROR allocating memory for VORBIS_COMMENT block\n", e->inbasefilename);
					static_metadata_clear(&static_metadata);
					return false;
				}
				for(i = flac_decoder_data->num_metadata_blocks; i > 1; i--)
					flac_decoder_data->metadata_blocks[i] = flac_decoder_data->metadata_blocks[i-1];
				flac_decoder_data->metadata_blocks[1] = vc;
				flac_decoder_data->num_metadata_blocks++;
			}
		}
		{
			/*
			 * next handle cuesheet: if --cuesheet was specified, use
			 * it; else if file has existing CUESHEET and cuesheet's
			 * lead-out offset is correct, keep it; else no CUESHEET
			 */
			size_t i, j;
			for(i = 0, j = 0; i < flac_decoder_data->num_metadata_blocks; i++) {
				FLAC__bool existing_cuesheet_is_bad = false;
				/* check if existing cuesheet matches the input audio */
				if(flac_decoder_data->metadata_blocks[i]->type == FLAC__METADATA_TYPE_CUESHEET && 0 == static_metadata.cuesheet) {
					const FLAC__StreamMetadata_CueSheet *cs = &flac_decoder_data->metadata_blocks[i]->data.cue_sheet;
					if(e->total_samples_to_encode == 0) {
						flac__utils_printf(stderr, 1, "%s: WARNING, cuesheet in input FLAC file cannot be kept if input size is not known, dropping it...\n", e->inbasefilename);
						if(e->treat_warnings_as_errors) {
							static_metadata_clear(&static_metadata);
							return false;
						}
						existing_cuesheet_is_bad = true;
					}
					else if(e->total_samples_to_encode != cs->tracks[cs->num_tracks-1].offset) {
						flac__utils_printf(stderr, 1, "%s: WARNING, lead-out offset of cuesheet in input FLAC file does not match input length, dropping existing cuesheet...\n", e->inbasefilename);
						if(e->treat_warnings_as_errors) {
							static_metadata_clear(&static_metadata);
							return false;
						}
						existing_cuesheet_is_bad = true;
					}
				}
				if(flac_decoder_data->metadata_blocks[i]->type == FLAC__METADATA_TYPE_CUESHEET && (existing_cuesheet_is_bad || 0 != static_metadata.cuesheet)) {
					if(0 != static_metadata.cuesheet) {
						flac__utils_printf(stderr, 1, "%s: WARNING, replacing cuesheet in input FLAC file with the one given on the command-line\n", e->inbasefilename);
						if(e->treat_warnings_as_errors) {
							static_metadata_clear(&static_metadata);
							return false;
						}
					}
					FLAC__metadata_object_delete(flac_decoder_data->metadata_blocks[i]);
					flac_decoder_data->metadata_blocks[i] = 0;
				}
				else
					flac_decoder_data->metadata_blocks[j++] = flac_decoder_data->metadata_blocks[i];
			}
			flac_decoder_data->num_metadata_blocks = j;
			if(0 != static_metadata.cuesheet && flac_decoder_data->num_metadata_blocks < sizeof(flac_decoder_data->metadata_blocks)/sizeof(flac_decoder_data->metadata_blocks[0])) {
				/* prepend ours */
				FLAC__StreamMetadata *cs = FLAC__metadata_object_clone(static_metadata.cuesheet);
				if(0 == cs) {
					flac__utils_printf(stderr, 1, "%s: ERROR allocating memory for CUESHEET block\n", e->inbasefilename);
					static_metadata_clear(&static_metadata);
					return false;
				}
				for(i = flac_decoder_data->num_metadata_blocks; i > 1; i--)
					flac_decoder_data->metadata_blocks[i] = flac_decoder_data->metadata_blocks[i-1];
				flac_decoder_data->metadata_blocks[1] = cs;
				flac_decoder_data->num_metadata_blocks++;
			}
		}
		{
			/*
			 * next handle seektable: if -S- was specified, no
			 * SEEKTABLE; else if -S was specified, use it/them;
			 * else if file has existing SEEKTABLE and input size is
			 * preserved (no --skip/--until/etc specified), keep it;
			 * else use default seektable options
			 *
			 * note: meanings of num_requested_seek_points:
			 *  -1 : no -S option given, default to some value
			 *   0 : -S- given (no seektable)
			 *  >0 : one or more -S options given
			 */
			size_t i, j;
			FLAC__bool existing_seektable = false;
			for(i = 0, j = 0; i < flac_decoder_data->num_metadata_blocks; i++) {
				if(flac_decoder_data->metadata_blocks[i]->type == FLAC__METADATA_TYPE_SEEKTABLE)
					existing_seektable = true;
				if(flac_decoder_data->metadata_blocks[i]->type == FLAC__METADATA_TYPE_SEEKTABLE && (e->total_samples_to_encode != flac_decoder_data->metadata_blocks[0]->data.stream_info.total_samples || options.num_requested_seek_points >= 0)) {
					if(options.num_requested_seek_points > 0) {
						flac__utils_printf(stderr, 1, "%s: WARNING, replacing seektable in input FLAC file with the one given on the command-line\n", e->inbasefilename);
						if(e->treat_warnings_as_errors) {
							static_metadata_clear(&static_metadata);
							return false;
						}
					}
					else if(options.num_requested_seek_points == 0)
						; /* no warning, silently delete existing SEEKTABLE since user specified --no-seektable (-S-) */
					else {
						flac__utils_printf(stderr, 1, "%s: WARNING, can't use existing seektable in input FLAC since the input size is changing or unknown, dropping existing SEEKTABLE block...\n", e->inbasefilename);
						if(e->treat_warnings_as_errors) {
							static_metadata_clear(&static_metadata);
							return false;
						}
					}
					FLAC__metadata_object_delete(flac_decoder_data->metadata_blocks[i]);
					flac_decoder_data->metadata_blocks[i] = 0;
					existing_seektable = false;
				}
				else
					flac_decoder_data->metadata_blocks[j++] = flac_decoder_data->metadata_blocks[i];
			}
			flac_decoder_data->num_metadata_blocks = j;
			if((options.num_requested_seek_points > 0 || (options.num_requested_seek_points < 0 && !existing_seektable)) && flac_decoder_data->num_metadata_blocks < sizeof(flac_decoder_data->metadata_blocks)/sizeof(flac_decoder_data->metadata_blocks[0])) {
				/* prepend ours */
				FLAC__StreamMetadata *st = FLAC__metadata_object_clone(e->seek_table_template);
				if(0 == st) {
					flac__utils_printf(stderr, 1, "%s: ERROR allocating memory for SEEKTABLE block\n", e->inbasefilename);
					static_metadata_clear(&static_metadata);
					return false;
				}
				for(i = flac_decoder_data->num_metadata_blocks; i > 1; i--)
					flac_decoder_data->metadata_blocks[i] = flac_decoder_data->metadata_blocks[i-1];
				flac_decoder_data->metadata_blocks[1] = st;
				flac_decoder_data->num_metadata_blocks++;
			}
		}
		{
			/*
			 * finally handle padding: if --no-padding was specified,
			 * then delete all padding; else if -P was specified,
			 * use that instead of existing padding (if any); else
			 * if existing file has padding, move all existing
			 * padding blocks to one padding block at the end; else
			 * use default padding.
			 */
			int p = -1;
			size_t i, j;
			for(i = 0, j = 0; i < flac_decoder_data->num_metadata_blocks; i++) {
				if(flac_decoder_data->metadata_blocks[i]->type == FLAC__METADATA_TYPE_PADDING) {
					if(p < 0)
						p = 0;
					p += flac_decoder_data->metadata_blocks[i]->length;
					FLAC__metadata_object_delete(flac_decoder_data->metadata_blocks[i]);
					flac_decoder_data->metadata_blocks[i] = 0;
				}
				else
					flac_decoder_data->metadata_blocks[j++] = flac_decoder_data->metadata_blocks[i];
			}
			flac_decoder_data->num_metadata_blocks = j;
			if(options.padding > 0)
				p = options.padding;
			if(p < 0)
				p = e->total_samples_to_encode / sample_rate < 20*60? FLAC_ENCODE__DEFAULT_PADDING : FLAC_ENCODE__DEFAULT_PADDING*8;
			if(p > 0)
				p += (e->replay_gain ? GRABBAG__REPLAYGAIN_MAX_TAG_SPACE_REQUIRED : 0);
			if(options.padding != 0) {
				if(p > 0 && flac_decoder_data->num_metadata_blocks < sizeof(flac_decoder_data->metadata_blocks)/sizeof(flac_decoder_data->metadata_blocks[0])) {
					flac_decoder_data->metadata_blocks[flac_decoder_data->num_metadata_blocks] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING);
					if(0 == flac_decoder_data->metadata_blocks[flac_decoder_data->num_metadata_blocks]) {
						flac__utils_printf(stderr, 1, "%s: ERROR allocating memory for PADDING block\n", e->inbasefilename);
						static_metadata_clear(&static_metadata);
						return false;
					}
					flac_decoder_data->metadata_blocks[flac_decoder_data->num_metadata_blocks]->is_last = false; /* the encoder will set this for us */
					flac_decoder_data->metadata_blocks[flac_decoder_data->num_metadata_blocks]->length = p;
					flac_decoder_data->num_metadata_blocks++;
				}
			}
		}
		metadata = &flac_decoder_data->metadata_blocks[1]; /* don't include STREAMINFO */
		num_metadata = flac_decoder_data->num_metadata_blocks - 1;
	}
	else {
		/*
		 * we're not encoding from FLAC so we will build the metadata
		 * from scratch
		 */
		const foreign_metadata_t *foreign_metadata = EncoderSession_format_is_iff(e)? options.format_options.iff.foreign_metadata : 0;
		unsigned i;

		if(e->seek_table_template->data.seek_table.num_points > 0) {
			e->seek_table_template->is_last = false; /* the encoder will set this for us */
			static_metadata_append(&static_metadata, e->seek_table_template, /*needs_delete=*/false);
		}
		if(0 != static_metadata.cuesheet)
			static_metadata_append(&static_metadata, static_metadata.cuesheet, /*needs_delete=*/false);
		if(e->info.channel_mask) {
			if(!flac__utils_set_channel_mask_tag(options.vorbis_comment, e->info.channel_mask)) {
				flac__utils_printf(stderr, 1, "%s: ERROR adding channel mask tag\n", e->inbasefilename);
				static_metadata_clear(&static_metadata);
				return false;
			}
		}
		static_metadata_append(&static_metadata, options.vorbis_comment, /*needs_delete=*/false);
		for(i = 0; i < options.num_pictures; i++)
			static_metadata_append(&static_metadata, options.pictures[i], /*needs_delete=*/false);
		if(foreign_metadata) {
			for(i = 0; i < foreign_metadata->num_blocks; i++) {
				FLAC__StreamMetadata *p = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING);
				if(!p) {
					flac__utils_printf(stderr, 1, "%s: ERROR: out of memory\n", e->inbasefilename);
					static_metadata_clear(&static_metadata);
					return false;
				}
				static_metadata_append(&static_metadata, p, /*needs_delete=*/true);
				static_metadata.metadata[static_metadata.num_metadata-1]->length = FLAC__STREAM_METADATA_APPLICATION_ID_LEN/8 + foreign_metadata->blocks[i].size;
			}
		}
		if(options.padding != 0) {
			padding.is_last = false; /* the encoder will set this for us */
			padding.type = FLAC__METADATA_TYPE_PADDING;
			padding.length = (unsigned)(options.padding>0? options.padding : (e->total_samples_to_encode / sample_rate < 20*60? FLAC_ENCODE__DEFAULT_PADDING : FLAC_ENCODE__DEFAULT_PADDING*8)) + (e->replay_gain ? GRABBAG__REPLAYGAIN_MAX_TAG_SPACE_REQUIRED : 0);
			static_metadata_append(&static_metadata, &padding, /*needs_delete=*/false);
		}
		metadata = static_metadata.metadata;
		num_metadata = static_metadata.num_metadata;
	}

	/* check for a few things that have not already been checked.  the
	 * FLAC__stream_encoder_init*() will check it but only return
	 * FLAC__STREAM_ENCODER_INIT_STATUS_INVALID_METADATA so we check some
	 * up front to give a better error message.
	 */
	if(!verify_metadata(e, metadata, num_metadata)) {
		static_metadata_clear(&static_metadata);
		return false;
	}

	FLAC__stream_encoder_set_verify(e->encoder, options.verify);
	FLAC__stream_encoder_set_streamable_subset(e->encoder, !options.lax);
	FLAC__stream_encoder_set_channels(e->encoder, channels);
	FLAC__stream_encoder_set_bits_per_sample(e->encoder, bps);
	FLAC__stream_encoder_set_sample_rate(e->encoder, sample_rate);
	for(ic = 0; ic < options.num_compression_settings; ic++) {
		switch(options.compression_settings[ic].type) {
			case CST_BLOCKSIZE:
				FLAC__stream_encoder_set_blocksize(e->encoder, options.compression_settings[ic].value.t_unsigned);
				break;
			case CST_COMPRESSION_LEVEL:
				FLAC__stream_encoder_set_compression_level(e->encoder, options.compression_settings[ic].value.t_unsigned);
				apodizations[0] = '\0';
				break;
			case CST_DO_MID_SIDE:
				FLAC__stream_encoder_set_do_mid_side_stereo(e->encoder, options.compression_settings[ic].value.t_bool);
				break;
			case CST_LOOSE_MID_SIDE:
				FLAC__stream_encoder_set_loose_mid_side_stereo(e->encoder, options.compression_settings[ic].value.t_bool);
				break;
			case CST_APODIZATION:
				if(strlen(apodizations)+strlen(options.compression_settings[ic].value.t_string)+2 >= sizeof(apodizations)) {
					flac__utils_printf(stderr, 1, "%s: ERROR: too many apodization functions requested\n", e->inbasefilename);
					static_metadata_clear(&static_metadata);
					return false;
				}
				else {
					safe_strncat(apodizations, options.compression_settings[ic].value.t_string, sizeof(apodizations));
					safe_strncat(apodizations, ";", sizeof(apodizations));
				}
				break;
			case CST_MAX_LPC_ORDER:
				FLAC__stream_encoder_set_max_lpc_order(e->encoder, options.compression_settings[ic].value.t_unsigned);
				break;
			case CST_QLP_COEFF_PRECISION:
				FLAC__stream_encoder_set_qlp_coeff_precision(e->encoder, options.compression_settings[ic].value.t_unsigned);
				break;
			case CST_DO_QLP_COEFF_PREC_SEARCH:
				FLAC__stream_encoder_set_do_qlp_coeff_prec_search(e->encoder, options.compression_settings[ic].value.t_bool);
				break;
			case CST_DO_ESCAPE_CODING:
				FLAC__stream_encoder_set_do_escape_coding(e->encoder, options.compression_settings[ic].value.t_bool);
				break;
			case CST_DO_EXHAUSTIVE_MODEL_SEARCH:
				FLAC__stream_encoder_set_do_exhaustive_model_search(e->encoder, options.compression_settings[ic].value.t_bool);
				break;
			case CST_MIN_RESIDUAL_PARTITION_ORDER:
				FLAC__stream_encoder_set_min_residual_partition_order(e->encoder, options.compression_settings[ic].value.t_unsigned);
				break;
			case CST_MAX_RESIDUAL_PARTITION_ORDER:
				FLAC__stream_encoder_set_max_residual_partition_order(e->encoder, options.compression_settings[ic].value.t_unsigned);
				break;
			case CST_RICE_PARAMETER_SEARCH_DIST:
				FLAC__stream_encoder_set_rice_parameter_search_dist(e->encoder, options.compression_settings[ic].value.t_unsigned);
				break;
		}
	}
	if(*apodizations)
		FLAC__stream_encoder_set_apodization(e->encoder, apodizations);
	FLAC__stream_encoder_set_total_samples_estimate(e->encoder, e->total_samples_to_encode);
	FLAC__stream_encoder_set_metadata(e->encoder, (num_metadata > 0)? metadata : 0, num_metadata);

	FLAC__stream_encoder_disable_constant_subframes(e->encoder, options.debug.disable_constant_subframes);
	FLAC__stream_encoder_disable_fixed_subframes(e->encoder, options.debug.disable_fixed_subframes);
	FLAC__stream_encoder_disable_verbatim_subframes(e->encoder, options.debug.disable_verbatim_subframes);
	if(!options.debug.do_md5) {
		flac__utils_printf(stderr, 1, "%s: WARNING, MD5 computation disabled, resulting file will not have MD5 sum\n", e->inbasefilename);
		if(e->treat_warnings_as_errors) {
			static_metadata_clear(&static_metadata);
			return false;
		}
		FLAC__stream_encoder_set_do_md5(e->encoder, false);
	}

#if FLAC__HAS_OGG
	if(e->use_ogg) {
		FLAC__stream_encoder_set_ogg_serial_number(e->encoder, options.serial_number);

		init_status = FLAC__stream_encoder_init_ogg_file(e->encoder, e->is_stdout? 0 : e->outfilename, encoder_progress_callback, /*client_data=*/e);
	}
	else
#endif
	{
		init_status = FLAC__stream_encoder_init_file(e->encoder, e->is_stdout? 0 : e->outfilename, encoder_progress_callback, /*client_data=*/e);
	}

	if(init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
		print_error_with_init_status(e, "ERROR initializing encoder", init_status);
		if(FLAC__stream_encoder_get_state(e->encoder) != FLAC__STREAM_ENCODER_IO_ERROR)
			e->outputfile_opened = true;
		static_metadata_clear(&static_metadata);
		return false;
	}
	else
		e->outputfile_opened = true;

	e->stats_frames_interval =
		(FLAC__stream_encoder_get_do_exhaustive_model_search(e->encoder) && FLAC__stream_encoder_get_do_qlp_coeff_prec_search(e->encoder))? 0x1f :
		(FLAC__stream_encoder_get_do_exhaustive_model_search(e->encoder) || FLAC__stream_encoder_get_do_qlp_coeff_prec_search(e->encoder))? 0x3f :
		0xff;

	static_metadata_clear(&static_metadata);

	return true;
}