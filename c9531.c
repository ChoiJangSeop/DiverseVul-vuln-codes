dissect_blip(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, _U_ void *data)
{

	proto_tree *blip_tree;
	gint offset = 0;

	/* Set the protcol column to say BLIP */
	col_set_str(pinfo->cinfo, COL_PROTOCOL, "BLIP");
	/* Clear out stuff in the info column */
	col_clear(pinfo->cinfo,COL_INFO);

	// ------------------------------------- Setup BLIP tree -----------------------------------------------------------


	/* Add a subtree to dissection.  See WSDG 9.2.2. Dissecting the details of the protocol  */
	proto_item *blip_item = proto_tree_add_item(tree, proto_blip, tvb, offset, -1, ENC_NA);

	blip_tree = proto_item_add_subtree(blip_item, ett_blip);


	// ------------------------ BLIP Frame Header: Message Number VarInt -----------------------------------------------

	// This gets the message number as a var int in order to find out how much to bump
	// the offset for the next proto_tree item
	guint64 value_message_num;
	guint varint_message_num_length = tvb_get_varint(
			tvb,
			offset,
			FT_VARINT_MAX_LEN,
			&value_message_num,
			ENC_VARINT_PROTOBUF);

	proto_tree_add_item(blip_tree, hf_blip_message_number, tvb, offset, varint_message_num_length, ENC_VARINT_PROTOBUF);

	offset += varint_message_num_length;

	// ------------------------ BLIP Frame Header: Frame Flags VarInt --------------------------------------------------

	// This gets the message number as a var int in order to find out how much to bump
	// the offset for the next proto_tree item
	guint64 value_frame_flags;
	guint varint_frame_flags_length = tvb_get_varint(
			tvb,
			offset,
			FT_VARINT_MAX_LEN,
			&value_frame_flags,
			ENC_VARINT_PROTOBUF);

	guint64 masked = value_frame_flags & ~0x07;
	proto_tree_add_uint(blip_tree, hf_blip_frame_flags, tvb, offset, varint_frame_flags_length, (guint8)masked);

	offset += varint_frame_flags_length;

	const gchar* msg_type = get_message_type(value_frame_flags);
	gchar* msg_num = wmem_strdup_printf(wmem_packet_scope(), "#%" G_GUINT64_FORMAT, value_message_num);
	gchar* col_info = wmem_strconcat(wmem_packet_scope(), msg_type, msg_num, NULL);
	col_add_str(pinfo->cinfo, COL_INFO, col_info);

	// If it's an ACK message, handle that separately, since there are no properties etc.
	if (is_ack_message(value_frame_flags) == TRUE) {
		return handle_ack_message(tvb, pinfo, blip_tree, offset, value_frame_flags);
	}


	// ------------------------------------- Conversation Tracking -----------------------------------------------------

	blip_conversation_entry_t *conversation_entry_ptr = get_blip_conversation(pinfo);

	// Is this the first frame in a blip message with multiple frames?
	gboolean first_frame_in_msg = is_first_frame_in_msg(
			conversation_entry_ptr,
			pinfo,
			value_frame_flags,
			value_message_num
	);

	tvbuff_t* tvb_to_use = tvb;
	gboolean compressed = is_compressed(value_frame_flags);
	if(compressed) {
#ifdef HAVE_ZLIB
		tvb_to_use = decompress(pinfo, tvb, offset, tvb_reported_length_remaining(tvb, offset) - BLIP_BODY_CHECKSUM_SIZE);
		if(!tvb_to_use) {
			proto_tree_add_string(blip_tree, hf_blip_message_body, tvb, offset, tvb_reported_length_remaining(tvb, offset), "<Error decompressing message>");
			return tvb_reported_length(tvb);
		}
#else /* ! HAVE_ZLIB */
		proto_tree_add_string(blip_tree, hf_blip_message_body, tvb, offset, tvb_reported_length_remaining(tvb, offset), "<decompression support is not available>");
		return tvb_reported_length(tvb);
#endif /* ! HAVE_ZLIB */

		offset = 0;
	}

	// Is this the first frame in a message?
	if (first_frame_in_msg == TRUE) {

		// ------------------------ BLIP Frame Header: Properties Length VarInt --------------------------------------------------

		// WARNING: this only works because this code assumes that ALL MESSAGES FIT INTO ONE FRAME, which is absolutely not true.
		// In other words, as soon as there is a message that spans two frames, this code will break.

		guint64 value_properties_length;
		guint value_properties_length_varint_length = tvb_get_varint(
				tvb_to_use,
				offset,
				FT_VARINT_MAX_LEN,
				&value_properties_length,
				ENC_VARINT_PROTOBUF);

		proto_tree_add_item(blip_tree, hf_blip_properties_length, tvb_to_use, offset, value_properties_length_varint_length, ENC_VARINT_PROTOBUF);

		offset += value_properties_length_varint_length;

		// ------------------------ BLIP Frame: Properties --------------------------------------------------

		// WARNING: this only works because this code assumes that ALL MESSAGES FIT INTO ONE FRAME, which is absolutely not true.
		// In other words, as soon as there is a message that spans two frames, this code will break.

		// At this point, the length of the properties is known and is stored in value_properties_length.
		// This reads the entire properties out of the tvb and into a buffer (buf).
		guint8* buf = tvb_get_string_enc(wmem_packet_scope(), tvb_to_use, offset, (gint) value_properties_length, ENC_UTF_8);

		// "Profile\0subChanges\0continuous\0true\0foo\0bar" -> "Profile:subChanges:continuous:true:foo:bar"
		// Iterate over buf and change all the \0 null characters to ':', since otherwise trying to set a header
		// field to this buffer via proto_tree_add_item() will end up only printing it up to the first null character,
		// for example "Profile", even though there are many more properties that follow.
		for (int i = 0; i < (int) value_properties_length; i++) {
			if (i < (int) (value_properties_length - 1)) {
				if (buf[i] == '\0') {  // TODO: I don't even know if this is actually a safe assumption in a UTF-8 encoded string
					buf[i] = ':';
				}
			}
		}

		if(value_properties_length > 0) {
			proto_tree_add_string(blip_tree, hf_blip_properties, tvb_to_use, offset, (int)value_properties_length, (const char *)buf);
		}

		// Bump the offset by the length of the properties
		offset += (gint)value_properties_length;
	}

	// ------------------------ BLIP Frame: Message Body --------------------------------------------------

	// WS_DLL_PUBLIC gint tvb_reported_length_remaining(const tvbuff_t *tvb, const gint offset);
	gint reported_length_remaining = tvb_reported_length_remaining(tvb_to_use, offset);

	// Don't read in the trailing checksum at the end
	if (!compressed && reported_length_remaining >= BLIP_BODY_CHECKSUM_SIZE) {
		reported_length_remaining -= BLIP_BODY_CHECKSUM_SIZE;
	}

	if(reported_length_remaining > 0) {
		proto_tree_add_item(blip_tree, hf_blip_message_body, tvb_to_use, offset, reported_length_remaining, ENC_UTF_8|ENC_NA);
	}

	proto_tree_add_item(blip_tree, hf_blip_checksum, tvb, tvb_reported_length(tvb) - BLIP_BODY_CHECKSUM_SIZE, BLIP_BODY_CHECKSUM_SIZE, ENC_BIG_ENDIAN);

	// -------------------------------------------- Etc ----------------------------------------------------------------

	return tvb_captured_length(tvb);
}