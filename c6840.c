aiff_read_comm_chunk (SF_PRIVATE *psf, COMM_CHUNK *comm_fmt)
{	BUF_UNION	ubuf ;
	int subformat, samplerate ;

	ubuf.scbuf [0] = 0 ;

	/* The COMM chunk has an int aligned to an odd word boundary. Some
	** procesors are not able to deal with this (ie bus fault) so we have
	** to take special care.
	*/

	psf_binheader_readf (psf, "E242b", &(comm_fmt->numChannels), &(comm_fmt->numSampleFrames),
				&(comm_fmt->sampleSize), &(comm_fmt->sampleRate), SIGNED_SIZEOF (comm_fmt->sampleRate)) ;

	if (comm_fmt->size > 0x10000 && (comm_fmt->size & 0xffff) == 0)
	{	psf_log_printf (psf, " COMM : %d (0x%x) *** should be ", comm_fmt->size, comm_fmt->size) ;
		comm_fmt->size = ENDSWAP_32 (comm_fmt->size) ;
		psf_log_printf (psf, "%d (0x%x)\n", comm_fmt->size, comm_fmt->size) ;
		}
	else
		psf_log_printf (psf, " COMM : %d\n", comm_fmt->size) ;

	if (comm_fmt->size == SIZEOF_AIFF_COMM)
		comm_fmt->encoding = NONE_MARKER ;
	else if (comm_fmt->size == SIZEOF_AIFC_COMM_MIN)
		psf_binheader_readf (psf, "Em", &(comm_fmt->encoding)) ;
	else if (comm_fmt->size >= SIZEOF_AIFC_COMM)
	{	uint8_t encoding_len ;
		unsigned read_len ;

		psf_binheader_readf (psf, "Em1", &(comm_fmt->encoding), &encoding_len) ;

		comm_fmt->size = SF_MIN (sizeof (ubuf.scbuf), make_size_t (comm_fmt->size)) ;
		memset (ubuf.scbuf, 0, comm_fmt->size) ;
		read_len = comm_fmt->size - SIZEOF_AIFC_COMM + 1 ;
		psf_binheader_readf (psf, "b", ubuf.scbuf, read_len) ;
		ubuf.scbuf [read_len + 1] = 0 ;
		} ;

	samplerate = tenbytefloat2int (comm_fmt->sampleRate) ;

	psf_log_printf (psf, "  Sample Rate : %d\n", samplerate) ;
	psf_log_printf (psf, "  Frames      : %u%s\n", comm_fmt->numSampleFrames, (comm_fmt->numSampleFrames == 0 && psf->filelength > 104) ? " (Should not be 0)" : "") ;

	if (comm_fmt->numChannels < 1 || comm_fmt->numChannels >= SF_MAX_CHANNELS)
	{	psf_log_printf (psf, "  Channels    : %d (should be >= 1 and < %d)\n", comm_fmt->numChannels, SF_MAX_CHANNELS) ;
		return SFE_CHANNEL_COUNT_BAD ;
		} ;

	psf_log_printf (psf, "  Channels    : %d\n", comm_fmt->numChannels) ;

	/* Found some broken 'fl32' files with comm.samplesize == 16. Fix it here. */
	if ((comm_fmt->encoding == fl32_MARKER || comm_fmt->encoding == FL32_MARKER) && comm_fmt->sampleSize != 32)
	{	psf_log_printf (psf, "  Sample Size : %d (should be 32)\n", comm_fmt->sampleSize) ;
		comm_fmt->sampleSize = 32 ;
		}
	else if ((comm_fmt->encoding == fl64_MARKER || comm_fmt->encoding == FL64_MARKER) && comm_fmt->sampleSize != 64)
	{	psf_log_printf (psf, "  Sample Size : %d (should be 64)\n", comm_fmt->sampleSize) ;
		comm_fmt->sampleSize = 64 ;
		}
	else
		psf_log_printf (psf, "  Sample Size : %d\n", comm_fmt->sampleSize) ;

	subformat = s_bitwidth_to_subformat (comm_fmt->sampleSize) ;

	psf->sf.samplerate = samplerate ;
	psf->sf.frames = comm_fmt->numSampleFrames ;
	psf->sf.channels = comm_fmt->numChannels ;
	psf->bytewidth = BITWIDTH2BYTES (comm_fmt->sampleSize) ;

	psf->endian = SF_ENDIAN_BIG ;

	switch (comm_fmt->encoding)
	{	case NONE_MARKER :
				psf->sf.format = (SF_FORMAT_AIFF | subformat) ;
				break ;

		case twos_MARKER :
		case in24_MARKER :
		case in32_MARKER :
				psf->sf.format = (SF_ENDIAN_BIG | SF_FORMAT_AIFF | subformat) ;
				break ;

		case sowt_MARKER :
		case ni24_MARKER :
		case ni32_MARKER :
				psf->endian = SF_ENDIAN_LITTLE ;
				psf->sf.format = (SF_ENDIAN_LITTLE | SF_FORMAT_AIFF | subformat) ;
				break ;

		case fl32_MARKER :
		case FL32_MARKER :
				psf->sf.format = (SF_FORMAT_AIFF | SF_FORMAT_FLOAT) ;
				break ;

		case ulaw_MARKER :
		case ULAW_MARKER :
				psf->sf.format = (SF_FORMAT_AIFF | SF_FORMAT_ULAW) ;
				break ;

		case alaw_MARKER :
		case ALAW_MARKER :
				psf->sf.format = (SF_FORMAT_AIFF | SF_FORMAT_ALAW) ;
				break ;

		case fl64_MARKER :
		case FL64_MARKER :
				psf->sf.format = (SF_FORMAT_AIFF | SF_FORMAT_DOUBLE) ;
				break ;

		case raw_MARKER :
				psf->sf.format = (SF_FORMAT_AIFF | SF_FORMAT_PCM_U8) ;
				break ;

		case DWVW_MARKER :
				psf->sf.format = SF_FORMAT_AIFF ;
				switch (comm_fmt->sampleSize)
				{	case 12 :
						psf->sf.format |= SF_FORMAT_DWVW_12 ;
						break ;
					case 16 :
						psf->sf.format |= SF_FORMAT_DWVW_16 ;
						break ;
					case 24 :
						psf->sf.format |= SF_FORMAT_DWVW_24 ;
						break ;

					default :
						psf->sf.format |= SF_FORMAT_DWVW_N ;
						break ;
					} ;
				break ;

		case GSM_MARKER :
				psf->sf.format = SF_FORMAT_AIFF ;
				psf->sf.format = (SF_FORMAT_AIFF | SF_FORMAT_GSM610) ;
				break ;


		case ima4_MARKER :
				psf->endian = SF_ENDIAN_BIG ;
				psf->sf.format = (SF_FORMAT_AIFF | SF_FORMAT_IMA_ADPCM) ;
				break ;

		default :
			psf_log_printf (psf, "AIFC : Unimplemented format : %M\n", comm_fmt->encoding) ;
			return SFE_UNIMPLEMENTED ;
		} ;

	if (! ubuf.scbuf [0])
		psf_log_printf (psf, "  Encoding    : %M\n", comm_fmt->encoding) ;
	else
		psf_log_printf (psf, "  Encoding    : %M => %s\n", comm_fmt->encoding, ubuf.scbuf) ;

	return 0 ;
} /* aiff_read_comm_chunk */