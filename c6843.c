aiff_read_header (SF_PRIVATE *psf, COMM_CHUNK *comm_fmt)
{	SSND_CHUNK	ssnd_fmt ;
	AIFF_PRIVATE *paiff ;
	BUF_UNION	ubuf ;
	uint32_t	chunk_size = 0, FORMsize, SSNDsize, bytesread, mark_count = 0 ;
	int			k, found_chunk = 0, done = 0, error = 0 ;
	char		*cptr ;
	int			instr_found = 0, mark_found = 0 ;

	if (psf->filelength > SF_PLATFORM_S64 (0xffffffff))
		psf_log_printf (psf, "Warning : filelength > 0xffffffff. This is bad!!!!\n") ;

	if ((paiff = psf->container_data) == NULL)
		return SFE_INTERNAL ;

	paiff->comm_offset = 0 ;
	paiff->ssnd_offset = 0 ;

	/* Set position to start of file to begin reading header. */
	psf_binheader_readf (psf, "p", 0) ;

	memset (comm_fmt, 0, sizeof (COMM_CHUNK)) ;

	/* Until recently AIF* file were all BIG endian. */
	psf->endian = SF_ENDIAN_BIG ;

	/*	AIFF files can apparently have their chunks in any order. However, they
	**	must have a FORM chunk. Approach here is to read all the chunks one by
	**	one and then check for the mandatory chunks at the end.
	*/
	while (! done)
	{	unsigned	marker ;
		size_t jump = chunk_size & 1 ;

		marker = chunk_size = 0 ;
		psf_binheader_readf (psf, "Ejm4", jump, &marker, &chunk_size) ;
		if (marker == 0)
		{	sf_count_t pos = psf_ftell (psf) ;
			psf_log_printf (psf, "Have 0 marker at position %D (0x%x).\n", pos, pos) ;
			break ;
			} ;

		if (psf->file.mode == SFM_RDWR && (found_chunk & HAVE_SSND))
			return SFE_AIFF_RW_SSND_NOT_LAST ;

		psf_store_read_chunk_u32 (&psf->rchunks, marker, psf_ftell (psf), chunk_size) ;

		switch (marker)
		{	case FORM_MARKER :
					if (found_chunk)
						return SFE_AIFF_NO_FORM ;

					FORMsize = chunk_size ;

					found_chunk |= HAVE_FORM ;
					psf_binheader_readf (psf, "m", &marker) ;
					switch (marker)
					{	case AIFC_MARKER :
						case AIFF_MARKER :
							found_chunk |= (marker == AIFC_MARKER) ? (HAVE_AIFC | HAVE_AIFF) : HAVE_AIFF ;
							break ;
						default :
							break ;
						} ;

					if (psf->fileoffset > 0 && psf->filelength > FORMsize + 8)
					{	/* Set file length. */
						psf->filelength = FORMsize + 8 ;
						psf_log_printf (psf, "FORM : %u\n %M\n", FORMsize, marker) ;
						}
					else if (FORMsize != psf->filelength - 2 * SIGNED_SIZEOF (chunk_size))
					{	chunk_size = psf->filelength - 2 * sizeof (chunk_size) ;
						psf_log_printf (psf, "FORM : %u (should be %u)\n %M\n", FORMsize, chunk_size, marker) ;
						FORMsize = chunk_size ;
						}
					else
						psf_log_printf (psf, "FORM : %u\n %M\n", FORMsize, marker) ;
					/* Set this to 0, so we don't jump a byte when parsing the next marker. */
					chunk_size = 0 ;
					break ;


			case COMM_MARKER :
					paiff->comm_offset = psf_ftell (psf) - 8 ;
					chunk_size += chunk_size & 1 ;
					comm_fmt->size = chunk_size ;
					if ((error = aiff_read_comm_chunk (psf, comm_fmt)) != 0)
						return error ;

					found_chunk |= HAVE_COMM ;
					break ;

			case PEAK_MARKER :
					/* Must have COMM chunk before PEAK chunk. */
					if ((found_chunk & (HAVE_FORM | HAVE_AIFF | HAVE_COMM)) != (HAVE_FORM | HAVE_AIFF | HAVE_COMM))
						return SFE_AIFF_PEAK_B4_COMM ;

					psf_log_printf (psf, "%M : %d\n", marker, chunk_size) ;
					if (chunk_size != AIFF_PEAK_CHUNK_SIZE (psf->sf.channels))
					{	psf_binheader_readf (psf, "j", chunk_size) ;
						psf_log_printf (psf, "*** File PEAK chunk too big.\n") ;
						return SFE_WAV_BAD_PEAK ;
						} ;

					if ((psf->peak_info = peak_info_calloc (psf->sf.channels)) == NULL)
						return SFE_MALLOC_FAILED ;

					/* read in rest of PEAK chunk. */
					psf_binheader_readf (psf, "E44", &(psf->peak_info->version), &(psf->peak_info->timestamp)) ;

					if (psf->peak_info->version != 1)
						psf_log_printf (psf, "  version    : %d *** (should be version 1)\n", psf->peak_info->version) ;
					else
						psf_log_printf (psf, "  version    : %d\n", psf->peak_info->version) ;

					psf_log_printf (psf, "  time stamp : %d\n", psf->peak_info->timestamp) ;
					psf_log_printf (psf, "    Ch   Position       Value\n") ;

					cptr = ubuf.cbuf ;
					for (k = 0 ; k < psf->sf.channels ; k++)
					{	float value ;
						uint32_t position ;

						psf_binheader_readf (psf, "Ef4", &value, &position) ;
						psf->peak_info->peaks [k].value = value ;
						psf->peak_info->peaks [k].position = position ;

						snprintf (cptr, sizeof (ubuf.scbuf), "    %2d   %-12" PRId64 "   %g\n",
								k, psf->peak_info->peaks [k].position, psf->peak_info->peaks [k].value) ;
						cptr [sizeof (ubuf.scbuf) - 1] = 0 ;
						psf_log_printf (psf, "%s", cptr) ;
						} ;

					psf->peak_info->peak_loc = ((found_chunk & HAVE_SSND) == 0) ? SF_PEAK_START : SF_PEAK_END ;
					break ;

			case SSND_MARKER :
					if ((found_chunk & HAVE_AIFC) && (found_chunk & HAVE_FVER) == 0)
						psf_log_printf (psf, "*** Valid AIFC files should have an FVER chunk.\n") ;

					paiff->ssnd_offset = psf_ftell (psf) - 8 ;
					SSNDsize = chunk_size ;
					psf_binheader_readf (psf, "E44", &(ssnd_fmt.offset), &(ssnd_fmt.blocksize)) ;

					psf->datalength = SSNDsize - sizeof (ssnd_fmt) ;
					psf->dataoffset = psf_ftell (psf) ;

					if (psf->datalength > psf->filelength - psf->dataoffset || psf->datalength < 0)
					{	psf_log_printf (psf, " SSND : %u (should be %D)\n", SSNDsize, psf->filelength - psf->dataoffset + sizeof (SSND_CHUNK)) ;
						psf->datalength = psf->filelength - psf->dataoffset ;
						}
					else
						psf_log_printf (psf, " SSND : %u\n", SSNDsize) ;

					if (ssnd_fmt.offset == 0 || psf->dataoffset + ssnd_fmt.offset == ssnd_fmt.blocksize)
					{	psf_log_printf (psf, "  Offset     : %u\n", ssnd_fmt.offset) ;
						psf_log_printf (psf, "  Block Size : %u\n", ssnd_fmt.blocksize) ;

						psf->dataoffset += ssnd_fmt.offset ;
						psf->datalength -= ssnd_fmt.offset ;
						}
					else
					{	psf_log_printf (psf, "  Offset     : %u\n", ssnd_fmt.offset) ;
						psf_log_printf (psf, "  Block Size : %u ???\n", ssnd_fmt.blocksize) ;
						psf->dataoffset += ssnd_fmt.offset ;
						psf->datalength -= ssnd_fmt.offset ;
						} ;

					/* Only set dataend if there really is data at the end. */
					if (psf->datalength + psf->dataoffset < psf->filelength)
						psf->dataend = psf->datalength + psf->dataoffset ;

					found_chunk |= HAVE_SSND ;

					if (! psf->sf.seekable)
						break ;

					/* Seek to end of SSND chunk. */
					psf_fseek (psf, psf->dataoffset + psf->datalength, SEEK_SET) ;
					break ;

			case c_MARKER :
					if (chunk_size == 0)
						break ;
					if (chunk_size >= SIGNED_SIZEOF (ubuf.scbuf))
					{	psf_log_printf (psf, " %M : %d (too big)\n", marker, chunk_size) ;
						return SFE_INTERNAL ;
						} ;

					cptr = ubuf.cbuf ;
					psf_binheader_readf (psf, "b", cptr, chunk_size + (chunk_size & 1)) ;
					cptr [chunk_size] = 0 ;

					psf_sanitize_string (cptr, chunk_size) ;

					psf_log_printf (psf, " %M : %s\n", marker, cptr) ;
					psf_store_string (psf, SF_STR_COPYRIGHT, cptr) ;
					chunk_size += chunk_size & 1 ;
					break ;

			case AUTH_MARKER :
					if (chunk_size == 0)
						break ;
					if (chunk_size >= SIGNED_SIZEOF (ubuf.scbuf) - 1)
					{	psf_log_printf (psf, " %M : %d (too big)\n", marker, chunk_size) ;
						return SFE_INTERNAL ;
						} ;

					cptr = ubuf.cbuf ;
					psf_binheader_readf (psf, "b", cptr, chunk_size + (chunk_size & 1)) ;
					cptr [chunk_size] = 0 ;
					psf_log_printf (psf, " %M : %s\n", marker, cptr) ;
					psf_store_string (psf, SF_STR_ARTIST, cptr) ;
					chunk_size += chunk_size & 1 ;
					break ;

			case COMT_MARKER :
				{	uint16_t count, id, len ;
					uint32_t timestamp, bytes ;

					if (chunk_size == 0)
						break ;
					bytes = chunk_size ;
					bytes -= psf_binheader_readf (psf, "E2", &count) ;
					psf_log_printf (psf, " %M : %d\n  count  : %d\n", marker, chunk_size, count) ;

					for (k = 0 ; k < count ; k++)
					{	bytes -= psf_binheader_readf (psf, "E422", &timestamp, &id, &len) ;
						psf_log_printf (psf, "   time   : 0x%x\n   marker : %x\n   length : %d\n", timestamp, id, len) ;

						if (len + 1 > SIGNED_SIZEOF (ubuf.scbuf))
						{	psf_log_printf (psf, "\nError : string length (%d) too big.\n", len) ;
							return SFE_INTERNAL ;
							} ;

						cptr = ubuf.cbuf ;
						bytes -= psf_binheader_readf (psf, "b", cptr, len) ;
						cptr [len] = 0 ;
						psf_log_printf (psf, "   string : %s\n", cptr) ;
						} ;

					if (bytes > 0)
						psf_binheader_readf (psf, "j", bytes) ;
					} ;
					break ;

			case APPL_MARKER :
				{	unsigned appl_marker ;

					if (chunk_size == 0)
						break ;
					if (chunk_size >= SIGNED_SIZEOF (ubuf.scbuf) - 1)
					{	psf_log_printf (psf, " %M : %u (too big, skipping)\n", marker, chunk_size) ;
						psf_binheader_readf (psf, "j", chunk_size + (chunk_size & 1)) ;
						break ;
						} ;

					if (chunk_size < 4)
					{	psf_log_printf (psf, " %M : %d (too small, skipping)\n", marker, chunk_size) ;
						psf_binheader_readf (psf, "j", chunk_size + (chunk_size & 1)) ;
						break ;
						} ;

					cptr = ubuf.cbuf ;
					psf_binheader_readf (psf, "mb", &appl_marker, cptr, chunk_size + (chunk_size & 1) - 4) ;
					cptr [chunk_size] = 0 ;

					for (k = 0 ; k < (int) chunk_size ; k++)
						if (! psf_isprint (cptr [k]))
						{	cptr [k] = 0 ;
							break ;
							} ;

					psf_log_printf (psf, " %M : %d\n  AppSig : %M\n  Name   : %s\n", marker, chunk_size, appl_marker, cptr) ;
					psf_store_string (psf, SF_STR_SOFTWARE, cptr) ;
					chunk_size += chunk_size & 1 ;
					} ;
					break ;

			case NAME_MARKER :
					if (chunk_size == 0)
						break ;
					if (chunk_size >= SIGNED_SIZEOF (ubuf.scbuf) - 2)
					{	psf_log_printf (psf, " %M : %d (too big)\n", marker, chunk_size) ;
						return SFE_INTERNAL ;
						} ;

					cptr = ubuf.cbuf ;
					psf_binheader_readf (psf, "b", cptr, chunk_size + (chunk_size & 1)) ;
					cptr [chunk_size] = 0 ;
					psf_log_printf (psf, " %M : %s\n", marker, cptr) ;
					psf_store_string (psf, SF_STR_TITLE, cptr) ;
					chunk_size += chunk_size & 1 ;
					break ;

			case ANNO_MARKER :
					if (chunk_size == 0)
						break ;
					if (chunk_size >= SIGNED_SIZEOF (ubuf.scbuf) - 2)
					{	psf_log_printf (psf, " %M : %d (too big)\n", marker, chunk_size) ;
						return SFE_INTERNAL ;
						} ;

					cptr = ubuf.cbuf ;
					psf_binheader_readf (psf, "b", cptr, chunk_size + (chunk_size & 1)) ;
					cptr [chunk_size] = 0 ;
					psf_log_printf (psf, " %M : %s\n", marker, cptr) ;
					psf_store_string (psf, SF_STR_COMMENT, cptr) ;
					chunk_size += chunk_size & 1 ;
					break ;

			case INST_MARKER :
					if (chunk_size != SIZEOF_INST_CHUNK)
					{	psf_log_printf (psf, " %M : %d (should be %d)\n", marker, chunk_size, SIZEOF_INST_CHUNK) ;
						psf_binheader_readf (psf, "j", chunk_size) ;
						break ;
						} ;
					psf_log_printf (psf, " %M : %d\n", marker, chunk_size) ;
					{	uint8_t bytes [6] ;
						int16_t gain ;

						if (psf->instrument == NULL && (psf->instrument = psf_instrument_alloc ()) == NULL)
							return SFE_MALLOC_FAILED ;

						psf_binheader_readf (psf, "b", bytes, 6) ;
						psf_log_printf (psf, "  Base Note : %u\n  Detune    : %u\n"
											"  Low  Note : %u\n  High Note : %u\n"
											"  Low  Vel. : %u\n  High Vel. : %u\n",
											bytes [0], bytes [1], bytes [2], bytes [3], bytes [4], bytes [5]) ;
						psf->instrument->basenote = bytes [0] ;
						psf->instrument->detune = bytes [1] ;
						psf->instrument->key_lo = bytes [2] ;
						psf->instrument->key_hi = bytes [3] ;
						psf->instrument->velocity_lo = bytes [4] ;
						psf->instrument->velocity_hi = bytes [5] ;
						psf_binheader_readf (psf, "E2", &gain) ;
						psf->instrument->gain = gain ;
						psf_log_printf (psf, "  Gain (dB) : %d\n", gain) ;
						} ;
					{	int16_t	mode ; /* 0 - no loop, 1 - forward looping, 2 - backward looping */
						const char	*loop_mode ;
						uint16_t begin, end ;

						psf_binheader_readf (psf, "E222", &mode, &begin, &end) ;
						loop_mode = get_loop_mode_str (mode) ;
						mode = get_loop_mode (mode) ;
						if (mode == SF_LOOP_NONE)
						{	psf->instrument->loop_count = 0 ;
							psf->instrument->loops [0].mode = SF_LOOP_NONE ;
							}
						else
						{	psf->instrument->loop_count = 1 ;
							psf->instrument->loops [0].mode = SF_LOOP_FORWARD ;
							psf->instrument->loops [0].start = begin ;
							psf->instrument->loops [0].end = end ;
							psf->instrument->loops [0].count = 0 ;
							} ;
						psf_log_printf (psf, "  Sustain\n   mode  : %d => %s\n   begin : %u\n   end   : %u\n",
											mode, loop_mode, begin, end) ;
						psf_binheader_readf (psf, "E222", &mode, &begin, &end) ;
						loop_mode = get_loop_mode_str (mode) ;
						mode = get_loop_mode (mode) ;
						if (mode == SF_LOOP_NONE)
							psf->instrument->loops [1].mode = SF_LOOP_NONE ;
						else
						{	psf->instrument->loop_count += 1 ;
							psf->instrument->loops [1].mode = SF_LOOP_FORWARD ;
							psf->instrument->loops [1].start = begin ;
							psf->instrument->loops [1].end = end ;
							psf->instrument->loops [1].count = 0 ;
							} ;
						psf_log_printf (psf, "  Release\n   mode  : %d => %s\n   begin : %u\n   end   : %u\n",
										mode, loop_mode, begin, end) ;
						} ;
					instr_found++ ;
					break ;

			case basc_MARKER :
					psf_log_printf (psf, " basc : %u\n", chunk_size) ;

					if ((error = aiff_read_basc_chunk (psf, chunk_size)))
						return error ;
					break ;

			case MARK_MARKER :
					psf_log_printf (psf, " %M : %d\n", marker, chunk_size) ;
					{	uint16_t mark_id, n = 0 ;
						uint32_t position ;

						bytesread = psf_binheader_readf (psf, "E2", &n) ;
						mark_count = n ;
						psf_log_printf (psf, "  Count : %u\n", mark_count) ;
						if (paiff->markstr != NULL)
						{	psf_log_printf (psf, "*** Second MARK chunk found. Throwing away the first.\n") ;
							free (paiff->markstr) ;
							} ;
						paiff->markstr = calloc (mark_count, sizeof (MARK_ID_POS)) ;
						if (paiff->markstr == NULL)
							return SFE_MALLOC_FAILED ;

						if (mark_count > 1000)
						{	psf_log_printf (psf, "  More than 1000 markers, skipping!\n") ;
							psf_binheader_readf (psf, "j", chunk_size - bytesread) ;
							break ;
						} ;

						if ((psf->cues = psf_cues_alloc (mark_count)) == NULL)
							return SFE_MALLOC_FAILED ;

						for (n = 0 ; n < mark_count && bytesread < chunk_size ; n++)
						{	uint32_t pstr_len ;
							uint8_t ch ;

							bytesread += psf_binheader_readf (psf, "E241", &mark_id, &position, &ch) ;
							psf_log_printf (psf, "   Mark ID  : %u\n   Position : %u\n", mark_id, position) ;

							psf->cues->cue_points [n].indx = mark_id ;
							psf->cues->cue_points [n].position = 0 ;
							psf->cues->cue_points [n].fcc_chunk = MAKE_MARKER ('d', 'a', 't', 'a') ; /* always data */
							psf->cues->cue_points [n].chunk_start = 0 ;
							psf->cues->cue_points [n].block_start = 0 ;
							psf->cues->cue_points [n].sample_offset = position ;

							pstr_len = (ch & 1) ? ch : ch + 1 ;

							if (pstr_len < sizeof (ubuf.scbuf) - 1)
							{	bytesread += psf_binheader_readf (psf, "b", ubuf.scbuf, pstr_len) ;
								ubuf.scbuf [pstr_len] = 0 ;
								}
							else
							{	uint32_t read_len = pstr_len - (sizeof (ubuf.scbuf) - 1) ;
								bytesread += psf_binheader_readf (psf, "bj", ubuf.scbuf, read_len, pstr_len - read_len) ;
								ubuf.scbuf [sizeof (ubuf.scbuf) - 1] = 0 ;
								}

							psf_log_printf (psf, "   Name     : %s\n", ubuf.scbuf) ;

							psf_strlcpy (psf->cues->cue_points [n].name, sizeof (psf->cues->cue_points [n].name), ubuf.cbuf) ;

							paiff->markstr [n].markerID = mark_id ;
							paiff->markstr [n].position = position ;
							/*
							**	TODO if ubuf.scbuf is equal to
							**	either Beg_loop, Beg loop or beg loop and spam
							**	if (psf->instrument == NULL && (psf->instrument = psf_instrument_alloc ()) == NULL)
							**		return SFE_MALLOC_FAILED ;
							*/
							} ;
						} ;
					mark_found++ ;
					psf_binheader_readf (psf, "j", chunk_size - bytesread) ;
					break ;

			case FVER_MARKER :
					found_chunk |= HAVE_FVER ;
					/* Falls through. */

			case SFX_MARKER :
					psf_log_printf (psf, " %M : %d\n", marker, chunk_size) ;
					psf_binheader_readf (psf, "j", chunk_size) ;
					break ;

			case NONE_MARKER :
					/* Fix for broken AIFC files with incorrect COMM chunk length. */
					chunk_size = (chunk_size >> 24) - 3 ;
					psf_log_printf (psf, " %M : %d\n", marker, chunk_size) ;
					psf_binheader_readf (psf, "j", make_size_t (chunk_size)) ;
					break ;

			case CHAN_MARKER :
					if (chunk_size < 12)
					{	psf_log_printf (psf, " %M : %d (should be >= 12)\n", marker, chunk_size) ;
						psf_binheader_readf (psf, "j", chunk_size) ;
						break ;
						}

					psf_log_printf (psf, " %M : %d\n", marker, chunk_size) ;

					if ((error = aiff_read_chanmap (psf, chunk_size)))
						return error ;
					break ;

			default :
					if (chunk_size >= 0xffff0000)
					{	done = SF_TRUE ;
						psf_log_printf (psf, "*** Unknown chunk marker (%X) at position %D with length %u. Exiting parser.\n", marker, psf_ftell (psf) - 8, chunk_size) ;
						break ;
						} ;

					if (psf_isprint ((marker >> 24) & 0xFF) && psf_isprint ((marker >> 16) & 0xFF)
						&& psf_isprint ((marker >> 8) & 0xFF) && psf_isprint (marker & 0xFF))
					{	psf_log_printf (psf, " %M : %u (unknown marker)\n", marker, chunk_size) ;

						psf_binheader_readf (psf, "j", chunk_size) ;
						break ;
						} ;

					if (psf_ftell (psf) & 0x03)
					{	psf_log_printf (psf, "  Unknown chunk marker at position %D. Resynching.\n", psf_ftell (psf) - 8) ;
						psf_binheader_readf (psf, "j", -3) ;
						break ;
						} ;
					psf_log_printf (psf, "*** Unknown chunk marker %X at position %D. Exiting parser.\n", marker, psf_ftell (psf)) ;
					done = SF_TRUE ;
					break ;
			} ;	/* switch (marker) */

		if (chunk_size >= psf->filelength)
		{	psf_log_printf (psf, "*** Chunk size %u > file length %D. Exiting parser.\n", chunk_size, psf->filelength) ;
			break ;
			} ;

		if ((! psf->sf.seekable) && (found_chunk & HAVE_SSND))
			break ;

		if (psf_ftell (psf) >= psf->filelength - (2 * SIGNED_SIZEOF (int32_t)))
			break ;
		} ; /* while (1) */

	if (instr_found && mark_found)
	{	int ji, str_index ;
		/* Next loop will convert markers to loop positions for internal handling */
		for (ji = 0 ; ji < psf->instrument->loop_count ; ji ++)
		{	if (ji < ARRAY_LEN (psf->instrument->loops))
			{	psf->instrument->loops [ji].start = marker_to_position (paiff->markstr, psf->instrument->loops [ji].start, mark_count) ;
				psf->instrument->loops [ji].end = marker_to_position (paiff->markstr, psf->instrument->loops [ji].end, mark_count) ;
				psf->instrument->loops [ji].mode = SF_LOOP_FORWARD ;
				} ;
			} ;

		/* The markers that correspond to loop positions can now be removed from cues struct */
		if (psf->cues->cue_count > (uint32_t) (psf->instrument->loop_count * 2))
		{	uint32_t j ;

			for (j = 0 ; j < psf->cues->cue_count - (uint32_t) (psf->instrument->loop_count * 2) ; j ++)
			{	/* This simply copies the information in cues above loop positions and writes it at current count instead */
				psf->cues->cue_points [j].indx = psf->cues->cue_points [j + psf->instrument->loop_count * 2].indx ;
				psf->cues->cue_points [j].position = psf->cues->cue_points [j + psf->instrument->loop_count * 2].position ;
				psf->cues->cue_points [j].fcc_chunk = psf->cues->cue_points [j + psf->instrument->loop_count * 2].fcc_chunk ;
				psf->cues->cue_points [j].chunk_start = psf->cues->cue_points [j + psf->instrument->loop_count * 2].chunk_start ;
				psf->cues->cue_points [j].block_start = psf->cues->cue_points [j + psf->instrument->loop_count * 2].block_start ;
				psf->cues->cue_points [j].sample_offset = psf->cues->cue_points [j + psf->instrument->loop_count * 2].sample_offset ;
				for (str_index = 0 ; str_index < 256 ; str_index++)
					psf->cues->cue_points [j].name [str_index] = psf->cues->cue_points [j + psf->instrument->loop_count * 2].name [str_index] ;
				} ;
			psf->cues->cue_count -= psf->instrument->loop_count * 2 ;
			} else
			{	/* All the cues were in fact loop positions so we can actually remove the cues altogether */
				free (psf->cues) ;
				psf->cues = NULL ;
				}
		} ;

	if (psf->sf.channels < 1)
		return SFE_CHANNEL_COUNT_ZERO ;

	if (psf->sf.channels >= SF_MAX_CHANNELS)
		return SFE_CHANNEL_COUNT ;

	if (! (found_chunk & HAVE_FORM))
		return SFE_AIFF_NO_FORM ;

	if (! (found_chunk & HAVE_AIFF))
		return SFE_AIFF_COMM_NO_FORM ;

	if (! (found_chunk & HAVE_COMM))
		return SFE_AIFF_SSND_NO_COMM ;

	if (! psf->dataoffset)
		return SFE_AIFF_NO_DATA ;

	return 0 ;
} /* aiff_read_header */