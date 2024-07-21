decompress_zstd(tvbuff_t *tvb, packet_info *pinfo, int offset, guint32 length, tvbuff_t **decompressed_tvb, int *decompressed_offset)
{
    ZSTD_inBuffer input = { tvb_memdup(wmem_packet_scope(), tvb, offset, length), length, 0 };
    ZSTD_DStream *zds = ZSTD_createDStream();
    size_t rc = 0;
    tvbuff_t *composite_tvb = NULL;
    int ret = 0;

    do {
        ZSTD_outBuffer output = { wmem_alloc(pinfo->pool, ZSTD_DStreamOutSize()), ZSTD_DStreamOutSize(), 0 };
        rc = ZSTD_decompressStream(zds, &output, &input);
        // rc holds either the number of decompressed offsets or the error code.
        // Both values are positive, one has to use ZSTD_isError to determine if the call succeeded.
        if (ZSTD_isError(rc)) {
            goto end;
        }
        if (!composite_tvb) {
            composite_tvb = tvb_new_composite();
        }
        tvb_composite_append(composite_tvb,
                             tvb_new_child_real_data(tvb, (guint8*)output.dst, (guint)output.pos, (gint)output.pos));
        // rc == 0 means there is nothing more to decompress, but there could be still something in the data
    } while (rc > 0);
    ret = 1;
end:
    if (composite_tvb) {
        tvb_composite_finalize(composite_tvb);
    }
    ZSTD_freeDStream(zds);
    if (ret == 1) {
        *decompressed_tvb = composite_tvb;
        *decompressed_offset = 0;
    }
    else {
        col_append_str(pinfo->cinfo, COL_INFO, " [zstd decompression failed]");
    }
    return ret;
}