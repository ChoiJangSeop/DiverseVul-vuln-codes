decompress_snappy(tvbuff_t *tvb, packet_info *pinfo, int offset, guint32 length, tvbuff_t **decompressed_tvb, int *decompressed_offset)
{
    guint8 *data = (guint8*)tvb_memdup(wmem_packet_scope(), tvb, offset, length);
    size_t uncompressed_size;
    snappy_status rc = SNAPPY_OK;
    tvbuff_t *composite_tvb = NULL;
    int ret = 0;

    if (tvb_memeql(tvb, offset, kafka_xerial_header, sizeof(kafka_xerial_header)) == 0) {

        /* xerial framing format */
        guint32 chunk_size, pos = 16;

        while (pos < length) {
            if (pos > length-4) {
                // XXX - this is presumably an error, as the chunk size
                // doesn't fully fit in the data, so an error should be
                // reported.
                goto end;
            }
            chunk_size = tvb_get_ntohl(tvb, offset+pos);
            pos += 4;
            if (chunk_size > length) {
                // XXX - this is presumably an error, as the chunk to be
                // decompressed doesn't fully fit in the data, so an error
                // should be reported.
                goto end;
            }
            if (pos > length-chunk_size) {
                // XXX - this is presumably an error, as the chunk to be
                // decompressed doesn't fully fit in the data, so an error
                // should be reported.
                goto end;
            }
            rc = snappy_uncompressed_length(&data[pos], chunk_size, &uncompressed_size);
            if (rc != SNAPPY_OK) {
                goto end;
            }
            guint8 *decompressed_buffer = (guint8*)wmem_alloc(pinfo->pool, uncompressed_size);
            rc = snappy_uncompress(&data[pos], chunk_size, decompressed_buffer, &uncompressed_size);
            if (rc != SNAPPY_OK) {
                goto end;
            }

            if (!composite_tvb) {
                composite_tvb = tvb_new_composite();
            }
            tvb_composite_append(composite_tvb,
                      tvb_new_child_real_data(tvb, decompressed_buffer, (guint)uncompressed_size, (gint)uncompressed_size));
            pos += chunk_size;
        }

    } else {

        /* unframed format */
        rc = snappy_uncompressed_length(data, length, &uncompressed_size);
        if (rc != SNAPPY_OK) {
            goto end;
        }

        guint8 *decompressed_buffer = (guint8*)wmem_alloc(pinfo->pool, uncompressed_size);

        rc = snappy_uncompress(data, length, decompressed_buffer, &uncompressed_size);
        if (rc != SNAPPY_OK) {
            goto end;
        }

        *decompressed_tvb = tvb_new_child_real_data(tvb, decompressed_buffer, (guint)uncompressed_size, (gint)uncompressed_size);
        *decompressed_offset = 0;

    }
    ret = 1;
end:
    if (composite_tvb) {
        tvb_composite_finalize(composite_tvb);
        if (ret == 1) {
            *decompressed_tvb = composite_tvb;
            *decompressed_offset = 0;
        }
    }
    if (ret == 0) {
        col_append_str(pinfo->cinfo, COL_INFO, " [snappy decompression failed]");
    }
    return ret;
}