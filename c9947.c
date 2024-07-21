decompress_gzip(tvbuff_t *tvb, packet_info *pinfo, int offset, guint32 length, tvbuff_t **decompressed_tvb, int *decompressed_offset)
{
    *decompressed_tvb = tvb_child_uncompress(tvb, tvb, offset, length);
    *decompressed_offset = 0;
    if (*decompressed_tvb) {
        return 1;
    } else {
        col_append_str(pinfo->cinfo, COL_INFO, " [gzip decompression failed] ");
        return 0;
    }
}