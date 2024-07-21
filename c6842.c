w64_read_header	(SF_PRIVATE *psf, int *blockalign, int *framesperblock)
{	WAVLIKE_PRIVATE *wpriv ;
	WAV_FMT 	*wav_fmt ;
	int			dword = 0, marker, format = 0 ;
	sf_count_t	chunk_size, bytesread = 0 ;
	int			parsestage = 0, error, done = 0 ;

	if ((wpriv = psf->container_data) == NULL)
		return SFE_INTERNAL ;
	wav_fmt = &wpriv->wav_fmt ;

	/* Set position to start of file to begin reading header. */
	psf_binheader_readf (psf, "p", 0) ;

	while (! done)
	{	/* Each new chunk must start on an 8 byte boundary, so jump if needed. */
		if (psf->header.indx & 0x7)
			psf_binheader_readf (psf, "j", 8 - (psf->header.indx & 0x7)) ;

		/* Generate hash of 16 byte marker. */
		marker = chunk_size = 0 ;
		bytesread = psf_binheader_readf (psf, "eh8", &marker, &chunk_size) ;
		if (bytesread == 0)
			break ;
		switch (marker)
		{	case riff_HASH16 :
					if (parsestage)
						return SFE_W64_NO_RIFF ;

					if (psf->filelength != chunk_size)
						psf_log_printf (psf, "riff : %D (should be %D)\n", chunk_size, psf->filelength) ;
					else
						psf_log_printf (psf, "riff : %D\n", chunk_size) ;

					parsestage |= HAVE_riff ;

					bytesread += psf_binheader_readf (psf, "h", &marker) ;
					if (marker == wave_HASH16)
					{ 	if ((parsestage & HAVE_riff) != HAVE_riff)
							return SFE_W64_NO_WAVE ;
						psf_log_printf (psf, "wave\n") ;
						parsestage |= HAVE_wave ;
					} ;
					chunk_size = 0 ;
					break ;

			case ACID_HASH16:
					psf_log_printf (psf, "Looks like an ACID file. Exiting.\n") ;
					return SFE_UNIMPLEMENTED ;

			case fmt_HASH16 :
					if ((parsestage & (HAVE_riff | HAVE_wave)) != (HAVE_riff | HAVE_wave))
						return SFE_WAV_NO_FMT ;

					psf_log_printf (psf, " fmt : %D\n", chunk_size) ;

					/* size of 16 byte marker and 8 byte chunk_size value. */
					chunk_size -= 24 ;

					if ((error = wavlike_read_fmt_chunk (psf, (int) chunk_size)))
						return error ;

					if (chunk_size % 8)
						psf_binheader_readf (psf, "j", 8 - (chunk_size % 8)) ;

					format		= wav_fmt->format ;
					parsestage |= HAVE_fmt ;
					chunk_size = 0 ;
					break ;

			case fact_HASH16:
					{	sf_count_t frames ;

						psf_binheader_readf (psf, "e8", &frames) ;
						psf_log_printf (psf, "fact : %D\n  frames : %D\n",
										chunk_size, frames) ;
						} ;
					chunk_size = 0 ;
					break ;


			case data_HASH16 :
					if ((parsestage & (HAVE_riff | HAVE_wave | HAVE_fmt)) != (HAVE_riff | HAVE_wave | HAVE_fmt))
						return SFE_W64_NO_DATA ;

					psf->dataoffset = psf_ftell (psf) ;
					psf->datalength = SF_MIN (chunk_size - 24, psf->filelength - psf->dataoffset) ;

					if (chunk_size % 8)
						chunk_size += 8 - (chunk_size % 8) ;

					psf_log_printf (psf, "data : %D\n", chunk_size) ;

					parsestage |= HAVE_data ;

					if (! psf->sf.seekable)
						break ;

					/* Seek past data and continue reading header. */
					psf_fseek (psf, chunk_size, SEEK_CUR) ;
					chunk_size = 0 ;
					break ;

			case levl_HASH16 :
					psf_log_printf (psf, "levl : %D\n", chunk_size) ;
					chunk_size -= 24 ;
					break ;

			case list_HASH16 :
					psf_log_printf (psf, "list : %D\n", chunk_size) ;
					chunk_size -= 24 ;
					break ;

			case junk_HASH16 :
					psf_log_printf (psf, "junk : %D\n", chunk_size) ;
					chunk_size -= 24 ;
					break ;

			case bext_HASH16 :
					psf_log_printf (psf, "bext : %D\n", chunk_size) ;
					chunk_size -= 24 ;
					break ;

			case MARKER_HASH16 :
					psf_log_printf (psf, "marker : %D\n", chunk_size) ;
					chunk_size -= 24 ;
					break ;

			case SUMLIST_HASH16 :
					psf_log_printf (psf, "summary list : %D\n", chunk_size) ;
					chunk_size -= 24 ;
					break ;

			default :
					psf_log_printf (psf, "*** Unknown chunk marker (%X) at position %D with length %D. Exiting parser.\n", marker, psf_ftell (psf) - 8, chunk_size) ;
					done = SF_TRUE ;
					break ;
			} ;	/* switch (dword) */

		if (chunk_size >= psf->filelength)
		{	psf_log_printf (psf, "*** Chunk size %u > file length %D. Exiting parser.\n", chunk_size, psf->filelength) ;
			break ;
			} ;

		if (psf->sf.seekable == 0 && (parsestage & HAVE_data))
			break ;

		if (psf_ftell (psf) >= (psf->filelength - (2 * SIGNED_SIZEOF (dword))))
			break ;

		if (chunk_size > 0 && chunk_size < 0xffff0000)
		{	dword = chunk_size ;
			psf_binheader_readf (psf, "j", dword - 24) ;
			} ;
		} ; /* while (1) */

	if (psf->dataoffset <= 0)
		return SFE_W64_NO_DATA ;

	if (psf->sf.channels < 1)
		return SFE_CHANNEL_COUNT_ZERO ;

	if (psf->sf.channels >= SF_MAX_CHANNELS)
		return SFE_CHANNEL_COUNT ;

	psf->endian = SF_ENDIAN_LITTLE ;		/* All W64 files are little endian. */

	if (psf_ftell (psf) != psf->dataoffset)
		psf_fseek (psf, psf->dataoffset, SEEK_SET) ;

	if (psf->blockwidth)
	{	if (psf->filelength - psf->dataoffset < psf->datalength)
			psf->sf.frames = (psf->filelength - psf->dataoffset) / psf->blockwidth ;
		else
			psf->sf.frames = psf->datalength / psf->blockwidth ;
		} ;

	switch (format)
	{	case WAVE_FORMAT_PCM :
		case WAVE_FORMAT_EXTENSIBLE :
					/* extensible might be FLOAT, MULAW, etc as well! */
					psf->sf.format = SF_FORMAT_W64 | u_bitwidth_to_subformat (psf->bytewidth * 8) ;
					break ;

		case WAVE_FORMAT_MULAW :
					psf->sf.format = (SF_FORMAT_W64 | SF_FORMAT_ULAW) ;
					break ;

		case WAVE_FORMAT_ALAW :
					psf->sf.format = (SF_FORMAT_W64 | SF_FORMAT_ALAW) ;
					break ;

		case WAVE_FORMAT_MS_ADPCM :
					psf->sf.format = (SF_FORMAT_W64 | SF_FORMAT_MS_ADPCM) ;
					*blockalign = wav_fmt->msadpcm.blockalign ;
					*framesperblock = wav_fmt->msadpcm.samplesperblock ;
					break ;

		case WAVE_FORMAT_IMA_ADPCM :
					psf->sf.format = (SF_FORMAT_W64 | SF_FORMAT_IMA_ADPCM) ;
					*blockalign = wav_fmt->ima.blockalign ;
					*framesperblock = wav_fmt->ima.samplesperblock ;
					break ;

		case WAVE_FORMAT_GSM610 :
					psf->sf.format = (SF_FORMAT_W64 | SF_FORMAT_GSM610) ;
					break ;

		case WAVE_FORMAT_IEEE_FLOAT :
					psf->sf.format = SF_FORMAT_W64 ;
					psf->sf.format |= (psf->bytewidth == 8) ? SF_FORMAT_DOUBLE : SF_FORMAT_FLOAT ;
					break ;

		default : return SFE_UNIMPLEMENTED ;
		} ;

	return 0 ;
} /* w64_read_header */