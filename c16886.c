sfe_apply_metadata_changes (const char * filenames [2], const METADATA_INFO * info)
{	SNDFILE *infile = NULL, *outfile = NULL ;
	SF_INFO sfinfo ;
	METADATA_INFO tmpinfo ;
	int error_code = 0 ;

	memset (&sfinfo, 0, sizeof (sfinfo)) ;
	memset (&tmpinfo, 0, sizeof (tmpinfo)) ;

	if (filenames [1] == NULL)
		infile = outfile = sf_open (filenames [0], SFM_RDWR, &sfinfo) ;
	else
	{	infile = sf_open (filenames [0], SFM_READ, &sfinfo) ;

		/* Output must be WAV. */
		sfinfo.format = SF_FORMAT_WAV | (SF_FORMAT_SUBMASK & sfinfo.format) ;
		outfile = sf_open (filenames [1], SFM_WRITE, &sfinfo) ;
		} ;

	if (infile == NULL)
	{	printf ("Error : Not able to open input file '%s' : %s\n", filenames [0], sf_strerror (infile)) ;
		error_code = 1 ;
		goto cleanup_exit ;
		} ;

	if (outfile == NULL)
	{	printf ("Error : Not able to open output file '%s' : %s\n", filenames [1], sf_strerror (outfile)) ;
		error_code = 1 ;
		goto cleanup_exit ;
		} ;

	if (info->has_bext_fields && merge_broadcast_info (infile, outfile, sfinfo.format, info))
	{	error_code = 1 ;
		goto cleanup_exit ;
		} ;

	if (infile != outfile)
	{	int infileminor = SF_FORMAT_SUBMASK & sfinfo.format ;

		/* If the input file is not the same as the output file, copy the data. */
		if ((infileminor == SF_FORMAT_DOUBLE) || (infileminor == SF_FORMAT_FLOAT))
			sfe_copy_data_fp (outfile, infile, sfinfo.channels, SF_FALSE) ;
		else
			sfe_copy_data_int (outfile, infile, sfinfo.channels) ;
		} ;

	update_strings (outfile, info) ;

cleanup_exit :

	if (outfile != NULL && outfile != infile)
		sf_close (outfile) ;

	if (infile != NULL)
		sf_close (infile) ;

	if (error_code)
		exit (error_code) ;

	return ;
} /* sfe_apply_metadata_changes */