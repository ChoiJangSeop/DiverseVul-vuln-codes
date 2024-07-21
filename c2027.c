init_uncompress( compress_filter_context_t *zfx, z_stream *zs )
{
    int rc;

    /****************
     * PGP uses a windowsize of 13 bits. Using a negative value for
     * it forces zlib not to expect a zlib header.  This is a
     * undocumented feature Peter Gutmann told me about.
     *    
     * We must use 15 bits for the inflator because CryptoEx uses 15
     * bits thus the output would get scrambled w/o error indication
     * if we would use 13 bits.  For the uncompressing this does not
     * matter at all.
     */
    if( (rc = zfx->algo == 1? inflateInit2( zs, -15)
			    : inflateInit( zs )) != Z_OK ) {
	log_fatal("zlib problem: %s\n", zs->msg? zs->msg :
			       rc == Z_MEM_ERROR ? "out of core" :
			       rc == Z_VERSION_ERROR ? "invalid lib version" :
						       "unknown error" );
    }

    zfx->inbufsize = 2048;
    zfx->inbuf = xmalloc( zfx->inbufsize );
    zs->avail_in = 0;
}