cdata_finish (cdata_t *cd, GError **error)
{
    z_stream *z = &cd->z;
    int zret;

    if (cd->decomp.comptype == GCAB_COMPRESSION_LZX) {
        LZXfdi_clear (&cd->decomp);
        return;
    }

    if (!z->opaque)
        return;

    zret = inflateEnd (z);
    z->opaque = NULL;

    if (zret != Z_OK)
        g_set_error (error, GCAB_ERROR, GCAB_ERROR_FAILED,
                     "zlib failed: %s", zError (zret));
}