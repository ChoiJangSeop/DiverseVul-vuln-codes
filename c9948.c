decompress(tvbuff_t *tvb, packet_info *pinfo, int offset, guint32 length, int codec, tvbuff_t **decompressed_tvb, int *decompressed_offset)
{
    switch (codec) {
        case KAFKA_MESSAGE_CODEC_SNAPPY:
            return decompress_snappy(tvb, pinfo, offset, length, decompressed_tvb, decompressed_offset);
        case KAFKA_MESSAGE_CODEC_LZ4:
            return decompress_lz4(tvb, pinfo, offset, length, decompressed_tvb, decompressed_offset);
        case KAFKA_MESSAGE_CODEC_ZSTD:
            return decompress_zstd(tvb, pinfo, offset, length, decompressed_tvb, decompressed_offset);
        case KAFKA_MESSAGE_CODEC_GZIP:
            return decompress_gzip(tvb, pinfo, offset, length, decompressed_tvb, decompressed_offset);
        case KAFKA_MESSAGE_CODEC_NONE:
            return decompress_none(tvb, pinfo, offset, length, decompressed_tvb, decompressed_offset);
        default:
            col_append_str(pinfo->cinfo, COL_INFO, " [unsupported compression type]");
            return 0;
    }
}