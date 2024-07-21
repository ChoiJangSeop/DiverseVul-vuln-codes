sfe_copy_data_fp (SNDFILE *outfile, SNDFILE *infile, int channels, int normalize)
{	static double	data [BUFFER_LEN], max ;
	sf_count_t		frames, readcount, k ;

	frames = BUFFER_LEN / channels ;
	readcount = frames ;

	sf_command (infile, SFC_CALC_SIGNAL_MAX, &max, sizeof (max)) ;

	if (!normalize && max < 1.0)
	{	while (readcount > 0)
		{	readcount = sf_readf_double (infile, data, frames) ;
			sf_writef_double (outfile, data, readcount) ;
			} ;
		}
	else
	{	sf_command (infile, SFC_SET_NORM_DOUBLE, NULL, SF_FALSE) ;

		while (readcount > 0)
		{	readcount = sf_readf_double (infile, data, frames) ;
			for (k = 0 ; k < readcount * channels ; k++)
				data [k] /= max ;
			sf_writef_double (outfile, data, readcount) ;
			} ;
		} ;

	return ;
} /* sfe_copy_data_fp */