dissect_kafka_message_old(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset, int end_offset _U_)
{
    proto_item  *message_ti;
    proto_tree  *subtree;
    tvbuff_t    *decompressed_tvb;
    int         decompressed_offset;
    int         start_offset = offset;
    int         bytes_offset;
    gint8       magic_byte;
    guint8      codec;
    guint32     message_size;
    guint32     length;

    message_size = tvb_get_guint32(tvb, start_offset + 8, ENC_BIG_ENDIAN);

    subtree = proto_tree_add_subtree(tree, tvb, start_offset, message_size + 12, ett_kafka_message, &message_ti, "Message");

    offset = dissect_kafka_int64(subtree, hf_kafka_offset, tvb, pinfo, offset, NULL);

    offset = dissect_kafka_int32(subtree, hf_kafka_message_size, tvb, pinfo, offset, NULL);

    offset = dissect_kafka_int32(subtree, hf_kafka_message_crc, tvb, pinfo, offset, NULL);

    offset = dissect_kafka_int8(subtree, hf_kafka_message_magic, tvb, pinfo, offset, &magic_byte);

    offset = dissect_kafka_int8(subtree, hf_kafka_message_codec, tvb, pinfo, offset, &codec);
    codec &= KAFKA_MESSAGE_CODEC_MASK;

    offset = dissect_kafka_int8(subtree, hf_kafka_message_timestamp_type, tvb, pinfo, offset, NULL);

    if (magic_byte == 1) {
        proto_tree_add_item(subtree, hf_kafka_message_timestamp, tvb, offset, 8, ENC_TIME_MSECS|ENC_BIG_ENDIAN);
        offset += 8;
    }

    bytes_offset = dissect_kafka_regular_bytes(subtree, hf_kafka_message_key, tvb, pinfo, offset, NULL, NULL);
    if (bytes_offset > offset) {
        offset = bytes_offset;
    } else {
        expert_add_info(pinfo, message_ti, &ei_kafka_bad_bytes_length);
        return offset;
    }

    /*
     * depending on the compression codec, the payload is the actual message payload (codes=none)
     * or compressed set of messages (otherwise). In the new format (since Kafka 1.0) there
     * is no such duality.
     */
    if (codec == 0) {
        bytes_offset = dissect_kafka_regular_bytes(subtree, hf_kafka_message_value, tvb, pinfo, offset, NULL, &length);
        if (bytes_offset > offset) {
            offset = bytes_offset;
        } else {
            expert_add_info(pinfo, message_ti, &ei_kafka_bad_bytes_length);
            return offset;
        }
    } else {
        length = tvb_get_ntohl(tvb, offset);
        offset += 4;
        if (decompress(tvb, pinfo, offset, length, codec, &decompressed_tvb, &decompressed_offset)==1) {
            add_new_data_source(pinfo, decompressed_tvb, "Decompressed content");
            show_compression_reduction(tvb, subtree, length, tvb_captured_length(decompressed_tvb));
            dissect_kafka_message_set(decompressed_tvb, pinfo, subtree, decompressed_offset,
                tvb_reported_length_remaining(decompressed_tvb, decompressed_offset), codec);
        } else {
            proto_item_append_text(subtree, " [Cannot decompress records]");
        }
        offset += length;
    }

    proto_item_set_end(message_ti, tvb, offset);

    return offset;
}