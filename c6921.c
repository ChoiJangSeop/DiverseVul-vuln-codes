wav_read_smpl_chunk (SF_PRIVATE *psf, uint32_t chunklen)
{	char buffer [512] ;
	uint32_t thisread, bytesread = 0, dword, sampler_data, loop_count ;
	uint32_t note, pitch, start, end, type = -1, count ;
	int j, k ;

	chunklen += (chunklen & 1) ;

	bytesread += psf_binheader_readf (psf, "4", &dword) ;
	psf_log_printf (psf, "  Manufacturer : %X\n", dword) ;

	bytesread += psf_binheader_readf (psf, "4", &dword) ;
	psf_log_printf (psf, "  Product      : %u\n", dword) ;

	bytesread += psf_binheader_readf (psf, "4", &dword) ;
	psf_log_printf (psf, "  Period       : %u nsec\n", dword) ;

	bytesread += psf_binheader_readf (psf, "4", &note) ;
	psf_log_printf (psf, "  Midi Note    : %u\n", note) ;

	bytesread += psf_binheader_readf (psf, "4", &pitch) ;
	if (pitch != 0)
	{	snprintf (buffer, sizeof (buffer), "%f",
					(1.0 * 0x80000000) / ((uint32_t) pitch)) ;
		psf_log_printf (psf, "  Pitch Fract. : %s\n", buffer) ;
		}
	else
		psf_log_printf (psf, "  Pitch Fract. : 0\n") ;

	bytesread += psf_binheader_readf (psf, "4", &dword) ;
	psf_log_printf (psf, "  SMPTE Format : %u\n", dword) ;

	bytesread += psf_binheader_readf (psf, "4", &dword) ;
	snprintf (buffer, sizeof (buffer), "%02d:%02d:%02d %02d",
				(dword >> 24) & 0x7F, (dword >> 16) & 0x7F, (dword >> 8) & 0x7F, dword & 0x7F) ;
	psf_log_printf (psf, "  SMPTE Offset : %s\n", buffer) ;

	bytesread += psf_binheader_readf (psf, "4", &loop_count) ;
	psf_log_printf (psf, "  Loop Count   : %u\n", loop_count) ;

	if (loop_count == 0 && chunklen == bytesread)
		return 0 ;

	/* Sampler Data holds the number of data bytes after the CUE chunks which
	** is not actually CUE data. Display value after CUE data.
	*/
	bytesread += psf_binheader_readf (psf, "4", &sampler_data) ;

	if ((psf->instrument = psf_instrument_alloc ()) == NULL)
		return SFE_MALLOC_FAILED ;

	psf->instrument->loop_count = loop_count ;

	for (j = 0 ; loop_count > 0 && chunklen - bytesread >= 24 ; j ++)
	{	if ((thisread = psf_binheader_readf (psf, "4", &dword)) == 0)
			break ;
		bytesread += thisread ;
		psf_log_printf (psf, "    Cue ID : %2u", dword) ;

		bytesread += psf_binheader_readf (psf, "4", &type) ;
		psf_log_printf (psf, "  Type : %2u", type) ;

		bytesread += psf_binheader_readf (psf, "4", &start) ;
		psf_log_printf (psf, "  Start : %5u", start) ;

		bytesread += psf_binheader_readf (psf, "4", &end) ;
		psf_log_printf (psf, "  End : %5u", end) ;

		bytesread += psf_binheader_readf (psf, "4", &dword) ;
		psf_log_printf (psf, "  Fraction : %5u", dword) ;

		bytesread += psf_binheader_readf (psf, "4", &count) ;
		psf_log_printf (psf, "  Count : %5u\n", count) ;

		if (j < ARRAY_LEN (psf->instrument->loops))
		{	psf->instrument->loops [j].start = start ;
			psf->instrument->loops [j].end = end + 1 ;
			psf->instrument->loops [j].count = count ;

			switch (type)
			{	case 0 :
					psf->instrument->loops [j].mode = SF_LOOP_FORWARD ;
					break ;
				case 1 :
					psf->instrument->loops [j].mode = SF_LOOP_ALTERNATING ;
					break ;
				case 2 :
					psf->instrument->loops [j].mode = SF_LOOP_BACKWARD ;
					break ;
				default:
					psf->instrument->loops [j].mode = SF_LOOP_NONE ;
					break ;
				} ;
			} ;

		loop_count -- ;
		} ;

	if (chunklen - bytesread == 0)
	{	if (sampler_data != 0)
			psf_log_printf (psf, "  Sampler Data : %u (should be 0)\n", sampler_data) ;
		else
			psf_log_printf (psf, "  Sampler Data : %u\n", sampler_data) ;
		}
	else
	{	if (sampler_data != chunklen - bytesread)
		{	psf_log_printf (psf, "  Sampler Data : %u (should have been %u)\n", sampler_data, chunklen - bytesread) ;
			sampler_data = chunklen - bytesread ;
			}
		else
			psf_log_printf (psf, "  Sampler Data : %u\n", sampler_data) ;

		psf_log_printf (psf, "      ") ;
		for (k = 0 ; k < (int) sampler_data ; k++)
		{	char ch ;

			if (k > 0 && (k % 20) == 0)
				psf_log_printf (psf, "\n      ") ;

			if ((thisread = psf_binheader_readf (psf, "1", &ch)) == 0)
				break ;
			bytesread += thisread ;
			psf_log_printf (psf, "%02X ", ch & 0xFF) ;
			} ;

		psf_log_printf (psf, "\n") ;
		} ;

	psf->instrument->basenote = note ;
	psf->instrument->detune = (int8_t)(pitch / (0x40000000 / 25.0) + 0.5) ;
	psf->instrument->gain = 1 ;
	psf->instrument->velocity_lo = psf->instrument->key_lo = 0 ;
	psf->instrument->velocity_hi = psf->instrument->key_hi = 127 ;

	return 0 ;
} /* wav_read_smpl_chunk */