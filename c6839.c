wav_read_header	(SF_PRIVATE *psf, int *blockalign, int *framesperblock)
{	WAVLIKE_PRIVATE	*wpriv ;
	WAV_FMT		*wav_fmt ;
	FACT_CHUNK	fact_chunk ;
	uint32_t	marker, chunk_size = 0, RIFFsize = 0, done = 0 ;
	int			parsestage = 0, error, format = 0 ;

	if (psf->is_pipe == 0 && psf->filelength > SF_PLATFORM_S64 (0xffffffff))
		psf_log_printf (psf, "Warning : filelength > 0xffffffff. This is bad!!!!\n") ;

	if ((wpriv = psf->container_data) == NULL)
		return SFE_INTERNAL ;
	wav_fmt = &wpriv->wav_fmt ;

	/* Set position to start of file to begin reading header. */
	psf_binheader_readf (psf, "pmj", 0, &marker, -4) ;
	psf->header.indx = 0 ;

	/* RIFX signifies big-endian format for all header and data  to prevent
	** lots of code copying here, we'll set the psf->rwf_endian flag once here,
	** and never specify endian-ness for all other header ops/
	*/
	psf->rwf_endian = (marker == RIFF_MARKER) ? SF_ENDIAN_LITTLE : SF_ENDIAN_BIG ;

	while (! done)
	{	size_t jump = chunk_size & 1 ;

		marker = chunk_size = 0 ;
		psf_binheader_readf (psf, "jm4", jump, &marker, &chunk_size) ;
		if (marker == 0)
		{	sf_count_t pos = psf_ftell (psf) ;
			psf_log_printf (psf, "Have 0 marker at position %D (0x%x).\n", pos, pos) ;
			break ;
			} ;

		psf_store_read_chunk_u32 (&psf->rchunks, marker, psf_ftell (psf), chunk_size) ;

		switch (marker)
		{	case RIFF_MARKER :
			case RIFX_MARKER :
					if (parsestage)
						return SFE_WAV_NO_RIFF ;

					parsestage |= HAVE_RIFF ;

					RIFFsize = chunk_size ;

					if (psf->fileoffset > 0 && psf->filelength > RIFFsize + 8)
					{	/* Set file length. */
						psf->filelength = RIFFsize + 8 ;
						if (marker == RIFF_MARKER)
							psf_log_printf (psf, "RIFF : %u\n", RIFFsize) ;
						else
							psf_log_printf (psf, "RIFX : %u\n", RIFFsize) ;
						}
					else if (psf->filelength < RIFFsize + 2 * SIGNED_SIZEOF (marker))
					{	if (marker == RIFF_MARKER)
							psf_log_printf (psf, "RIFF : %u (should be %D)\n", RIFFsize, psf->filelength - 2 * SIGNED_SIZEOF (marker)) ;
						else
							psf_log_printf (psf, "RIFX : %u (should be %D)\n", RIFFsize, psf->filelength - 2 * SIGNED_SIZEOF (marker)) ;

						RIFFsize = psf->filelength - 2 * SIGNED_SIZEOF (RIFFsize) ;
						}
					else
					{	if (marker == RIFF_MARKER)
							psf_log_printf (psf, "RIFF : %u\n", RIFFsize) ;
						else
							psf_log_printf (psf, "RIFX : %u\n", RIFFsize) ;
					} ;

					psf_binheader_readf (psf, "m", &marker) ;
					if (marker != WAVE_MARKER)
						return SFE_WAV_NO_WAVE ;
					parsestage |= HAVE_WAVE ;
					psf_log_printf (psf, "WAVE\n") ;
					chunk_size = 0 ;
					break ;

			case fmt_MARKER :
					if ((parsestage & (HAVE_RIFF | HAVE_WAVE)) != (HAVE_RIFF | HAVE_WAVE))
						return SFE_WAV_NO_FMT ;

					/* If this file has a SECOND fmt chunk, I don't want to know about it. */
					if (parsestage & HAVE_fmt)
						break ;

					parsestage |= HAVE_fmt ;

					psf_log_printf (psf, "fmt  : %d\n", chunk_size) ;

					if ((error = wavlike_read_fmt_chunk (psf, chunk_size)))
						return error ;

					format = wav_fmt->format ;
					break ;

			case data_MARKER :
					if ((parsestage & (HAVE_RIFF | HAVE_WAVE | HAVE_fmt)) != (HAVE_RIFF | HAVE_WAVE | HAVE_fmt))
						return SFE_WAV_NO_DATA ;

					if (psf->file.mode == SFM_RDWR && (parsestage & HAVE_other) != 0)
						return SFE_RDWR_BAD_HEADER ;

					parsestage |= HAVE_data ;

					psf->datalength = chunk_size ;
					if (psf->datalength & 1)
						psf_log_printf (psf, "*** 'data' chunk should be an even number of bytes in length.\n") ;

					psf->dataoffset = psf_ftell (psf) ;

					if (psf->dataoffset > 0)
					{	if (chunk_size == 0 && RIFFsize == 8 && psf->filelength > 44)
						{	psf_log_printf (psf, "*** Looks like a WAV file which wasn't closed properly. Fixing it.\n") ;
							psf->datalength = psf->filelength - psf->dataoffset ;
							} ;

						if (psf->datalength > psf->filelength - psf->dataoffset)
						{	psf_log_printf (psf, "data : %D (should be %D)\n", psf->datalength, psf->filelength - psf->dataoffset) ;
							psf->datalength = psf->filelength - psf->dataoffset ;
							}
						else
							psf_log_printf (psf, "data : %D\n", psf->datalength) ;

						/* Only set dataend if there really is data at the end. */
						if (psf->datalength + psf->dataoffset < psf->filelength)
							psf->dataend = psf->datalength + psf->dataoffset ;

						psf->datalength += chunk_size & 1 ;
						chunk_size = 0 ;
						} ;

					if (! psf->sf.seekable || psf->dataoffset < 0)
						break ;

					/* Seek past data and continue reading header. */
					psf_fseek (psf, psf->datalength, SEEK_CUR) ;

					if (psf_ftell (psf) != psf->datalength + psf->dataoffset)
						psf_log_printf (psf, "*** psf_fseek past end error ***\n") ;
					break ;

			case fact_MARKER :
					if ((parsestage & (HAVE_RIFF | HAVE_WAVE)) != (HAVE_RIFF | HAVE_WAVE))
						return SFE_WAV_BAD_FACT ;

					parsestage |= HAVE_fact ;

					if ((parsestage & HAVE_fmt) != HAVE_fmt)
						psf_log_printf (psf, "*** Should have 'fmt ' chunk before 'fact'\n") ;

					psf_binheader_readf (psf, "4", & (fact_chunk.frames)) ;

					if (chunk_size > SIGNED_SIZEOF (fact_chunk))
						psf_binheader_readf (psf, "j", (int) (chunk_size - SIGNED_SIZEOF (fact_chunk))) ;

					if (chunk_size)
						psf_log_printf (psf, "%M : %u\n", marker, chunk_size) ;
					else
						psf_log_printf (psf, "%M : %u (should not be zero)\n", marker, chunk_size) ;

					psf_log_printf (psf, "  frames  : %d\n", fact_chunk.frames) ;
					break ;

			case PEAK_MARKER :
					if ((parsestage & (HAVE_RIFF | HAVE_WAVE | HAVE_fmt)) != (HAVE_RIFF | HAVE_WAVE | HAVE_fmt))
						return SFE_WAV_PEAK_B4_FMT ;

					parsestage |= HAVE_PEAK ;

					psf_log_printf (psf, "%M : %u\n", marker, chunk_size) ;
					if ((error = wavlike_read_peak_chunk (psf, chunk_size)) != 0)
						return error ;
					psf->peak_info->peak_loc = ((parsestage & HAVE_data) == 0) ? SF_PEAK_START : SF_PEAK_END ;
					break ;

			case cue_MARKER :
					parsestage |= HAVE_other ;

					{	uint32_t thisread, bytesread, cue_count, position, offset ;
						int id, chunk_id, chunk_start, block_start, cue_index ;

						bytesread = psf_binheader_readf (psf, "4", &cue_count) ;
						psf_log_printf (psf, "%M : %u\n", marker, chunk_size) ;

						if (cue_count > 1000)
						{	psf_log_printf (psf, "  Count : %u (skipping)\n", cue_count) ;
							psf_binheader_readf (psf, "j", (cue_count > 20 ? 20 : cue_count) * 24) ;
							break ;
							} ;

						psf_log_printf (psf, "  Count : %d\n", cue_count) ;

						if ((psf->cues = psf_cues_alloc (cue_count)) == NULL)
							return SFE_MALLOC_FAILED ;

						cue_index = 0 ;

						while (cue_count)
						{
							if ((thisread = psf_binheader_readf (psf, "e44m444", &id, &position, &chunk_id, &chunk_start, &block_start, &offset)) == 0)
								break ;
							bytesread += thisread ;

							psf_log_printf (psf,	"   Cue ID : %2d"
													"  Pos : %5u  Chunk : %M"
													"  Chk Start : %d  Blk Start : %d"
													"  Offset : %5d\n",
									id, position, chunk_id, chunk_start, block_start, offset) ;
							psf->cues->cue_points [cue_index].indx = id ;
							psf->cues->cue_points [cue_index].position = position ;
							psf->cues->cue_points [cue_index].fcc_chunk = chunk_id ;
							psf->cues->cue_points [cue_index].chunk_start = chunk_start ;
							psf->cues->cue_points [cue_index].block_start = block_start ;
							psf->cues->cue_points [cue_index].sample_offset = offset ;
							psf->cues->cue_points [cue_index].name [0] = '\0' ;
							cue_count -- ;
							cue_index ++ ;
							} ;

						if (bytesread != chunk_size)
						{	psf_log_printf (psf, "**** Chunk size weirdness (%d != %d)\n", chunk_size, bytesread) ;
							psf_binheader_readf (psf, "j", chunk_size - bytesread) ;
							} ;
						} ;
					break ;

			case smpl_MARKER :
					parsestage |= HAVE_other ;

					psf_log_printf (psf, "smpl : %u\n", chunk_size) ;

					if ((error = wav_read_smpl_chunk (psf, chunk_size)))
						return error ;
					break ;

			case acid_MARKER :
					parsestage |= HAVE_other ;

					psf_log_printf (psf, "acid : %u\n", chunk_size) ;

					if ((error = wav_read_acid_chunk (psf, chunk_size)))
						return error ;
					break ;

			case INFO_MARKER :
			case LIST_MARKER :
					parsestage |= HAVE_other ;

					if ((error = wavlike_subchunk_parse (psf, marker, chunk_size)) != 0)
						return error ;
					break ;

			case bext_MARKER :
					/*
					The 'bext' chunk can actually be updated, so don't need to set this.
					parsestage |= HAVE_other ;
					*/
					if ((error = wavlike_read_bext_chunk (psf, chunk_size)))
						return error ;
					break ;

			case PAD_MARKER :
					/*
					We can eat into a 'PAD ' chunk if we need to.
					parsestage |= HAVE_other ;
					*/
					psf_log_printf (psf, "%M : %u\n", marker, chunk_size) ;
					psf_binheader_readf (psf, "j", chunk_size) ;
					break ;

			case cart_MARKER:
					if ((error = wavlike_read_cart_chunk (psf, chunk_size)))
						return error ;
					break ;

			case iXML_MARKER : /* See http://en.wikipedia.org/wiki/IXML */
			case strc_MARKER : /* Multiple of 32 bytes. */
			case afsp_MARKER :
			case clm_MARKER :
			case elmo_MARKER :
			case levl_MARKER :
			case plst_MARKER :
			case minf_MARKER :
			case elm1_MARKER :
			case regn_MARKER :
			case ovwf_MARKER :
			case inst_MARKER :
			case AFAn_MARKER :
			case umid_MARKER :
			case SyLp_MARKER :
			case Cr8r_MARKER :
			case JUNK_MARKER :
			case PMX_MARKER :
			case DISP_MARKER :
			case MEXT_MARKER :
			case FLLR_MARKER :
					psf_log_printf (psf, "%M : %u\n", marker, chunk_size) ;
					psf_binheader_readf (psf, "j", chunk_size) ;
					break ;

			default :
					if (chunk_size >= 0xffff0000)
					{	done = SF_TRUE ;
						psf_log_printf (psf, "*** Unknown chunk marker (%X) at position %D with length %u. Exiting parser.\n", marker, psf_ftell (psf) - 8, chunk_size) ;
						break ;
						} ;

					if (psf_isprint ((marker >> 24) & 0xFF) && psf_isprint ((marker >> 16) & 0xFF)
						&& psf_isprint ((marker >> 8) & 0xFF) && psf_isprint (marker & 0xFF))
					{	psf_log_printf (psf, "*** %M : %u (unknown marker)\n", marker, chunk_size) ;
						psf_binheader_readf (psf, "j", chunk_size) ;
						break ;
						} ;
					if (psf_ftell (psf) & 0x03)
					{	psf_log_printf (psf, "  Unknown chunk marker at position %D. Resynching.\n", psf_ftell (psf) - 8) ;
						psf_binheader_readf (psf, "j", -3) ;
						/* File is too messed up so we prevent editing in RDWR mode here. */
						parsestage |= HAVE_other ;
						break ;
						} ;
					psf_log_printf (psf, "*** Unknown chunk marker (%X) at position %D. Exiting parser.\n", marker, psf_ftell (psf) - 8) ;
					done = SF_TRUE ;
					break ;
			} ;	/* switch (marker) */

		if (chunk_size >= psf->filelength)
		{	psf_log_printf (psf, "*** Chunk size %u > file length %D. Exiting parser.\n", chunk_size, psf->filelength) ;
			break ;
			} ;

		if (! psf->sf.seekable && (parsestage & HAVE_data))
			break ;

		if (psf_ftell (psf) >= psf->filelength - SIGNED_SIZEOF (chunk_size))
		{	psf_log_printf (psf, "End\n") ;
			break ;
			} ;
		} ; /* while (1) */

	if (psf->dataoffset <= 0)
		return SFE_WAV_NO_DATA ;

	if (psf->sf.channels < 1)
		return SFE_CHANNEL_COUNT_ZERO ;

	if (psf->sf.channels >= SF_MAX_CHANNELS)
		return SFE_CHANNEL_COUNT ;

	if (format != WAVE_FORMAT_PCM && (parsestage & HAVE_fact) == 0)
		psf_log_printf (psf, "**** All non-PCM format files should have a 'fact' chunk.\n") ;

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

	switch (format)
	{	case WAVE_FORMAT_EXTENSIBLE :
			if (psf->sf.format == (SF_FORMAT_WAVEX | SF_FORMAT_MS_ADPCM))
			{	*blockalign = wav_fmt->msadpcm.blockalign ;
				*framesperblock = wav_fmt->msadpcm.samplesperblock ;
				} ;
			break ;

		case WAVE_FORMAT_NMS_VBXADPCM :
			*blockalign = wav_fmt->min.blockalign ;
			*framesperblock = 160 ;
			switch (wav_fmt->min.bitwidth)
			{	case 2 :
					psf->sf.format = SF_FORMAT_WAV | SF_FORMAT_NMS_ADPCM_16 ;
					break ;
				case 3 :
					psf->sf.format = SF_FORMAT_WAV | SF_FORMAT_NMS_ADPCM_24 ;
					break ;
				case 4 :
					psf->sf.format = SF_FORMAT_WAV | SF_FORMAT_NMS_ADPCM_32 ;
					break ;

				default :
					return SFE_UNIMPLEMENTED ;
				}
				break ;

		case WAVE_FORMAT_PCM :
					psf->sf.format = SF_FORMAT_WAV | u_bitwidth_to_subformat (psf->bytewidth * 8) ;
					break ;

		case WAVE_FORMAT_MULAW :
		case IBM_FORMAT_MULAW :
					psf->sf.format = (SF_FORMAT_WAV | SF_FORMAT_ULAW) ;
					break ;

		case WAVE_FORMAT_ALAW :
		case IBM_FORMAT_ALAW :
					psf->sf.format = (SF_FORMAT_WAV | SF_FORMAT_ALAW) ;
					break ;

		case WAVE_FORMAT_MS_ADPCM :
					psf->sf.format = (SF_FORMAT_WAV | SF_FORMAT_MS_ADPCM) ;
					*blockalign = wav_fmt->msadpcm.blockalign ;
					*framesperblock = wav_fmt->msadpcm.samplesperblock ;
					break ;

		case WAVE_FORMAT_IMA_ADPCM :
					psf->sf.format = (SF_FORMAT_WAV | SF_FORMAT_IMA_ADPCM) ;
					*blockalign = wav_fmt->ima.blockalign ;
					*framesperblock = wav_fmt->ima.samplesperblock ;
					break ;

		case WAVE_FORMAT_GSM610 :
					psf->sf.format = (SF_FORMAT_WAV | SF_FORMAT_GSM610) ;
					break ;

		case WAVE_FORMAT_IEEE_FLOAT :
					psf->sf.format = SF_FORMAT_WAV ;
					psf->sf.format |= (psf->bytewidth == 8) ? SF_FORMAT_DOUBLE : SF_FORMAT_FLOAT ;
					break ;

		case WAVE_FORMAT_G721_ADPCM :
					psf->sf.format = SF_FORMAT_WAV | SF_FORMAT_G721_32 ;
					break ;

		default : return SFE_UNIMPLEMENTED ;
		} ;

	if (wpriv->fmt_is_broken)
		wavlike_analyze (psf) ;

	/* Only set the format endian-ness if its non-standard big-endian. */
	if (psf->endian == SF_ENDIAN_BIG)
		psf->sf.format |= SF_ENDIAN_BIG ;

	return 0 ;
} /* wav_read_header */