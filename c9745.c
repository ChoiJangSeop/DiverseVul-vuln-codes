proto_register_gquic(void)
{
    module_t *gquic_module;

    static hf_register_info hf[] = {
        /* Long/Short header for Q046 */
        { &hf_gquic_header_form,
          { "Header Form", "gquic.header_form",
            FT_UINT8, BASE_DEC, VALS(gquic_short_long_header_vals), 0x80,
            "The most significant bit (0x80) of the first octet is set to 1 for long headers and 0 for short headers.", HFILL }
        },
        { &hf_gquic_fixed_bit,
          { "Fixed Bit", "gquic.fixed_bit",
            FT_BOOLEAN, 8, NULL, 0x40,
            "Must be 1", HFILL }
        },
        { &hf_gquic_long_packet_type,
          { "Packet Type", "gquic.long.packet_type",
            FT_UINT8, BASE_DEC, VALS(gquic_long_packet_type_vals), 0x30,
            "Long Header Packet Type", HFILL }
        },
        { &hf_gquic_long_reserved,
          { "Reserved", "gquic.long.reserved",
            FT_UINT8, BASE_DEC, NULL, 0x0c,
            "Reserved bits", HFILL }
        },
        { &hf_gquic_packet_number_length,
          { "Packet Number Length", "gquic.packet_number_length",
            FT_UINT8, BASE_DEC, VALS(gquic_packet_number_lengths), 0x03,
            "Packet Number field length", HFILL }
	},
        { &hf_gquic_dcil,
          { "Destination Connection ID Length", "gquic.dcil",
            FT_UINT8, BASE_DEC, VALS(quic_cid_lengths), 0xF0,
            NULL, HFILL }
        },
        { &hf_gquic_scil,
          { "Source Connection ID Length", "gquic.scil",
            FT_UINT8, BASE_DEC, VALS(quic_cid_lengths), 0x0F,
            NULL, HFILL }
        },

        /* Public header for < Q046 */
        { &hf_gquic_puflags,
            { "Public Flags", "gquic.puflags",
              FT_UINT8, BASE_HEX, NULL, 0x0,
              "Specifying per-packet public flags", HFILL }
        },
        { &hf_gquic_puflags_vrsn,
            { "Version", "gquic.puflags.version",
              FT_BOOLEAN, 8, TFS(&tfs_yes_no), PUFLAGS_VRSN,
              "Signifies that this packet also contains the version of the (Google)QUIC protocol", HFILL }
        },
        { &hf_gquic_puflags_rst,
            { "Reset", "gquic.puflags.reset",
              FT_BOOLEAN, 8, TFS(&tfs_yes_no), PUFLAGS_RST,
              "Signifies that this packet is a public reset packet", HFILL }
        },
        { &hf_gquic_puflags_dnonce,
            { "Diversification nonce", "gquic.puflags.nonce",
              FT_BOOLEAN, 8, TFS(&tfs_yes_no), PUFLAGS_DNONCE,
              "Indicates the presence of a 32 byte diversification nonce", HFILL }
        },
        { &hf_gquic_puflags_cid,
            { "CID Length", "gquic.puflags.cid",
              FT_BOOLEAN, 8, TFS(&puflags_cid_tfs), PUFLAGS_CID,
              "Indicates the full 8 byte Connection ID is present", HFILL }
        },
        { &hf_gquic_puflags_cid_old,
            { "CID Length", "gquic.puflags.cid.old",
              FT_UINT8, BASE_HEX, VALS(puflags_cid_old_vals), PUFLAGS_CID_OLD,
              "Signifies the Length of CID", HFILL }
        },
        { &hf_gquic_puflags_pkn,
            { "Packet Number Length", "gquic.puflags.pkn",
              FT_UINT8, BASE_HEX, VALS(puflags_pkn_vals), PUFLAGS_PKN,
              "Signifies the Length of packet number", HFILL }
        },
        { &hf_gquic_puflags_mpth,
            { "Multipath", "gquic.puflags.mpth",
              FT_BOOLEAN, 8, TFS(&tfs_yes_no), PUFLAGS_MPTH,
              "Reserved for multipath use", HFILL }
        },
        { &hf_gquic_puflags_rsv,
            { "Reserved", "gquic.puflags.rsv",
              FT_UINT8, BASE_HEX, NULL, PUFLAGS_RSV,
              "Must be Zero", HFILL }
        },
        { &hf_gquic_cid,
            { "CID", "gquic.cid",
              FT_UINT64, BASE_DEC, NULL, 0x0,
              "Connection ID 64 bit pseudo random number", HFILL }
        },
        { &hf_gquic_version,
            { "Version", "gquic.version",
              FT_STRING, BASE_NONE, NULL, 0x0,
              "32 bit opaque tag that represents the version of the (Google)QUIC", HFILL }
        },
        { &hf_gquic_diversification_nonce,
            { "Diversification nonce", "gquic.diversification_nonce",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_packet_number,
            { "Packet Number", "gquic.packet_number",
              FT_UINT64, BASE_DEC, NULL, 0x0,
              "The lower 8, 16, 32, or 48 bits of the packet number", HFILL }
        },

        { &hf_gquic_prflags,
            { "Private Flags", "gquic.prflags",
              FT_UINT8, BASE_HEX, NULL, 0x0,
              "Specifying per-packet Private flags", HFILL }
        },

        { &hf_gquic_prflags_entropy,
            { "Entropy", "gquic.prflags.entropy",
              FT_BOOLEAN, 8, TFS(&tfs_yes_no), PRFLAGS_ENTROPY,
              "For data packets, signifies that this packet contains the 1 bit of entropy, for fec packets, contains the xor of the entropy of protected packets", HFILL }
        },
        { &hf_gquic_prflags_fecg,
            { "FEC Group", "gquic.prflags.fecg",
              FT_BOOLEAN, 8, TFS(&tfs_yes_no), PRFLAGS_FECG,
              "Indicates whether the fec byte is present.", HFILL }
        },
        { &hf_gquic_prflags_fec,
            { "FEC", "gquic.prflags.fec",
              FT_BOOLEAN, 8, TFS(&tfs_yes_no), PRFLAGS_FEC,
              "Signifies that this packet represents an FEC packet", HFILL }
        },
        { &hf_gquic_prflags_rsv,
            { "Reserved", "gquic.prflags.rsv",
              FT_UINT8, BASE_HEX, NULL, PRFLAGS_RSV,
              "Must be Zero", HFILL }
        },

        { &hf_gquic_message_authentication_hash,
            { "Message Authentication Hash", "gquic.message_authentication_hash",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              "The hash is an FNV1a-128 hash, serialized in little endian order", HFILL }
        },
        { &hf_gquic_frame,
            { "Frame", "gquic.frame",
              FT_NONE, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_frame_type,
            { "Frame Type", "gquic.frame_type",
              FT_UINT8 ,BASE_RANGE_STRING | BASE_HEX, RVALS(frame_type_vals), 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_frame_type_padding_length,
            { "Padding Length", "gquic.frame_type.padding.length",
              FT_UINT32, BASE_DEC, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_frame_type_padding,
            { "Padding", "gquic.frame_type.padding",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              "Must be zero", HFILL }
        },
        { &hf_gquic_frame_type_rsts_stream_id,
            { "Stream ID", "gquic.frame_type.rsts.stream_id",
              FT_UINT32, BASE_DEC, NULL, 0x0,
              "Stream ID of the stream being terminated", HFILL }
        },
        { &hf_gquic_frame_type_rsts_byte_offset,
            { "Byte offset", "gquic.frame_type.rsts.byte_offset",
              FT_UINT64, BASE_DEC, NULL, 0x0,
              "Indicating the absolute byte offset of the end of data for this stream", HFILL }
        },
        { &hf_gquic_frame_type_rsts_error_code,
            { "Error code", "gquic.frame_type.rsts.error_code",
              FT_UINT32, BASE_DEC|BASE_EXT_STRING, &rststream_error_code_vals_ext, 0x0,
              "Indicates why the stream is being closed", HFILL }
        },
        { &hf_gquic_frame_type_cc_error_code,
            { "Error code", "gquic.frame_type.cc.error_code",
              FT_UINT32, BASE_DEC|BASE_EXT_STRING, &error_code_vals_ext, 0x0,
              "Indicates the reason for closing this connection", HFILL }
        },
        { &hf_gquic_frame_type_cc_reason_phrase_length,
            { "Reason phrase Length", "gquic.frame_type.cc.reason_phrase.length",
              FT_UINT16, BASE_DEC, NULL, 0x0,
              "Specifying the length of the reason phrase", HFILL }
        },
        { &hf_gquic_frame_type_cc_reason_phrase,
            { "Reason phrase", "gquic.frame_type.cc.reason_phrase",
              FT_STRING, BASE_NONE, NULL, 0x0,
              "An optional human-readable explanation for why the connection was closed", HFILL }
        },
        { &hf_gquic_frame_type_goaway_error_code,
            { "Error code", "gquic.frame_type.goaway.error_code",
              FT_UINT32, BASE_DEC|BASE_EXT_STRING, &error_code_vals_ext, 0x0,
              "Indicates the reason for closing this connection", HFILL }
        },
        { &hf_gquic_frame_type_goaway_last_good_stream_id,
            { "Last Good Stream ID", "gquic.frame_type.goaway.last_good_stream_id",
              FT_UINT32, BASE_DEC, NULL, 0x0,
              "last Stream ID which was accepted by the sender of the GOAWAY message", HFILL }
        },
        { &hf_gquic_frame_type_goaway_reason_phrase_length,
            { "Reason phrase Length", "gquic.frame_type.goaway.reason_phrase.length",
              FT_UINT16, BASE_DEC, NULL, 0x0,
              "Specifying the length of the reason phrase", HFILL }
        },
        { &hf_gquic_frame_type_goaway_reason_phrase,
            { "Reason phrase", "gquic.frame_type.goaway.reason_phrase",
              FT_STRING, BASE_NONE, NULL, 0x0,
              "An optional human-readable explanation for why the connection was closed", HFILL }
        },
        { &hf_gquic_frame_type_wu_stream_id,
            { "Stream ID", "gquic.frame_type.wu.stream_id",
              FT_UINT32, BASE_DEC, NULL, 0x0,
              "ID of the stream whose flow control windows is begin updated, or 0 to specify the connection-level flow control window", HFILL }
        },
        { &hf_gquic_frame_type_wu_byte_offset,
            { "Byte offset", "gquic.frame_type.wu.byte_offset",
              FT_UINT64, BASE_DEC, NULL, 0x0,
              "Indicating the absolute byte offset of data which can be sent on the given stream", HFILL }
        },
        { &hf_gquic_frame_type_blocked_stream_id,
            { "Stream ID", "gquic.frame_type.blocked.stream_id",
              FT_UINT32, BASE_DEC, NULL, 0x0,
              "Indicating the stream which is flow control blocked", HFILL }
        },
        { &hf_gquic_frame_type_sw_send_entropy,
            { "Send Entropy", "gquic.frame_type.sw.send_entropy",
              FT_UINT8, BASE_DEC, NULL, 0x0,
              "Specifying the cumulative hash of entropy in all sent packets up to the packet with packet number one less than the least unacked packet", HFILL }
        },
        { &hf_gquic_frame_type_sw_least_unacked_delta,
            { "Least unacked delta", "gquic.frame_type.sw.least_unacked_delta",
              FT_UINT64, BASE_DEC, NULL, 0x0,
              "A variable length packet number delta with the same length as the packet header's packet number", HFILL }
        },
        { &hf_gquic_crypto_offset,
            { "Offset", "gquic.crypto.offset",
              FT_UINT64, BASE_DEC, NULL, 0x0,
              "Byte offset into the stream", HFILL }
        },
        { &hf_gquic_crypto_length,
            { "Length", "gquic.crypto.length",
              FT_UINT64, BASE_DEC, NULL, 0x0,
              "Length of the Crypto Data field", HFILL }
        },
        { &hf_gquic_crypto_crypto_data,
            { "Crypto Data", "gquic.crypto.crypto_data",
              FT_NONE, BASE_NONE, NULL, 0x0,
              "The cryptographic message data", HFILL }
        },
        { &hf_gquic_frame_type_stream,
            { "Stream", "gquic.frame_type.stream",
              FT_BOOLEAN, 8, NULL, FTFLAGS_STREAM,
              NULL, HFILL }
        },
        { &hf_gquic_frame_type_stream_f,
            { "FIN", "gquic.frame_type.stream.f",
              FT_BOOLEAN, 8, NULL, FTFLAGS_STREAM_F,
              NULL, HFILL }
        },
        { &hf_gquic_frame_type_stream_d,
            { "Data Length", "gquic.frame_type.stream.d",
              FT_BOOLEAN, 8, TFS(&len_data_vals), FTFLAGS_STREAM_D,
              NULL, HFILL }
        },
        { &hf_gquic_frame_type_stream_ooo,
            { "Offset Length", "gquic.frame_type.stream.ooo",
              FT_UINT8, BASE_DEC, VALS(len_offset_vals), FTFLAGS_STREAM_OOO,
              NULL, HFILL }
        },
        { &hf_gquic_frame_type_stream_ss,
            { "Stream Length", "gquic.frame_type.stream.ss",
              FT_UINT8, BASE_DEC, VALS(len_stream_vals), FTFLAGS_STREAM_SS,
              NULL, HFILL }
        },
        { &hf_gquic_frame_type_ack,
            { "ACK", "gquic.frame_type.ack",
              FT_BOOLEAN, 8, NULL, FTFLAGS_ACK,
              NULL, HFILL }
        },
        { &hf_gquic_frame_type_ack_n,
            { "NACK", "gquic.frame_type.ack.n",
              FT_BOOLEAN, 8, NULL, FTFLAGS_ACK_N,
              NULL, HFILL }
        },
        { &hf_gquic_frame_type_ack_u,
            { "Unused", "gquic.frame_type.ack.u",
              FT_BOOLEAN, 8, NULL, FTFLAGS_ACK_U,
              NULL, HFILL }
        },
        { &hf_gquic_frame_type_ack_t,
            { "Truncated", "gquic.frame_type.ack.t",
              FT_BOOLEAN, 8, NULL, FTFLAGS_ACK_T,
              NULL, HFILL }
        },
        { &hf_gquic_frame_type_ack_ll,
            { "Largest Observed Length", "gquic.frame_type.ack.ll",
              FT_UINT8, BASE_DEC, VALS(len_largest_observed_vals), FTFLAGS_ACK_LL,
              "Length of the Largest Observed field as 1, 2, 4, or 6 bytes long", HFILL }
        },
        { &hf_gquic_frame_type_ack_mm,
            { "Missing Packet Length", "gquic.frame_type.ack.mm",
              FT_UINT8, BASE_DEC, VALS(len_missing_packet_vals), FTFLAGS_ACK_MM,
              "Length of the Missing Packet Number Delta field as 1, 2, 4, or 6 bytes long", HFILL }
        },
        /* ACK before Q034 */
        { &hf_gquic_frame_type_ack_received_entropy,
            { "Received Entropy", "gquic.frame_type.ack.received_entropy",
              FT_UINT8, BASE_DEC, NULL, 0x0,
              "Specifying the cumulative hash of entropy in all received packets up to the largest observed packet", HFILL }
        },
        { &hf_gquic_frame_type_ack_largest_observed,
            { "Largest Observed", "gquic.frame_type.ack.largest_observed",
              FT_UINT64, BASE_DEC, NULL, 0x0,
              "Representing the largest packet number the peer has observed", HFILL }
        },
        { &hf_gquic_frame_type_ack_ack_delay_time,
            { "Ack Delay time", "gquic.frame_type.ack.ack_delay_time",
              FT_UINT16, BASE_DEC, NULL, 0x0,
              "Specifying the time elapsed in microseconds from when largest observed was received until this Ack frame was sent", HFILL }
        },
        { &hf_gquic_frame_type_ack_num_timestamp,
            { "Num Timestamp", "gquic.frame_type.ack.num_timestamp",
              FT_UINT8, BASE_DEC, NULL, 0x0,
              "Specifying the number of TCP timestamps that are included in this frame", HFILL }
        },
        { &hf_gquic_frame_type_ack_delta_largest_observed,
            { "Delta Largest Observed", "gquic.frame_type.ack.delta_largest_observed",
              FT_UINT8, BASE_DEC, NULL, 0x0,
              "Specifying the packet number delta from the first timestamp to the largest observed", HFILL }
        },
        { &hf_gquic_frame_type_ack_first_timestamp,
            { "First Timestamp", "gquic.frame_type.ack.first_timestamp",
              FT_UINT32, BASE_DEC, NULL, 0x0,
              "Specifying the time delta in microseconds, from the beginning of the connection of the arrival of the packet specified by Largest Observed minus Delta Largest Observed", HFILL }
        },
        { &hf_gquic_frame_type_ack_time_since_previous_timestamp,
            { "Time since Previous timestamp", "gquic.frame_type.ack.time_since_previous_timestamp",
              FT_UINT16, BASE_DEC, NULL, 0x0,
              "This is the time delta from the previous timestamp", HFILL }
        },
        { &hf_gquic_frame_type_ack_num_ranges,
            { "Num Ranges", "gquic.frame_type.ack.num_ranges",
              FT_UINT8, BASE_DEC, NULL, 0x0,
              "Specifying the number of missing packet ranges between largest observed and least unacked", HFILL }
        },
        { &hf_gquic_frame_type_ack_missing_packet,
            { "Missing Packet Packet Number Delta", "gquic.frame_type.ack.missing_packet",
              FT_UINT64, BASE_DEC, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_frame_type_ack_range_length,
            { "Range Length", "gquic.frame_type.ack.range_length",
              FT_UINT8, BASE_DEC, NULL, 0x0,
              "Specifying one less than the number of sequential nacks in the range", HFILL }
        },
        { &hf_gquic_frame_type_ack_num_revived,
            { "Num Revived", "gquic.frame_type.ack.num_revived",
              FT_UINT8, BASE_DEC, NULL, 0x0,
              "Specifying the number of revived packets, recovered via FEC", HFILL }
        },
        { &hf_gquic_frame_type_ack_revived_packet,
            { "Revived Packet Packet Number", "gquic.frame_type.ack.revived_packet",
              FT_UINT64, BASE_DEC, NULL, 0x0,
              "Representing a packet the peer has revived via FEC", HFILL }
        },
        /* ACK after Q034 */
        { &hf_gquic_frame_type_ack_largest_acked,
            { "Largest Acked", "gquic.frame_type.ack.largest_acked",
              FT_UINT64, BASE_DEC, NULL, 0x0,
              "Representing the largest packet number the peer has observed", HFILL }
        },
        { &hf_gquic_frame_type_ack_largest_acked_delta_time,
            { "Largest Acked Delta Time", "gquic.frame_type.ack.largest_acked_delta_time",
              FT_UINT16, BASE_DEC, NULL, 0x0,
              "Specifying the time elapsed in microseconds from when largest acked was received until this Ack frame was sent", HFILL }
        },
        { &hf_gquic_frame_type_ack_num_blocks,
            { "Num blocks", "gquic.frame_type.ack.num_blocks",
              FT_UINT8, BASE_DEC, NULL, 0x0,
              "Specifying one less than the number of ack blocks", HFILL }
        },
        { &hf_gquic_frame_type_ack_first_ack_block_length,
            { "First Ack block length", "gquic.frame_type.ack.first_ack_block_length",
              FT_UINT64, BASE_DEC, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_frame_type_ack_gap_to_next_block,
            { "Gap to next block", "gquic.frame_type.ack.gap_to_next_block",
              FT_UINT8, BASE_DEC, NULL, 0x0,
              "Specifying the number of packets between ack blocks", HFILL }
        },
        { &hf_gquic_frame_type_ack_ack_block_length,
            { "Ack block length", "gquic.frame_type.ack.ack_block_length",
              FT_UINT64, BASE_DEC, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_frame_type_ack_delta_largest_acked,
            { "Delta Largest Observed", "gquic.frame_type.ack.delta_largest_acked",
              FT_UINT8, BASE_DEC, NULL, 0x0,
              "Specifying the packet number delta from the first timestamp to the largest observed", HFILL }
        },
        { &hf_gquic_frame_type_ack_time_since_largest_acked,
            { "Time Since Largest Acked", "gquic.frame_type.ack.time_since_largest_acked",
              FT_UINT32, BASE_DEC, NULL, 0x0,
              "Specifying the time delta in microseconds, from the beginning of the connection of the arrival of the packet specified by Largest Observed minus Delta Largest Observed", HFILL }
        },



        { &hf_gquic_stream_id,
            { "Stream ID", "gquic.stream_id",
              FT_UINT32, BASE_DEC, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_offset,
            { "Offset", "gquic.offset",
              FT_UINT64, BASE_DEC, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_data_len,
            { "Data Length", "gquic.data_len",
              FT_UINT32, BASE_DEC, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag,
            { "Tag", "gquic.tag",
              FT_STRING, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_number,
            { "Tag Number", "gquic.tag_number",
              FT_UINT16, BASE_DEC, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tags,
            { "Tag/value", "gquic.tags",
              FT_NONE, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_type,
            { "Tag Type", "gquic.tag_type",
              FT_STRING, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_offset_end,
            { "Tag offset end", "gquic.tag_offset_end",
              FT_UINT32, BASE_DEC, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_length,
            { "Tag length", "gquic.tag_offset_length",
              FT_UINT32, BASE_DEC, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_value,
            { "Tag/value", "gquic.tag_value",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_sni,
            { "Server Name Indication", "gquic.tag.sni",
              FT_STRING, BASE_NONE, NULL, 0x0,
              "The fully qualified DNS name of the server, canonicalised to lowercase with no trailing period", HFILL }
        },
        { &hf_gquic_tag_pad,
            { "Padding", "gquic.tag.pad",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              "Pad.....", HFILL }
        },
        { &hf_gquic_tag_ver,
            { "Version", "gquic.tag.version",
              FT_STRING, BASE_NONE, NULL, 0x0,
              "Version of gquic supported", HFILL }
        },
        { &hf_gquic_tag_pdmd,
            { "Proof demand", "gquic.tag.pdmd",
              FT_STRING, BASE_NONE, NULL, 0x0,
              "a list of tags describing the types of proof acceptable to the client, in preference order", HFILL }
        },
        { &hf_gquic_tag_ccs,
            { "Common certificate sets", "gquic.tag.ccs",
              FT_UINT64, BASE_HEX, NULL, 0x0,
              "A series of 64-bit, FNV-1a hashes of sets of common certificates that the client possesses", HFILL }
        },
        { &hf_gquic_tag_uaid,
            { "Client's User Agent ID", "gquic.tag.uaid",
              FT_STRING, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_stk,
            { "Source-address token", "gquic.tag.stk",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_sno,
            { "Server nonce", "gquic.tag.sno",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_prof,
            { "Proof (Signature)", "gquic.tag.prof",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_scfg,
            { "Server Config Tag", "gquic.tag.scfg",
              FT_STRING, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_scfg_number,
            { "Number Server Config Tag", "gquic.tag.scfg.number",
              FT_UINT32, BASE_DEC, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_rrej,
            { "Reasons for server sending", "gquic.tag.rrej",
              FT_UINT32, BASE_DEC|BASE_EXT_STRING, &handshake_failure_reason_vals_ext, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_crt,
            { "Certificate chain", "gquic.tag.crt",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_aead,
            { "Authenticated encryption algorithms", "gquic.tag.aead",
              FT_STRING, BASE_NONE, NULL, 0x0,
              "A list of tags, in preference order, specifying the AEAD primitives supported by the server", HFILL }
        },
        { &hf_gquic_tag_scid,
            { "Server Config ID", "gquic.tag.scid",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              "An opaque, 16-byte identifier for this server config", HFILL }
        },
        { &hf_gquic_tag_pubs,
            { "Public value", "gquic.tag.pubs",
              FT_UINT24, BASE_DEC_HEX, NULL, 0x0,
              "A list of public values, 24-bit, little-endian length prefixed", HFILL }
        },
        { &hf_gquic_tag_kexs,
            { "Key exchange algorithms", "gquic.tag.kexs",
              FT_STRING, BASE_NONE, NULL, 0x0,
              "A list of tags, in preference order, specifying the key exchange algorithms that the server supports", HFILL }
        },
        { &hf_gquic_tag_obit,
            { "Server orbit", "gquic.tag.obit",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_expy,
            { "Expiry", "gquic.tag.expy",
              FT_UINT64, BASE_DEC, NULL, 0x0,
              "a 64-bit expiry time for the server config in UNIX epoch seconds", HFILL }
        },
        { &hf_gquic_tag_nonc,
            { "Client nonce", "gquic.tag.nonc",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              "32 bytes consisting of 4 bytes of timestamp (big-endian, UNIX epoch seconds), 8 bytes of server orbit and 20 bytes of random data", HFILL }
        },
        { &hf_gquic_tag_mspc,
            { "Max streams per connection", "gquic.tag.mspc",
              FT_UINT32, BASE_DEC, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_tcid,
            { "Connection ID truncation", "gquic.tag.tcid",
              FT_UINT32, BASE_DEC_HEX, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_srbf,
            { "Socket receive buffer", "gquic.tag.srbf",
              FT_UINT32, BASE_DEC_HEX, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_icsl,
            { "Idle connection state", "gquic.tag.icsl",
              FT_UINT32, BASE_DEC_HEX, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_scls,
            { "Silently close on timeout", "gquic.tag.scls",
              FT_UINT32, BASE_DEC_HEX, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_copt,
            { "Connection options", "gquic.tag.copt",
              FT_STRING, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_ccrt,
            { "Cached certificates", "gquic.tag.ccrt",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_irtt,
            { "Estimated initial RTT", "gquic.tag.irtt",
              FT_UINT32, BASE_DEC, NULL, 0x0,
              "in us", HFILL }
        },
        { &hf_gquic_tag_cfcw,
            { "Initial session/connection", "gquic.tag.cfcw",
              FT_UINT32, BASE_DEC, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_sfcw,
            { "Initial stream flow control", "gquic.tag.sfcw",
              FT_UINT32, BASE_DEC, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_cetv,
            { "Client encrypted tag-value", "gquic.tag.cetv",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_xlct,
            { "Expected leaf certificate", "gquic.tag.xlct",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_nonp,
            { "Client Proof nonce", "gquic.tag.nonp",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_csct,
            { "Signed cert timestamp", "gquic.tag.csct",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_ctim,
            { "Client Timestamp", "gquic.tag.ctim",
              FT_ABSOLUTE_TIME, ABSOLUTE_TIME_UTC, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_rnon,
            { "Public reset nonce proof", "gquic.tag.rnon",
              FT_UINT64, BASE_DEC, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_rseq,
            { "Rejected Packet Number", "gquic.tag.rseq",
              FT_UINT64, BASE_DEC, NULL, 0x0,
              "a 64-bit packet number", HFILL }
        },
        { &hf_gquic_tag_cadr_addr_type,
            { "Client IP Address Type", "gquic.tag.caddr.addr.type",
              FT_UINT16, BASE_DEC, VALS(cadr_type_vals), 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_cadr_addr_ipv4,
            { "Client IP Address", "gquic.tag.caddr.addr.ipv4",
              FT_IPv4, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_cadr_addr_ipv6,
            { "Client IP Address", "gquic.tag.caddr.addr.ipv6",
              FT_IPv6, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_cadr_addr,
            { "Client IP Address", "gquic.tag.caddr.addr",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_cadr_port,
            { "Client Port (Source)", "gquic.tag.caddr.port",
              FT_UINT16, BASE_DEC, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_mids,
            { "Max incoming dynamic streams", "gquic.tag.mids",
              FT_UINT32, BASE_DEC, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_fhol,
            { "Force Head Of Line blocking", "gquic.tag.fhol",
              FT_UINT32, BASE_DEC, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_sttl,
            { "Server Config TTL", "gquic.tag.sttl",
              FT_UINT64, BASE_DEC, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_smhl,
            { "Support Max Header List (size)", "gquic.tag.smhl",
              FT_UINT64, BASE_DEC, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_tbkp,
            { "Token Binding Key Params.", "gquic.tag.tbkp",
              FT_STRING, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_tag_mad0,
            { "Max Ack Delay", "gquic.tag.mad0",
              FT_UINT32, BASE_DEC, NULL, 0x0,
              NULL, HFILL }
        },

        { &hf_gquic_tag_unknown,
            { "Unknown tag", "gquic.tag.unknown",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_padding,
            { "Padding", "gquic.padding",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_stream_data,
            { "Stream Data", "gquic.stream_data",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_gquic_payload,
            { "Payload", "gquic.payload",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              "(Google) QUIC Payload..", HFILL }
        },
    };


    static gint *ett[] = {
        &ett_gquic,
        &ett_gquic_puflags,
        &ett_gquic_prflags,
        &ett_gquic_ft,
        &ett_gquic_ftflags,
        &ett_gquic_tag_value
    };

    static ei_register_info ei[] = {
        { &ei_gquic_tag_undecoded, { "gquic.tag.undecoded", PI_UNDECODED, PI_NOTE, "Dissector for (Google)QUIC Tag code not implemented, Contact Wireshark developers if you want this supported", EXPFILL }},
        { &ei_gquic_tag_length, { "gquic.tag.length.truncated", PI_MALFORMED, PI_NOTE, "Truncated Tag Length...", EXPFILL }},
        { &ei_gquic_tag_unknown, { "gquic.tag.unknown.data", PI_UNDECODED, PI_NOTE, "Unknown Data", EXPFILL }},
        { &ei_gquic_version_invalid, { "gquic.version.invalid", PI_MALFORMED, PI_ERROR, "Invalid Version", EXPFILL }},
        { &ei_gquic_invalid_parameter, { "gquic.invalid.parameter", PI_MALFORMED, PI_ERROR, "Invalid Parameter", EXPFILL }}
    };

    expert_module_t *expert_gquic;

    proto_gquic = proto_register_protocol("GQUIC (Google Quick UDP Internet Connections)", "GQUIC", "gquic");

    proto_register_field_array(proto_gquic, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));

    gquic_module = prefs_register_protocol(proto_gquic, NULL);

    prefs_register_bool_preference(gquic_module, "debug.quic",
                       "Force decode of all (Google) QUIC Payload",
                       "Help for debug...",
                       &g_gquic_debug);

    expert_gquic = expert_register_protocol(proto_gquic);
    expert_register_field_array(expert_gquic, ei, array_length(ei));

    gquic_handle = register_dissector("gquic", dissect_gquic, proto_gquic);
}