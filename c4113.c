static PHP_FUNCTION(bzcompress)
{
	char             *source;          /* Source data to compress */
	long              zblock_size = 0; /* Optional block size to use */
	long              zwork_factor = 0;/* Optional work factor to use */
	char             *dest = NULL;     /* Destination to place the compressed data into */
	int               error,           /* Error Container */
					  block_size  = 4, /* Block size for compression algorithm */
					  work_factor = 0, /* Work factor for compression algorithm */
					  argc;            /* Argument count */
	int               source_len;      /* Length of the source data */
	unsigned int      dest_len;        /* Length of the destination buffer */ 

	argc = ZEND_NUM_ARGS();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|ll", &source, &source_len, &zblock_size, &zwork_factor) == FAILURE) {
		return;
	}

	/* Assign them to easy to use variables, dest_len is initially the length of the data
	   + .01 x length of data + 600 which is the largest size the results of the compression 
	   could possibly be, at least that's what the libbz2 docs say (thanks to jeremy@nirvani.net 
	   for pointing this out).  */
	dest_len   = (unsigned int) (source_len + (0.01 * source_len) + 600);
	
	/* Allocate the destination buffer */
	dest = emalloc(dest_len + 1);
	
	/* Handle the optional arguments */
	if (argc > 1) {
		block_size = zblock_size;
	}
	
	if (argc > 2) {
		work_factor = zwork_factor;
	}

	error = BZ2_bzBuffToBuffCompress(dest, &dest_len, source, source_len, block_size, 0, work_factor);
	if (error != BZ_OK) {
		efree(dest);
		RETURN_LONG(error);
	} else {
		/* Copy the buffer, we have perhaps allocate a lot more than we need,
		   so we erealloc() the buffer to the proper size */
		dest = erealloc(dest, dest_len + 1);
		dest[dest_len] = 0;
		RETURN_STRINGL(dest, dest_len, 0);
	}
}