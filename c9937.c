static void read_multiple_cb(struct bt_att_chan *chan, uint8_t opcode,
					const void *pdu, uint16_t length,
					void *user_data)
{
	struct bt_gatt_server *server = user_data;
	struct gatt_db_attribute *attr;
	struct read_mult_data *data = NULL;
	uint8_t ecode;
	size_t i = 0;
	uint16_t handle = 0;

	if (length < 4) {
		ecode = BT_ATT_ERROR_INVALID_PDU;
		goto error;
	}

	data = read_mult_data_new(server, chan, opcode, length / 2);
	if (!data)
		goto error;

	for (i = 0; i < data->num_handles; i++)
		data->handles[i] = get_le16(pdu + i * 2);

	handle = data->handles[0];

	util_debug(server->debug_callback, server->debug_data,
			"%s Req - %zu handles, 1st: 0x%04x",
			data->opcode == BT_ATT_OP_READ_MULT_REQ ?
			"Read Multiple" : "Read Multiple Variable Length",
			data->num_handles, handle);

	attr = gatt_db_get_attribute(server->db, handle);

	if (!attr) {
		ecode = BT_ATT_ERROR_INVALID_HANDLE;
		goto error;
	}

	ecode = check_permissions(data->server, attr, BT_ATT_PERM_READ |
						BT_ATT_PERM_READ_AUTHEN |
						BT_ATT_PERM_READ_ENCRYPT);
	if (ecode)
		goto error;

	if (gatt_db_attribute_read(attr, 0, opcode, server->att,
					read_multiple_complete_cb, data))
		return;

	ecode = BT_ATT_ERROR_UNLIKELY;

error:
	if (data)
		read_mult_data_free(data);

	bt_att_chan_send_error_rsp(chan, opcode, handle, ecode);
}