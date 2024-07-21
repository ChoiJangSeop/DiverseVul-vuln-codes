decompress_lz4(tvbuff_t *tvb, packet_info *pinfo, int offset, guint32 length, tvbuff_t **decompressed_tvb, int *decompressed_offset)
{
    LZ4F_decompressionContext_t lz4_ctxt = NULL;
    LZ4F_frameInfo_t lz4_info;
    LZ4F_errorCode_t rc = 0;
    size_t src_offset = 0, src_size = 0, dst_size = 0;
    guchar *decompressed_buffer = NULL;
    tvbuff_t *composite_tvb = NULL;

    int ret = 0;

    /* Prepare compressed data buffer */
    guint8 *data = (guint8*)tvb_memdup(wmem_packet_scope(), tvb, offset, length);
    /* Override header checksum to workaround buggy Kafka implementations */
    if (length > 7) {
        guint32 hdr_end = 6;
        if (data[4] & 0x08) {
            hdr_end += 8;
        }
        if (hdr_end < length) {
            data[hdr_end] = (XXH32(&data[4], hdr_end - 4, 0) >> 8) & 0xff;
        }
    }

    /* Allocate output buffer */
    rc = LZ4F_createDecompressionContext(&lz4_ctxt, LZ4F_VERSION);
    if (LZ4F_isError(rc)) {
        goto end;
    }

    src_offset = length;
    rc = LZ4F_getFrameInfo(lz4_ctxt, &lz4_info, data, &src_offset);
    if (LZ4F_isError(rc)) {
        goto end;
    }

    switch (lz4_info.blockSizeID) {
        case LZ4F_max64KB:
            dst_size = 1 << 16;
            break;
        case LZ4F_max256KB:
            dst_size = 1 << 18;
            break;
        case LZ4F_max1MB:
            dst_size = 1 << 20;
            break;
        case LZ4F_max4MB:
            dst_size = 1 << 22;
            break;
        default:
            goto end;
    }

    if (lz4_info.contentSize && lz4_info.contentSize < dst_size) {
        dst_size = (size_t)lz4_info.contentSize;
    }

    do {
        src_size = length - src_offset; // set the number of available octets
        if (src_size == 0) {
            goto end;
        }
        decompressed_buffer = (guchar*)wmem_alloc(pinfo->pool, dst_size);
        rc = LZ4F_decompress(lz4_ctxt, decompressed_buffer, &dst_size,
                              &data[src_offset], &src_size, NULL);
        if (LZ4F_isError(rc)) {
            goto end;
        }
        if (dst_size == 0) {
            goto end;
        }
        if (!composite_tvb) {
            composite_tvb = tvb_new_composite();
        }
        tvb_composite_append(composite_tvb,
                             tvb_new_child_real_data(tvb, (guint8*)decompressed_buffer, (guint)dst_size, (gint)dst_size));
        src_offset += src_size; // bump up the offset for the next iteration
    } while (rc > 0);

    ret = 1;
end:
    if (composite_tvb) {
        tvb_composite_finalize(composite_tvb);
    }
    LZ4F_freeDecompressionContext(lz4_ctxt);
    if (ret == 1) {
        *decompressed_tvb = composite_tvb;
        *decompressed_offset = 0;
    }
    else {
        col_append_str(pinfo->cinfo, COL_INFO, " [lz4 decompression failed]");
    }
    return ret;
}