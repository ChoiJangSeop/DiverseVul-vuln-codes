proto_register_blip(void)
{
	static hf_register_info hf[] = {
	{ &hf_blip_message_number,
		{ "Message Number", "blip.messagenum", FT_UINT64, BASE_DEC,
			NULL, 0x0, NULL, HFILL }
	},
	{ &hf_blip_frame_flags,
		{ "Frame Flags", "blip.frameflags", FT_UINT8, BASE_HEX | BASE_EXT_STRING,
			&flag_combos_ext, 0x0, NULL, HFILL }
	},
	{ &hf_blip_properties_length,
		{ "Properties Length", "blip.propslength", FT_UINT64, BASE_DEC,
			NULL, 0x0, NULL, HFILL }
	},
	{ &hf_blip_properties,
		{ "Properties", "blip.props", FT_STRING, STR_UNICODE,
			NULL, 0x0, NULL, HFILL }
		},
	{ &hf_blip_message_body,
		{ "Message Body", "blip.messagebody", FT_STRING, STR_UNICODE,
			NULL, 0x0, NULL, HFILL }
	},
	{ &hf_blip_ack_size,
		{ "ACK num bytes", "blip.numackbytes", FT_UINT64, BASE_DEC,
			NULL, 0x0, NULL, HFILL }
	},
	{ &hf_blip_checksum,
		{ "Checksum", "blip.checksum", FT_UINT32, BASE_DEC,
			NULL, 0x0, NULL, HFILL }
	}
	};

	/* Setup protocol subtree array */
	static gint *ett[] = {
		&ett_blip
	};

	proto_blip = proto_register_protocol("BLIP Couchbase Mobile", "BLIP", "blip");

	proto_register_field_array(proto_blip, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));

	blip_handle = register_dissector("blip", dissect_blip, proto_blip);
}