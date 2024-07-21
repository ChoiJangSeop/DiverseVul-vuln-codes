int flac__encode_file(FILE *infile, FLAC__off_t infilesize, const char *infilename, const char *outfilename, const FLAC__byte *lookahead, unsigned lookahead_length, encode_options_t options)
{
	EncoderSession encoder_session;
	size_t channel_map[FLAC__MAX_CHANNELS];
	int info_align_carry = -1, info_align_zero = -1;

	if(!EncoderSession_construct(&encoder_session, options, infilesize, infile, infilename, outfilename, lookahead, lookahead_length))
		return 1;

	/* initialize default channel map that preserves channel order */
	{
		size_t i;
		for(i = 0; i < sizeof(channel_map)/sizeof(channel_map[0]); i++)
			channel_map[i] = i;
	}

	/* read foreign metadata if requested */
	if(EncoderSession_format_is_iff(&encoder_session) && options.format_options.iff.foreign_metadata) {
		const char *error;
		if(!(
			options.format == FORMAT_WAVE || options.format == FORMAT_RF64?
				flac__foreign_metadata_read_from_wave(options.format_options.iff.foreign_metadata, infilename, &error) :
			options.format == FORMAT_WAVE64?
				flac__foreign_metadata_read_from_wave64(options.format_options.iff.foreign_metadata, infilename, &error) :
				flac__foreign_metadata_read_from_aiff(options.format_options.iff.foreign_metadata, infilename, &error)
		)) {
			flac__utils_printf(stderr, 1, "%s: ERROR reading foreign metadata: %s\n", encoder_session.inbasefilename, error);
			return EncoderSession_finish_error(&encoder_session);
		}
	}

	/* initialize encoder session with info about the audio (channels/bps/resolution/endianness/etc) */
	switch(options.format) {
		case FORMAT_RAW:
			if(!get_sample_info_raw(&encoder_session, options))
				return EncoderSession_finish_error(&encoder_session);
			break;
		case FORMAT_WAVE:
		case FORMAT_WAVE64:
		case FORMAT_RF64:
			if(!get_sample_info_wave(&encoder_session, options))
				return EncoderSession_finish_error(&encoder_session);
			break;
		case FORMAT_AIFF:
		case FORMAT_AIFF_C:
			if(!get_sample_info_aiff(&encoder_session, options))
				return EncoderSession_finish_error(&encoder_session);
			break;
		case FORMAT_FLAC:
		case FORMAT_OGGFLAC:
			/*
			 * set up FLAC decoder for the input
			 */
			if (0 == (encoder_session.fmt.flac.decoder = FLAC__stream_decoder_new())) {
				flac__utils_printf(stderr, 1, "%s: ERROR: creating decoder for FLAC input\n", encoder_session.inbasefilename);
				return EncoderSession_finish_error(&encoder_session);
			}
			if(!get_sample_info_flac(&encoder_session))
				return EncoderSession_finish_error(&encoder_session);
			break;
		default:
			FLAC__ASSERT(0);
			/* double protection */
			return EncoderSession_finish_error(&encoder_session);
	}

	/* some more checks */
	if(encoder_session.info.channels == 0 || encoder_session.info.channels > FLAC__MAX_CHANNELS) {
		flac__utils_printf(stderr, 1, "%s: ERROR: unsupported number of channels %u\n", encoder_session.inbasefilename, encoder_session.info.channels);
		return EncoderSession_finish_error(&encoder_session);
	}
	if(!FLAC__format_sample_rate_is_valid(encoder_session.info.sample_rate)) {
		flac__utils_printf(stderr, 1, "%s: ERROR: unsupported sample rate %u\n", encoder_session.inbasefilename, encoder_session.info.sample_rate);
		return EncoderSession_finish_error(&encoder_session);
	}
	if(encoder_session.info.bits_per_sample-encoder_session.info.shift < 4 || encoder_session.info.bits_per_sample-encoder_session.info.shift > 24) {
		flac__utils_printf(stderr, 1, "%s: ERROR: unsupported bits-per-sample %u\n", encoder_session.inbasefilename, encoder_session.info.bits_per_sample-encoder_session.info.shift);
		return EncoderSession_finish_error(&encoder_session);
	}
	if(options.sector_align) {
		if(encoder_session.info.channels != 2) {
			flac__utils_printf(stderr, 1, "%s: ERROR: file has %u channels, must be 2 for --sector-align\n", encoder_session.inbasefilename, encoder_session.info.channels);
			return EncoderSession_finish_error(&encoder_session);
		}
		if(encoder_session.info.sample_rate != 44100) {
			flac__utils_printf(stderr, 1, "%s: ERROR: file's sample rate is %u, must be 44100 for --sector-align\n", encoder_session.inbasefilename, encoder_session.info.sample_rate);
			return EncoderSession_finish_error(&encoder_session);
		}
		if(encoder_session.info.bits_per_sample-encoder_session.info.shift != 16) {
			flac__utils_printf(stderr, 1, "%s: ERROR: file has %u bits-per-sample, must be 16 for --sector-align\n", encoder_session.inbasefilename, encoder_session.info.bits_per_sample-encoder_session.info.shift);
			return EncoderSession_finish_error(&encoder_session);
		}
	}

	{
		FLAC__uint64 total_samples_in_input; /* WATCHOUT: may be 0 to mean "unknown" */
		FLAC__uint64 skip;
		FLAC__uint64 until; /* a value of 0 mean end-of-stream (i.e. --until=-0) */
		unsigned align_remainder = 0;

		switch(options.format) {
			case FORMAT_RAW:
				if(infilesize < 0)
					total_samples_in_input = 0;
				else
					total_samples_in_input = (FLAC__uint64)infilesize / encoder_session.info.bytes_per_wide_sample + *options.align_reservoir_samples;
				break;
			case FORMAT_WAVE:
			case FORMAT_WAVE64:
			case FORMAT_RF64:
			case FORMAT_AIFF:
			case FORMAT_AIFF_C:
				/* truncation in the division removes any padding byte that was counted in encoder_session.fmt.iff.data_bytes */
				total_samples_in_input = encoder_session.fmt.iff.data_bytes / encoder_session.info.bytes_per_wide_sample + *options.align_reservoir_samples;
				break;
			case FORMAT_FLAC:
			case FORMAT_OGGFLAC:
				total_samples_in_input = encoder_session.fmt.flac.client_data.metadata_blocks[0]->data.stream_info.total_samples + *options.align_reservoir_samples;
				break;
			default:
				FLAC__ASSERT(0);
				/* double protection */
				return EncoderSession_finish_error(&encoder_session);
		}

		/*
		 * now that we know the sample rate, canonicalize the
		 * --skip string to an absolute sample number:
		 */
		flac__utils_canonicalize_skip_until_specification(&options.skip_specification, encoder_session.info.sample_rate);
		FLAC__ASSERT(options.skip_specification.value.samples >= 0);
		skip = (FLAC__uint64)options.skip_specification.value.samples;
		FLAC__ASSERT(!options.sector_align || (options.format != FORMAT_FLAC && options.format != FORMAT_OGGFLAC && skip == 0));
		/* *options.align_reservoir_samples will be 0 unless --sector-align is used */
		FLAC__ASSERT(options.sector_align || *options.align_reservoir_samples == 0);

		/*
		 * now that we possibly know the input size, canonicalize the
		 * --until string to an absolute sample number:
		 */
		if(!canonicalize_until_specification(&options.until_specification, encoder_session.inbasefilename, encoder_session.info.sample_rate, skip, total_samples_in_input))
			return EncoderSession_finish_error(&encoder_session);
		until = (FLAC__uint64)options.until_specification.value.samples;
		FLAC__ASSERT(!options.sector_align || until == 0);

		/* adjust encoding parameters based on skip and until values */
		switch(options.format) {
			case FORMAT_RAW:
				infilesize -= (FLAC__off_t)skip * encoder_session.info.bytes_per_wide_sample;
				encoder_session.total_samples_to_encode = total_samples_in_input - skip;
				break;
			case FORMAT_WAVE:
			case FORMAT_WAVE64:
			case FORMAT_RF64:
			case FORMAT_AIFF:
			case FORMAT_AIFF_C:
				encoder_session.fmt.iff.data_bytes -= skip * encoder_session.info.bytes_per_wide_sample;
				if(options.ignore_chunk_sizes) {
					encoder_session.total_samples_to_encode = 0;
					FLAC__ASSERT(0 == until);
				}
				else {
					encoder_session.total_samples_to_encode = total_samples_in_input - skip;
				}
				break;
			case FORMAT_FLAC:
			case FORMAT_OGGFLAC:
				encoder_session.total_samples_to_encode = total_samples_in_input - skip;
				break;
			default:
				FLAC__ASSERT(0);
				/* double protection */
				return EncoderSession_finish_error(&encoder_session);
		}
		if(until > 0) {
			const FLAC__uint64 trim = total_samples_in_input - until;
			FLAC__ASSERT(total_samples_in_input > 0);
			FLAC__ASSERT(!options.sector_align);
			if(options.format == FORMAT_RAW)
				infilesize -= (FLAC__off_t)trim * encoder_session.info.bytes_per_wide_sample;
			else if(EncoderSession_format_is_iff(&encoder_session))
				encoder_session.fmt.iff.data_bytes -= trim * encoder_session.info.bytes_per_wide_sample;
			encoder_session.total_samples_to_encode -= trim;
		}
		if(options.sector_align && (options.format != FORMAT_RAW || infilesize >=0)) { /* for RAW, need to know the filesize */
			FLAC__ASSERT(skip == 0); /* asserted above too, but lest we forget */
			align_remainder = (unsigned)(encoder_session.total_samples_to_encode % 588);
			if(options.is_last_file)
				encoder_session.total_samples_to_encode += (588-align_remainder); /* will pad with zeroes */
			else
				encoder_session.total_samples_to_encode -= align_remainder; /* will stop short and carry over to next file */
		}
		switch(options.format) {
			case FORMAT_RAW:
				encoder_session.unencoded_size = encoder_session.total_samples_to_encode * encoder_session.info.bytes_per_wide_sample;
				break;
			case FORMAT_WAVE:
				/* +44 for the size of the WAVE headers; this is just an estimate for the progress indicator and doesn't need to be exact */
				encoder_session.unencoded_size = encoder_session.total_samples_to_encode * encoder_session.info.bytes_per_wide_sample + 44;
				break;
			case FORMAT_WAVE64:
				/* +44 for the size of the WAVE headers; this is just an estimate for the progress indicator and doesn't need to be exact */
				encoder_session.unencoded_size = encoder_session.total_samples_to_encode * encoder_session.info.bytes_per_wide_sample + 104;
				break;
			case FORMAT_RF64:
				/* +72 for the size of the RF64 headers; this is just an estimate for the progress indicator and doesn't need to be exact */
				encoder_session.unencoded_size = encoder_session.total_samples_to_encode * encoder_session.info.bytes_per_wide_sample + 80;
				break;
			case FORMAT_AIFF:
			case FORMAT_AIFF_C:
				/* +54 for the size of the AIFF headers; this is just an estimate for the progress indicator and doesn't need to be exact */
				encoder_session.unencoded_size = encoder_session.total_samples_to_encode * encoder_session.info.bytes_per_wide_sample + 54;
				break;
			case FORMAT_FLAC:
			case FORMAT_OGGFLAC:
				if(infilesize < 0)
					/* if we don't know, use 0 as hint to progress indicator (which is the only place this is used): */
					encoder_session.unencoded_size = 0;
				else if(skip == 0 && until == 0)
					encoder_session.unencoded_size = (FLAC__uint64)infilesize;
				else if(total_samples_in_input)
					encoder_session.unencoded_size = (FLAC__uint64)infilesize * encoder_session.total_samples_to_encode / total_samples_in_input;
				else
					encoder_session.unencoded_size = (FLAC__uint64)infilesize;
				break;
			default:
				FLAC__ASSERT(0);
				/* double protection */
				return EncoderSession_finish_error(&encoder_session);
		}

		if(encoder_session.total_samples_to_encode == 0) {
			encoder_session.unencoded_size = 0;
			flac__utils_printf(stderr, 2, "(No runtime statistics possible; please wait for encoding to finish...)\n");
		}

		if(options.format == FORMAT_FLAC || options.format == FORMAT_OGGFLAC)
			encoder_session.fmt.flac.client_data.samples_left_to_process = encoder_session.total_samples_to_encode;

		stats_new_file();
		/* init the encoder */
		if(!EncoderSession_init_encoder(&encoder_session, options))
			return EncoderSession_finish_error(&encoder_session);

		/* skip over any samples as requested */
		if(skip > 0) {
			switch(options.format) {
				case FORMAT_RAW:
					{
						unsigned skip_bytes = encoder_session.info.bytes_per_wide_sample * (unsigned)skip;
						if(skip_bytes > lookahead_length) {
							skip_bytes -= lookahead_length;
							lookahead_length = 0;
							if(!fskip_ahead(encoder_session.fin, skip_bytes)) {
								flac__utils_printf(stderr, 1, "%s: ERROR during read while skipping samples\n", encoder_session.inbasefilename);
								return EncoderSession_finish_error(&encoder_session);
							}
						}
						else {
							lookahead += skip_bytes;
							lookahead_length -= skip_bytes;
						}
					}
					break;
				case FORMAT_WAVE:
				case FORMAT_WAVE64:
				case FORMAT_RF64:
				case FORMAT_AIFF:
				case FORMAT_AIFF_C:
					if(!fskip_ahead(encoder_session.fin, skip * encoder_session.info.bytes_per_wide_sample)) {
						flac__utils_printf(stderr, 1, "%s: ERROR during read while skipping samples\n", encoder_session.inbasefilename);
						return EncoderSession_finish_error(&encoder_session);
					}
					break;
				case FORMAT_FLAC:
				case FORMAT_OGGFLAC:
					/*
					 * have to wait until the FLAC encoder is set up for writing
					 * before any seeking in the input FLAC file, because the seek
					 * itself will usually call the decoder's write callback, and
					 * our decoder's write callback passes samples to our FLAC
					 * encoder
					 */
					if(!FLAC__stream_decoder_seek_absolute(encoder_session.fmt.flac.decoder, skip)) {
						flac__utils_printf(stderr, 1, "%s: ERROR while skipping samples, FLAC decoder state = %s\n", encoder_session.inbasefilename, FLAC__stream_decoder_get_resolved_state_string(encoder_session.fmt.flac.decoder));
						return EncoderSession_finish_error(&encoder_session);
					}
					break;
				default:
					FLAC__ASSERT(0);
					/* double protection */
					return EncoderSession_finish_error(&encoder_session);
			}
		}

		/*
		 * first do any samples in the reservoir
		 */
		if(options.sector_align && *options.align_reservoir_samples > 0) {
			FLAC__ASSERT(options.format != FORMAT_FLAC && options.format != FORMAT_OGGFLAC); /* check again */
			if(!EncoderSession_process(&encoder_session, (const FLAC__int32 * const *)options.align_reservoir, *options.align_reservoir_samples)) {
				print_error_with_state(&encoder_session, "ERROR during encoding");
				return EncoderSession_finish_error(&encoder_session);
			}
		}

		/*
		 * decrement infilesize or the data_bytes counter if we need to align the file
		 */
		if(options.sector_align) {
			if(options.is_last_file) {
				*options.align_reservoir_samples = 0;
			}
			else {
				*options.align_reservoir_samples = align_remainder;
				if(options.format == FORMAT_RAW) {
					FLAC__ASSERT(infilesize >= 0);
					infilesize -= (FLAC__off_t)((*options.align_reservoir_samples) * encoder_session.info.bytes_per_wide_sample);
					FLAC__ASSERT(infilesize >= 0);
				}
				else if(EncoderSession_format_is_iff(&encoder_session))
					encoder_session.fmt.iff.data_bytes -= (*options.align_reservoir_samples) * encoder_session.info.bytes_per_wide_sample;
			}
		}

		/*
		 * now do samples from the file
		 */
		switch(options.format) {
			case FORMAT_RAW:
				if(infilesize < 0) {
					size_t bytes_read;
					while(!feof(infile)) {
						if(lookahead_length > 0) {
							FLAC__ASSERT(lookahead_length < CHUNK_OF_SAMPLES * encoder_session.info.bytes_per_wide_sample);
							memcpy(ubuffer.u8, lookahead, lookahead_length);
							bytes_read = fread(ubuffer.u8+lookahead_length, sizeof(unsigned char), CHUNK_OF_SAMPLES * encoder_session.info.bytes_per_wide_sample - lookahead_length, infile) + lookahead_length;
							if(ferror(infile)) {
								flac__utils_printf(stderr, 1, "%s: ERROR during read\n", encoder_session.inbasefilename);
								return EncoderSession_finish_error(&encoder_session);
							}
							lookahead_length = 0;
						}
						else
							bytes_read = fread(ubuffer.u8, sizeof(unsigned char), CHUNK_OF_SAMPLES * encoder_session.info.bytes_per_wide_sample, infile);

						if(bytes_read == 0) {
							if(ferror(infile)) {
								flac__utils_printf(stderr, 1, "%s: ERROR during read\n", encoder_session.inbasefilename);
								return EncoderSession_finish_error(&encoder_session);
							}
						}
						else if(bytes_read % encoder_session.info.bytes_per_wide_sample != 0) {
							flac__utils_printf(stderr, 1, "%s: ERROR: got partial sample\n", encoder_session.inbasefilename);
							return EncoderSession_finish_error(&encoder_session);
						}
						else {
							unsigned wide_samples = bytes_read / encoder_session.info.bytes_per_wide_sample;
							if(!format_input(input_, wide_samples, encoder_session.info.is_big_endian, encoder_session.info.is_unsigned_samples, encoder_session.info.channels, encoder_session.info.bits_per_sample, encoder_session.info.shift, channel_map))
								return EncoderSession_finish_error(&encoder_session);

							if(!EncoderSession_process(&encoder_session, (const FLAC__int32 * const *)input_, wide_samples)) {
								print_error_with_state(&encoder_session, "ERROR during encoding");
								return EncoderSession_finish_error(&encoder_session);
							}
						}
					}
				}
				else {
					size_t bytes_read;
					const FLAC__uint64 max_input_bytes = infilesize;
					FLAC__uint64 total_input_bytes_read = 0;
					while(total_input_bytes_read < max_input_bytes) {
						{
							size_t wanted = (CHUNK_OF_SAMPLES * encoder_session.info.bytes_per_wide_sample);
							wanted = (size_t) min((FLAC__uint64)wanted, max_input_bytes - total_input_bytes_read);

							if(lookahead_length > 0) {
								FLAC__ASSERT(lookahead_length <= wanted);
								memcpy(ubuffer.u8, lookahead, lookahead_length);
								wanted -= lookahead_length;
								bytes_read = lookahead_length;
								if(wanted > 0) {
									bytes_read += fread(ubuffer.u8+lookahead_length, sizeof(unsigned char), wanted, infile);
									if(ferror(infile)) {
										flac__utils_printf(stderr, 1, "%s: ERROR during read\n", encoder_session.inbasefilename);
										return EncoderSession_finish_error(&encoder_session);
									}
								}
								lookahead_length = 0;
							}
							else
								bytes_read = fread(ubuffer.u8, sizeof(unsigned char), wanted, infile);
						}

						if(bytes_read == 0) {
							if(ferror(infile)) {
								flac__utils_printf(stderr, 1, "%s: ERROR during read\n", encoder_session.inbasefilename);
								return EncoderSession_finish_error(&encoder_session);
							}
							else if(feof(infile)) {
								flac__utils_printf(stderr, 1, "%s: WARNING: unexpected EOF; expected %" PRIu64 " samples, got %" PRIu64 " samples\n", encoder_session.inbasefilename, encoder_session.total_samples_to_encode, encoder_session.samples_written);
								if(encoder_session.treat_warnings_as_errors)
									return EncoderSession_finish_error(&encoder_session);
								total_input_bytes_read = max_input_bytes;
							}
						}
						else {
							if(bytes_read % encoder_session.info.bytes_per_wide_sample != 0) {
								flac__utils_printf(stderr, 1, "%s: ERROR: got partial sample\n", encoder_session.inbasefilename);
								return EncoderSession_finish_error(&encoder_session);
							}
							else {
								unsigned wide_samples = bytes_read / encoder_session.info.bytes_per_wide_sample;
								if(!format_input(input_, wide_samples, encoder_session.info.is_big_endian, encoder_session.info.is_unsigned_samples, encoder_session.info.channels, encoder_session.info.bits_per_sample, encoder_session.info.shift, channel_map))
									return EncoderSession_finish_error(&encoder_session);

								if(!EncoderSession_process(&encoder_session, (const FLAC__int32 * const *)input_, wide_samples)) {
									print_error_with_state(&encoder_session, "ERROR during encoding");
									return EncoderSession_finish_error(&encoder_session);
								}
								total_input_bytes_read += bytes_read;
							}
						}
					}
				}
				break;
			case FORMAT_WAVE:
			case FORMAT_WAVE64:
			case FORMAT_RF64:
			case FORMAT_AIFF:
			case FORMAT_AIFF_C:
				while(encoder_session.fmt.iff.data_bytes > 0) {
					const size_t bytes_to_read = (size_t)min(
						encoder_session.fmt.iff.data_bytes,
						(FLAC__uint64)CHUNK_OF_SAMPLES * (FLAC__uint64)encoder_session.info.bytes_per_wide_sample
					);
					size_t bytes_read = fread(ubuffer.u8, sizeof(unsigned char), bytes_to_read, infile);
					if(bytes_read == 0) {
						if(ferror(infile)) {
							flac__utils_printf(stderr, 1, "%s: ERROR during read\n", encoder_session.inbasefilename);
							return EncoderSession_finish_error(&encoder_session);
						}
						else if(feof(infile)) {
							if(options.ignore_chunk_sizes) {
								flac__utils_printf(stderr, 1, "%s: INFO: hit EOF with --ignore-chunk-sizes, got %" PRIu64 " samples\n", encoder_session.inbasefilename, encoder_session.samples_written);
							}
							else {
								flac__utils_printf(stderr, 1, "%s: WARNING: unexpected EOF; expected %" PRIu64 " samples, got %" PRIu64 " samples\n", encoder_session.inbasefilename, encoder_session.total_samples_to_encode, encoder_session.samples_written);
								if(encoder_session.treat_warnings_as_errors)
									return EncoderSession_finish_error(&encoder_session);
							}
							encoder_session.fmt.iff.data_bytes = 0;
						}
					}
					else {
						if(bytes_read % encoder_session.info.bytes_per_wide_sample != 0) {
							flac__utils_printf(stderr, 1, "%s: ERROR: got partial sample\n", encoder_session.inbasefilename);
							return EncoderSession_finish_error(&encoder_session);
						}
						else {
							unsigned wide_samples = bytes_read / encoder_session.info.bytes_per_wide_sample;
							if(!format_input(input_, wide_samples, encoder_session.info.is_big_endian, encoder_session.info.is_unsigned_samples, encoder_session.info.channels, encoder_session.info.bits_per_sample, encoder_session.info.shift, channel_map))
								return EncoderSession_finish_error(&encoder_session);

							if(!EncoderSession_process(&encoder_session, (const FLAC__int32 * const *)input_, wide_samples)) {
								print_error_with_state(&encoder_session, "ERROR during encoding");
								return EncoderSession_finish_error(&encoder_session);
							}
							encoder_session.fmt.iff.data_bytes -= bytes_read;
						}
					}
				}
				break;
			case FORMAT_FLAC:
			case FORMAT_OGGFLAC:
				while(!encoder_session.fmt.flac.client_data.fatal_error && encoder_session.fmt.flac.client_data.samples_left_to_process > 0) {
					/* We can also hit the end of stream without samples_left_to_process
					 * going to 0 if there are errors and continue_through_decode_errors
					 * is on, so we want to break in that case too:
					 */
					if(encoder_session.continue_through_decode_errors && FLAC__stream_decoder_get_state(encoder_session.fmt.flac.decoder) == FLAC__STREAM_DECODER_END_OF_STREAM)
						break;
					if(!FLAC__stream_decoder_process_single(encoder_session.fmt.flac.decoder)) {
						flac__utils_printf(stderr, 1, "%s: ERROR: while decoding FLAC input, state = %s\n", encoder_session.inbasefilename, FLAC__stream_decoder_get_resolved_state_string(encoder_session.fmt.flac.decoder));
						return EncoderSession_finish_error(&encoder_session);
					}
				}
				if(encoder_session.fmt.flac.client_data.fatal_error) {
					flac__utils_printf(stderr, 1, "%s: ERROR: while decoding FLAC input, state = %s\n", encoder_session.inbasefilename, FLAC__stream_decoder_get_resolved_state_string(encoder_session.fmt.flac.decoder));
					return EncoderSession_finish_error(&encoder_session);
				}
				break;
			default:
				FLAC__ASSERT(0);
				/* double protection */
				return EncoderSession_finish_error(&encoder_session);
		}

		/*
		 * now read unaligned samples into reservoir or pad with zeroes if necessary
		 */
		if(options.sector_align) {
			if(options.is_last_file) {
				unsigned wide_samples = 588 - align_remainder;
				if(wide_samples < 588) {
					unsigned channel;

					info_align_zero = wide_samples;
					for(channel = 0; channel < encoder_session.info.channels; channel++)
						memset(input_[channel], 0, sizeof(input_[0][0]) * wide_samples);

					if(!EncoderSession_process(&encoder_session, (const FLAC__int32 * const *)input_, wide_samples)) {
						print_error_with_state(&encoder_session, "ERROR during encoding");
						return EncoderSession_finish_error(&encoder_session);
					}
				}
			}
			else {
				if(*options.align_reservoir_samples > 0) {
					size_t bytes_read;
					FLAC__ASSERT(CHUNK_OF_SAMPLES >= 588);
					bytes_read = fread(ubuffer.u8, sizeof(unsigned char), (*options.align_reservoir_samples) * encoder_session.info.bytes_per_wide_sample, infile);
					if(bytes_read == 0 && ferror(infile)) {
						flac__utils_printf(stderr, 1, "%s: ERROR during read\n", encoder_session.inbasefilename);
						return EncoderSession_finish_error(&encoder_session);
					}
					else if(bytes_read != (*options.align_reservoir_samples) * encoder_session.info.bytes_per_wide_sample) {
						flac__utils_printf(stderr, 1, "%s: WARNING: unexpected EOF; read %" PRIu64 " bytes; expected %" PRIu64 " samples, got %" PRIu64 " samples\n", encoder_session.inbasefilename, bytes_read, encoder_session.total_samples_to_encode, encoder_session.samples_written);
						if(encoder_session.treat_warnings_as_errors)
							return EncoderSession_finish_error(&encoder_session);
					}
					else {
						info_align_carry = *options.align_reservoir_samples;
						if(!format_input(options.align_reservoir, *options.align_reservoir_samples, encoder_session.info.is_big_endian, encoder_session.info.is_unsigned_samples, encoder_session.info.channels, encoder_session.info.bits_per_sample, encoder_session.info.shift, channel_map))
							return EncoderSession_finish_error(&encoder_session);
					}
				}
			}
		}
	}

	return EncoderSession_finish_ok(
		&encoder_session,
		info_align_carry,
		info_align_zero,
		EncoderSession_format_is_iff(&encoder_session)? options.format_options.iff.foreign_metadata : 0,
		options.error_on_compression_fail
	);
}