opj_codec_t* OPJ_CALLCONV opj_create_compress(OPJ_CODEC_FORMAT p_format)
{
    opj_codec_private_t *l_codec = 00;

    l_codec = (opj_codec_private_t*)opj_calloc(1, sizeof(opj_codec_private_t));
    if (!l_codec) {
        return 00;
    }

    l_codec->is_decompressor = 0;

    switch (p_format) {
    case OPJ_CODEC_J2K:
        l_codec->m_codec_data.m_compression.opj_encode = (OPJ_BOOL(*)(void *,
                struct opj_stream_private *,
                struct opj_event_mgr *)) opj_j2k_encode;

        l_codec->m_codec_data.m_compression.opj_end_compress = (OPJ_BOOL(*)(void *,
                struct opj_stream_private *,
                struct opj_event_mgr *)) opj_j2k_end_compress;

        l_codec->m_codec_data.m_compression.opj_start_compress = (OPJ_BOOL(*)(void *,
                struct opj_stream_private *,
                struct opj_image *,
                struct opj_event_mgr *)) opj_j2k_start_compress;

        l_codec->m_codec_data.m_compression.opj_write_tile = (OPJ_BOOL(*)(void *,
                OPJ_UINT32,
                OPJ_BYTE*,
                OPJ_UINT32,
                struct opj_stream_private *,
                struct opj_event_mgr *)) opj_j2k_write_tile;

        l_codec->m_codec_data.m_compression.opj_destroy = (void (*)(
                    void *)) opj_j2k_destroy;

        l_codec->m_codec_data.m_compression.opj_setup_encoder = (OPJ_BOOL(*)(void *,
                opj_cparameters_t *,
                struct opj_image *,
                struct opj_event_mgr *)) opj_j2k_setup_encoder;

        l_codec->m_codec = opj_j2k_create_compress();
        if (! l_codec->m_codec) {
            opj_free(l_codec);
            return 00;
        }

        break;

    case OPJ_CODEC_JP2:
        /* get a JP2 decoder handle */
        l_codec->m_codec_data.m_compression.opj_encode = (OPJ_BOOL(*)(void *,
                struct opj_stream_private *,
                struct opj_event_mgr *)) opj_jp2_encode;

        l_codec->m_codec_data.m_compression.opj_end_compress = (OPJ_BOOL(*)(void *,
                struct opj_stream_private *,
                struct opj_event_mgr *)) opj_jp2_end_compress;

        l_codec->m_codec_data.m_compression.opj_start_compress = (OPJ_BOOL(*)(void *,
                struct opj_stream_private *,
                struct opj_image *,
                struct opj_event_mgr *))  opj_jp2_start_compress;

        l_codec->m_codec_data.m_compression.opj_write_tile = (OPJ_BOOL(*)(void *,
                OPJ_UINT32,
                OPJ_BYTE*,
                OPJ_UINT32,
                struct opj_stream_private *,
                struct opj_event_mgr *)) opj_jp2_write_tile;

        l_codec->m_codec_data.m_compression.opj_destroy = (void (*)(
                    void *)) opj_jp2_destroy;

        l_codec->m_codec_data.m_compression.opj_setup_encoder = (OPJ_BOOL(*)(void *,
                opj_cparameters_t *,
                struct opj_image *,
                struct opj_event_mgr *)) opj_jp2_setup_encoder;

        l_codec->m_codec = opj_jp2_create(OPJ_FALSE);
        if (! l_codec->m_codec) {
            opj_free(l_codec);
            return 00;
        }

        break;

    case OPJ_CODEC_UNKNOWN:
    case OPJ_CODEC_JPT:
    default:
        opj_free(l_codec);
        return 00;
    }

    opj_set_default_event_handler(&(l_codec->m_event_mgr));
    return (opj_codec_t*) l_codec;
}