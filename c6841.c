rf64_read_header (SF_PRIVATE *psf, int *blockalign, int *framesperblock)
{	WAVLIKE_PRIVATE	*wpriv ;
	WAV_FMT		*wav_fmt ;
	sf_count_t riff_size = 0, frame_count = 0, ds64_datalength = 0 ;
	uint32_t marks [2], marker, chunk_size, parsestage = 0 ;
	int error, done = 0, format = 0 ;

	if ((wpriv = psf->container_data) == NULL)
		return SFE_INTERNAL ;
	wav_fmt = &wpriv->wav_fmt ;

	/* Set position to start of file to begin reading header. */
	psf_binheader_readf (psf, "pmmm", 0, &marker, marks, marks + 1) ;
	if (marker != RF64_MARKER || marks [1] != WAVE_MARKER)
		return SFE_RF64_NOT_RF64 ;

	if (marks [0] == FFFF_MARKER)
		psf_log_printf (psf, "%M\n  %M\n", RF64_MARKER, WAVE_MARKER) ;
	else
		psf_log_printf (psf, "%M : 0x%x (should be 0xFFFFFFFF)\n  %M\n", RF64_MARKER, WAVE_MARKER) ;

	while (NOT (done))
	{
		marker = chunk_size = 0 ;
		psf_binheader_readf (psf, "em4", &marker, &chunk_size) ;

		if (marker == 0)
		{	sf_count_t pos = psf_ftell (psf) ;
			psf_log_printf (psf, "Have 0 marker at position %D (0x%x).\n", pos, pos) ;
			break ;
			} ;

		psf_store_read_chunk_u32 (&psf->rchunks, marker, psf_ftell (psf), chunk_size) ;

		switch (marker)
		{	case ds64_MARKER :
				if (parsestage & HAVE_ds64)
				{	psf_log_printf (psf, "*** Second 'ds64' chunk?\n") ;
					break ;
					} ;

				{	unsigned int table_len, bytesread ;

					/* Read ds64 sizes (3 8-byte words). */
					bytesread = psf_binheader_readf (psf, "888", &riff_size, &ds64_datalength, &frame_count) ;

					/* Read table length. */
					bytesread += psf_binheader_readf (psf, "4", &table_len) ;
					/* Skip table for now. (this was "table_len + 4", why?) */
					bytesread += psf_binheader_readf (psf, "j", table_len) ;

					if (chunk_size == bytesread)
						psf_log_printf (psf, "%M : %u\n", marker, chunk_size) ;
					else if (chunk_size >= bytesread + 4)
					{	unsigned int next ;
						psf_binheader_readf (psf, "m", &next) ;
						if (next == fmt_MARKER)
						{	psf_log_printf (psf, "%M : %u (should be %u)\n", marker, chunk_size, bytesread) ;
							psf_binheader_readf (psf, "j", -4) ;
							}
						else
						{	psf_log_printf (psf, "%M : %u\n", marker, chunk_size) ;
							psf_binheader_readf (psf, "j", chunk_size - bytesread - 4) ;
							} ;
						} ;

					if (psf->filelength != riff_size + 8)
						psf_log_printf (psf, "  Riff size : %D (should be %D)\n", riff_size, psf->filelength - 8) ;
					else
						psf_log_printf (psf, "  Riff size : %D\n", riff_size) ;

					psf_log_printf (psf, "  Data size : %D\n", ds64_datalength) ;

					psf_log_printf (psf, "  Frames    : %D\n", frame_count) ;
					psf_log_printf (psf, "  Table length : %u\n", table_len) ;

					} ;
				parsestage |= HAVE_ds64 ;
				break ;

			case fmt_MARKER:
					psf_log_printf (psf, "%M : %u\n", marker, chunk_size) ;
					if ((error = wavlike_read_fmt_chunk (psf, chunk_size)) != 0)
						return error ;
					format = wav_fmt->format ;
					parsestage |= HAVE_fmt ;
					break ;

			case bext_MARKER :
					if ((error = wavlike_read_bext_chunk (psf, chunk_size)) != 0)
						return error ;
					parsestage |= HAVE_bext ;
					break ;

			case cart_MARKER :
					if ((error = wavlike_read_cart_chunk (psf, chunk_size)) != 0)
						return error ;
					parsestage |= HAVE_cart ;
					break ;

			case INFO_MARKER :
			case LIST_MARKER :
					if ((error = wavlike_subchunk_parse (psf, marker, chunk_size)) != 0)
						return error ;
					parsestage |= HAVE_other ;
					break ;

			case PEAK_MARKER :
					if ((parsestage & (HAVE_ds64 | HAVE_fmt)) != (HAVE_ds64 | HAVE_fmt))
						return SFE_RF64_PEAK_B4_FMT ;

					parsestage |= HAVE_PEAK ;

					psf_log_printf (psf, "%M : %u\n", marker, chunk_size) ;
					if ((error = wavlike_read_peak_chunk (psf, chunk_size)) != 0)
						return error ;
					psf->peak_info->peak_loc = ((parsestage & HAVE_data) == 0) ? SF_PEAK_START : SF_PEAK_END ;
					break ;

			case data_MARKER :
				/* see wav for more sophisticated parsing -> implement state machine with parsestage */

				if (HAVE_CHUNK (HAVE_ds64))
				{	if (chunk_size == 0xffffffff)
						psf_log_printf (psf, "%M : 0x%x\n", marker, chunk_size) ;
					else
						psf_log_printf (psf, "%M : 0x%x (should be 0xffffffff\n", marker, chunk_size) ;
					psf->datalength = ds64_datalength ;
					}
				else
				{	if (chunk_size == 0xffffffff)
					{	psf_log_printf (psf, "%M : 0x%x\n", marker, chunk_size) ;
						psf_log_printf (psf, "  *** Data length not specified no 'ds64' chunk.\n") ;
						}
					else
					{	psf_log_printf (psf, "%M : 0x%x\n**** Weird, RF64 file without a 'ds64' chunk and no valid 'data' size.\n", marker, chunk_size) ;
						psf->datalength = chunk_size ;
						} ;
					} ;

				psf->dataoffset = psf_ftell (psf) ;

				if (psf->dataoffset > 0)
				{	if (chunk_size == 0 && riff_size == 8 && psf->filelength > 44)
					{	psf_log_printf (psf, "  *** Looks like a WAV file which wasn't closed properly. Fixing it.\n") ;
						psf->datalength = psf->filelength - psf->dataoffset ;
						} ;

					/* Only set dataend if there really is data at the end. */
					if (psf->datalength + psf->dataoffset < psf->filelength)
						psf->dataend = psf->datalength + psf->dataoffset ;

					if (NOT (psf->sf.seekable) || psf->dataoffset < 0)
						break ;

					/* Seek past data and continue reading header. */
					psf_fseek (psf, psf->datalength, SEEK_CUR) ;

					if (psf_ftell (psf) != psf->datalength + psf->dataoffset)
						psf_log_printf (psf, "  *** psf_fseek past end error ***\n") ;
					} ;
				break ;

			case JUNK_MARKER :
			case PAD_MARKER :
				psf_log_printf (psf, "%M : %d\n", marker, chunk_size) ;
				psf_binheader_readf (psf, "j", chunk_size) ;
				break ;

			default :
					if (chunk_size >= 0xffff0000)
					{	psf_log_printf (psf, "*** Unknown chunk marker (%X) at position %D with length %u. Exiting parser.\n", marker, psf_ftell (psf) - 8, chunk_size) ;
						done = SF_TRUE ;
						break ;
						} ;

					if (isprint ((marker >> 24) & 0xFF) && isprint ((marker >> 16) & 0xFF)
						&& isprint ((marker >> 8) & 0xFF) && isprint (marker & 0xFF))
					{	psf_log_printf (psf, "*** %M : %d (unknown marker)\n", marker, chunk_size) ;
						psf_binheader_readf (psf, "j", chunk_size) ;
						break ;
						} ;
					if (psf_ftell (psf) & 0x03)
					{	psf_log_printf (psf, "  Unknown chunk marker at position 0x%x. Resynching.\n", chunk_size - 4) ;
						psf_binheader_readf (psf, "j", -3) ;
						break ;
						} ;
					psf_log_printf (psf, "*** Unknown chunk marker (0x%X) at position 0x%X. Exiting parser.\n", marker, psf_ftell (psf) - 4) ;
					done = SF_TRUE ;
					break ;
			} ;	/* switch (marker) */

		/* The 'data' chunk, a chunk size of 0xffffffff means that the 'data' chunk size
		** is actually given by the ds64_datalength field.
		*/
		if (marker != data_MARKER && chunk_size >= psf->filelength)
		{	psf_log_printf (psf, "*** Chunk size %u > file length %D. Exiting parser.\n", chunk_size, psf->filelength) ;
			break ;
			} ;

		if (psf_ftell (psf) >= psf->filelength - SIGNED_SIZEOF (marker))
		{	psf_log_printf (psf, "End\n") ;
			break ;
			} ;
		} ;

	if (psf->dataoffset <= 0)
		return SFE_RF64_NO_DATA ;

	if (psf->sf.channels < 1)
		return SFE_CHANNEL_COUNT_ZERO ;

	if (psf->sf.channels >= SF_MAX_CHANNELS)
		return SFE_CHANNEL_COUNT ;

	/* WAVs can be little or big endian */
	psf->endian = psf->rwf_endian ;

	psf_fseek (psf, psf->dataoffset, SEEK_SET) ;

	if (psf->is_pipe == 0)
	{	/*
		** Check for 'wvpk' at the start of the DATA section. Not able to
		** handle this.
		*/
		psf_binheader_readf (psf, "4", &marker) ;
		if (marker == wvpk_MARKER || marker == OggS_MARKER)
			return SFE_WAV_WVPK_DATA ;
		} ;

	/* Seek to start of DATA section. */
	psf_fseek (psf, psf->dataoffset, SEEK_SET) ;

	if (psf->blockwidth)
	{	if (psf->filelength - psf->dataoffset < psf->datalength)
			psf->sf.frames = (psf->filelength - psf->dataoffset) / psf->blockwidth ;
		else
			psf->sf.frames = psf->datalength / psf->blockwidth ;
		} ;

	if (frame_count != psf->sf.frames)
		psf_log_printf (psf, "*** Calculated frame count %d does not match value from 'ds64' chunk of %d.\n", psf->sf.frames, frame_count) ;

	switch (format)
	{
		case WAVE_FORMAT_EXTENSIBLE :

			/* with WAVE_FORMAT_EXTENSIBLE the psf->sf.format field is already set. We just have to set the major to rf64 */
			psf->sf.format = (psf->sf.format & ~SF_FORMAT_TYPEMASK) | SF_FORMAT_RF64 ;

			if (psf->sf.format == (SF_FORMAT_WAVEX | SF_FORMAT_MS_ADPCM))
			{	*blockalign = wav_fmt->msadpcm.blockalign ;
				*framesperblock = wav_fmt->msadpcm.samplesperblock ;
				} ;
			break ;

		case WAVE_FORMAT_PCM :
					psf->sf.format = SF_FORMAT_RF64 | u_bitwidth_to_subformat (psf->bytewidth * 8) ;
					break ;

		case WAVE_FORMAT_MULAW :
		case IBM_FORMAT_MULAW :
					psf->sf.format = (SF_FORMAT_RF64 | SF_FORMAT_ULAW) ;
					break ;

		case WAVE_FORMAT_ALAW :
		case IBM_FORMAT_ALAW :
					psf->sf.format = (SF_FORMAT_RF64 | SF_FORMAT_ALAW) ;
					break ;

		case WAVE_FORMAT_MS_ADPCM :
					psf->sf.format = (SF_FORMAT_RF64 | SF_FORMAT_MS_ADPCM) ;
					*blockalign = wav_fmt->msadpcm.blockalign ;
					*framesperblock = wav_fmt->msadpcm.samplesperblock ;
					break ;

		case WAVE_FORMAT_IMA_ADPCM :
					psf->sf.format = (SF_FORMAT_RF64 | SF_FORMAT_IMA_ADPCM) ;
					*blockalign = wav_fmt->ima.blockalign ;
					*framesperblock = wav_fmt->ima.samplesperblock ;
					break ;

		case WAVE_FORMAT_GSM610 :
					psf->sf.format = (SF_FORMAT_RF64 | SF_FORMAT_GSM610) ;
					break ;

		case WAVE_FORMAT_IEEE_FLOAT :
					psf->sf.format = SF_FORMAT_RF64 ;
					psf->sf.format |= (psf->bytewidth == 8) ? SF_FORMAT_DOUBLE : SF_FORMAT_FLOAT ;
					break ;

		case WAVE_FORMAT_G721_ADPCM :
					psf->sf.format = SF_FORMAT_RF64 | SF_FORMAT_G721_32 ;
					break ;

		default : return SFE_UNIMPLEMENTED ;
		} ;

	if (wpriv->fmt_is_broken)
		wavlike_analyze (psf) ;

	/* Only set the format endian-ness if its non-standard big-endian. */
	if (psf->endian == SF_ENDIAN_BIG)
		psf->sf.format |= SF_ENDIAN_BIG ;

	return 0 ;
} /* rf64_read_header */