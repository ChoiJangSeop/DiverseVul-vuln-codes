main (int argc, char * argv [])
{	const char	*progname, *infilename, *outfilename ;
	SNDFILE		*infile = NULL, *outfile = NULL ;
	SF_INFO		sfinfo ;
	int			k, outfilemajor, outfileminor = 0, infileminor ;
	int			override_sample_rate = 0 ; /* assume no sample rate override. */
	int			endian = SF_ENDIAN_FILE, normalize = SF_FALSE ;

	progname = program_name (argv [0]) ;

	if (argc < 3 || argc > 5)
		usage_exit (progname) ;

	infilename = argv [argc-2] ;
	outfilename = argv [argc-1] ;

	if (strcmp (infilename, outfilename) == 0)
	{	printf ("Error : Input and output filenames are the same.\n\n") ;
		usage_exit (progname) ;
		} ;

	if (strlen (infilename) > 1 && infilename [0] == '-')
	{	printf ("Error : Input filename (%s) looks like an option.\n\n", infilename) ;
		usage_exit (progname) ;
		} ;

	if (outfilename [0] == '-')
	{	printf ("Error : Output filename (%s) looks like an option.\n\n", outfilename) ;
		usage_exit (progname) ;
		} ;

	for (k = 1 ; k < argc - 2 ; k++)
	{	if (! strcmp (argv [k], "-pcms8"))
		{	outfileminor = SF_FORMAT_PCM_S8 ;
			continue ;
			} ;
		if (! strcmp (argv [k], "-pcmu8"))
		{	outfileminor = SF_FORMAT_PCM_U8 ;
			continue ;
			} ;
		if (! strcmp (argv [k], "-pcm16"))
		{	outfileminor = SF_FORMAT_PCM_16 ;
			continue ;
			} ;
		if (! strcmp (argv [k], "-pcm24"))
		{	outfileminor = SF_FORMAT_PCM_24 ;
			continue ;
			} ;
		if (! strcmp (argv [k], "-pcm32"))
		{	outfileminor = SF_FORMAT_PCM_32 ;
			continue ;
			} ;
		if (! strcmp (argv [k], "-float32"))
		{	outfileminor = SF_FORMAT_FLOAT ;
			continue ;
			} ;
		if (! strcmp (argv [k], "-ulaw"))
		{	outfileminor = SF_FORMAT_ULAW ;
			continue ;
			} ;
		if (! strcmp (argv [k], "-alaw"))
		{	outfileminor = SF_FORMAT_ALAW ;
			continue ;
			} ;
		if (! strcmp (argv [k], "-alac16"))
		{	outfileminor = SF_FORMAT_ALAC_16 ;
			continue ;
			} ;
		if (! strcmp (argv [k], "-alac20"))
		{	outfileminor = SF_FORMAT_ALAC_20 ;
			continue ;
			} ;
		if (! strcmp (argv [k], "-alac24"))
		{	outfileminor = SF_FORMAT_ALAC_24 ;
			continue ;
			} ;
		if (! strcmp (argv [k], "-alac32"))
		{	outfileminor = SF_FORMAT_ALAC_32 ;
			continue ;
			} ;
		if (! strcmp (argv [k], "-ima-adpcm"))
		{	outfileminor = SF_FORMAT_IMA_ADPCM ;
			continue ;
			} ;
		if (! strcmp (argv [k], "-ms-adpcm"))
		{	outfileminor = SF_FORMAT_MS_ADPCM ;
			continue ;
			} ;
		if (! strcmp (argv [k], "-gsm610"))
		{	outfileminor = SF_FORMAT_GSM610 ;
			continue ;
			} ;
		if (! strcmp (argv [k], "-dwvw12"))
		{	outfileminor = SF_FORMAT_DWVW_12 ;
			continue ;
			} ;
		if (! strcmp (argv [k], "-dwvw16"))
		{	outfileminor = SF_FORMAT_DWVW_16 ;
			continue ;
			} ;
		if (! strcmp (argv [k], "-dwvw24"))
		{	outfileminor = SF_FORMAT_DWVW_24 ;
			continue ;
			} ;
		if (! strcmp (argv [k], "-vorbis"))
		{	outfileminor = SF_FORMAT_VORBIS ;
			continue ;
			} ;

		if (strstr (argv [k], "-override-sample-rate=") == argv [k])
		{	const char *ptr ;

			ptr = argv [k] + strlen ("-override-sample-rate=") ;
			override_sample_rate = atoi (ptr) ;
			continue ;
			} ;

		if (! strcmp (argv [k], "-endian=little"))
		{	endian = SF_ENDIAN_LITTLE ;
			continue ;
			} ;

		if (! strcmp (argv [k], "-endian=big"))
		{	endian = SF_ENDIAN_BIG ;
			continue ;
			} ;

		if (! strcmp (argv [k], "-endian=cpu"))
		{	endian = SF_ENDIAN_CPU ;
			continue ;
			} ;

		if (! strcmp (argv [k], "-endian=file"))
		{	endian = SF_ENDIAN_FILE ;
			continue ;
			} ;

		if (! strcmp (argv [k], "-normalize"))
		{	normalize = SF_TRUE ;
			continue ;
			} ;

		printf ("Error : Not able to decode argunment '%s'.\n", argv [k]) ;
		exit (1) ;
		} ;

	memset (&sfinfo, 0, sizeof (sfinfo)) ;

	if ((infile = sf_open (infilename, SFM_READ, &sfinfo)) == NULL)
	{	printf ("Not able to open input file %s.\n", infilename) ;
		puts (sf_strerror (NULL)) ;
		return 1 ;
		} ;

	/* Update sample rate if forced to something else. */
	if (override_sample_rate)
		sfinfo.samplerate = override_sample_rate ;

	infileminor = sfinfo.format & SF_FORMAT_SUBMASK ;

	if ((sfinfo.format = sfe_file_type_of_ext (outfilename, sfinfo.format)) == 0)
	{	printf ("Error : Not able to determine output file type for %s.\n", outfilename) ;
		return 1 ;
		} ;

	outfilemajor = sfinfo.format & (SF_FORMAT_TYPEMASK | SF_FORMAT_ENDMASK) ;

	if (outfileminor == 0)
		outfileminor = sfinfo.format & SF_FORMAT_SUBMASK ;

	if (outfileminor != 0)
		sfinfo.format = outfilemajor | outfileminor ;
	else
		sfinfo.format = outfilemajor | (sfinfo.format & SF_FORMAT_SUBMASK) ;

	sfinfo.format |= endian ;

	if ((sfinfo.format & SF_FORMAT_TYPEMASK) == SF_FORMAT_XI)
		switch (sfinfo.format & SF_FORMAT_SUBMASK)
		{	case SF_FORMAT_PCM_16 :
					sfinfo.format = outfilemajor | SF_FORMAT_DPCM_16 ;
					break ;

			case SF_FORMAT_PCM_S8 :
			case SF_FORMAT_PCM_U8 :
					sfinfo.format = outfilemajor | SF_FORMAT_DPCM_8 ;
					break ;
			} ;

	if (sf_format_check (&sfinfo) == 0)
		report_format_error_exit (argv [0], &sfinfo) ;

	if ((sfinfo.format & SF_FORMAT_SUBMASK) == SF_FORMAT_GSM610 && sfinfo.samplerate != 8000)
	{	printf (
			"WARNING: GSM 6.10 data format only supports 8kHz sample rate. The converted\n"
			"ouput file will contain the input data converted to the GSM 6.10 data format\n"
			"but not re-sampled.\n"
			) ;
		} ;

	/* Open the output file. */
	if ((outfile = sf_open (outfilename, SFM_WRITE, &sfinfo)) == NULL)
	{	printf ("Not able to open output file %s : %s\n", outfilename, sf_strerror (NULL)) ;
		return 1 ;
		} ;

	/* Copy the metadata */
	copy_metadata (outfile, infile, sfinfo.channels) ;

	if (normalize
			|| (outfileminor == SF_FORMAT_DOUBLE) || (outfileminor == SF_FORMAT_FLOAT)
			|| (infileminor == SF_FORMAT_DOUBLE) || (infileminor == SF_FORMAT_FLOAT)
			|| (infileminor == SF_FORMAT_VORBIS) || (outfileminor == SF_FORMAT_VORBIS))
		sfe_copy_data_fp (outfile, infile, sfinfo.channels, normalize) ;
	else
		sfe_copy_data_int (outfile, infile, sfinfo.channels) ;

	sf_close (infile) ;
	sf_close (outfile) ;

	return 0 ;
} /* main */