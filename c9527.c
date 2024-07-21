proto_register_fb_zero(void)
{
    static hf_register_info hf[] = {
        { &hf_fb_zero_puflags,
            { "Public Flags", "fb_zero.puflags",
              FT_UINT8, BASE_HEX, NULL, 0x0,
              "Specifying per-packet public flags", HFILL }
        },
        { &hf_fb_zero_puflags_vrsn,
            { "Version", "fb_zero.puflags.version",
              FT_BOOLEAN, 8, TFS(&tfs_yes_no), PUFLAGS_VRSN,
              "Signifies that this packet also contains the version of the FB Zero protocol", HFILL }
        },
        { &hf_fb_zero_puflags_unknown,
            { "Unknown", "fb_zero.puflags.unknown",
              FT_UINT8, BASE_HEX, NULL, PUFLAGS_UNKN,
              NULL, HFILL }
        },

        { &hf_fb_zero_version,
            { "Version", "fb_zero.version",
              FT_STRING, BASE_NONE, NULL, 0x0,
              "32 bit opaque tag that represents the version of the ZB Zero (Always QTV)", HFILL }
        },
        { &hf_fb_zero_length,
            { "Length", "fb_zero.length",
              FT_UINT32, BASE_DEC, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_fb_zero_tag,
            { "Tag", "fb_zero.tag",
              FT_STRING, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_fb_zero_tag_number,
            { "Tag Number", "fb_zero.tag_number",
              FT_UINT16, BASE_DEC, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_fb_zero_tags,
            { "Tag/value", "fb_zero.tags",
              FT_NONE, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_fb_zero_tag_type,
            { "Tag Type", "fb_zero.tag_type",
              FT_STRING, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_fb_zero_tag_offset_end,
            { "Tag offset end", "fb_zero.tag_offset_end",
              FT_UINT32, BASE_DEC, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_fb_zero_tag_length,
            { "Tag length", "fb_zero.tag_offset_length",
              FT_UINT32, BASE_DEC, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_fb_zero_tag_value,
            { "Tag/value", "fb_zero.tag_value",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_fb_zero_tag_sni,
            { "Server Name Indication", "fb_zero.tag.sni",
              FT_STRING, BASE_NONE, NULL, 0x0,
              "The fully qualified DNS name of the server, canonicalised to lowercase with no trailing period", HFILL }
        },
        { &hf_fb_zero_tag_vers,
            { "Version", "fb_zero.tag.version",
              FT_STRING, BASE_NONE, NULL, 0x0,
              "Version of FB Zero supported", HFILL }
        },
        { &hf_fb_zero_tag_sno,
            { "Server nonce", "fb_zero.tag.sno",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_fb_zero_tag_aead,
            { "Authenticated encryption algorithms", "fb_zero.tag.aead",
              FT_STRING, BASE_NONE, NULL, 0x0,
              "A list of tags, in preference order, specifying the AEAD primitives supported by the server", HFILL }
        },
        { &hf_fb_zero_tag_scid,
            { "Server Config ID", "fb_zero.tag.scid",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              "An opaque, 16-byte identifier for this server config", HFILL }
        },
        { &hf_fb_zero_tag_time,
            { "Time", "fb_zero.tag.time",
              FT_UINT32, BASE_DEC, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_fb_zero_tag_alpn,
            { "ALPN", "fb_zero.tag.alpn",
              FT_STRING, BASE_NONE, NULL, 0x0,
              "Application-Layer Protocol Negotiation supported", HFILL }
        },
        { &hf_fb_zero_tag_pubs,
            { "Public value", "fb_zero.tag.pubs",
              FT_UINT24, BASE_DEC_HEX, NULL, 0x0,
              "A list of public values, 24-bit, little-endian length prefixed", HFILL }
        },
        { &hf_fb_zero_tag_kexs,
            { "Key exchange algorithms", "fb_zero.tag.kexs",
              FT_STRING, BASE_NONE, NULL, 0x0,
              "A list of tags, in preference order, specifying the key exchange algorithms that the server supports", HFILL }
        },
        { &hf_fb_zero_tag_nonc,
            { "Client nonce", "fb_zero.tag.nonc",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              "32 bytes consisting of 4 bytes of timestamp (big-endian, UNIX epoch seconds), 8 bytes of server orbit and 20 bytes of random data", HFILL }
        },
        { &hf_fb_zero_tag_unknown,
            { "Unknown tag", "fb_zero.tag.unknown",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_fb_zero_padding,
            { "Padding", "fb_zero.padding",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              NULL, HFILL }
        },
        { &hf_fb_zero_payload,
            { "Payload", "fb_zero.payload",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              "Fb Zero Payload..", HFILL }
        },
        { &hf_fb_zero_unknown,
            { "Unknown", "fb_zero.unknown",
              FT_BYTES, BASE_NONE, NULL, 0x0,
              "Unknown Data", HFILL }
        },
    };


    static gint *ett[] = {
        &ett_fb_zero,
        &ett_fb_zero_puflags,
        &ett_fb_zero_prflags,
        &ett_fb_zero_ft,
        &ett_fb_zero_ftflags,
        &ett_fb_zero_tag_value
    };

    static ei_register_info ei[] = {
        { &ei_fb_zero_tag_undecoded, { "fb_zero.tag.undecoded", PI_UNDECODED, PI_NOTE, "Dissector for FB Zero Tag code not implemented, Contact Wireshark developers if you want this supported", EXPFILL }},
        { &ei_fb_zero_tag_length, { "fb_zero.tag.length.truncated", PI_MALFORMED, PI_NOTE, "Truncated Tag Length...", EXPFILL }},
        { &ei_fb_zero_tag_unknown, { "fb_zero.tag.unknown.data", PI_UNDECODED, PI_NOTE, "Unknown Data", EXPFILL }},
    };

    expert_module_t *expert_fb_zero;

    proto_fb_zero = proto_register_protocol("(Facebook) Zero Protocol", "FBZERO", "fb_zero");

    fb_zero_handle = register_dissector("fb_zero", dissect_fb_zero, proto_fb_zero);

    proto_register_field_array(proto_fb_zero, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));

    expert_fb_zero = expert_register_protocol(proto_fb_zero);
    expert_register_field_array(expert_fb_zero, ei, array_length(ei));
}