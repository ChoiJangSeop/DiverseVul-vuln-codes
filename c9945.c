decompress_none(tvbuff_t *tvb, packet_info *pinfo _U_, int offset, guint32 length _U_, tvbuff_t **decompressed_tvb, int *decompressed_offset)
{
    *decompressed_tvb = tvb;
    *decompressed_offset = offset;
    return 1;
}