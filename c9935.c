static void read_multiple_complete_cb(struct gatt_db_attribute *attr, int err,
					const uint8_t *value, size_t len,
					void *user_data)
{
	struct read_mult_data *data = user_data;
	struct gatt_db_attribute *next_attr;
	uint16_t handle = gatt_db_attribute_get_handle(attr);
	uint8_t ecode;
	uint16_t length;

	if (err != 0) {
		ecode = err;
		goto error;
	}

	length = data->opcode == BT_ATT_OP_READ_MULT_VL_REQ ?
			MIN(len, MAX(data->mtu - data->length, 3) - 3) :
			MIN(len, data->mtu - data->length - 1);

	if (data->opcode == BT_ATT_OP_READ_MULT_VL_REQ) {
		/* The Length Value Tuple List may be truncated within the first
		 * two octets of a tuple due to the size limits of the current
		 * ATT_MTU, but the first two octets cannot be separated.
		 */
		if (data->mtu - data->length >= 3) {
			put_le16(len, data->rsp_data + data->length);
			data->length += 2;
		}
	}

	memcpy(data->rsp_data + data->length, value, length);
	data->length += length;

	data->cur_handle++;

	if (data->cur_handle == data->num_handles) {
		bt_att_chan_send_rsp(data->chan, data->opcode + 1,
						data->rsp_data, data->length);
		read_mult_data_free(data);
		return;
	}

	handle = data->handles[data->cur_handle];

	util_debug(data->server->debug_callback, data->server->debug_data,
				"%s Req - #%zu of %zu: 0x%04x",
				data->opcode == BT_ATT_OP_READ_MULT_REQ ?
				"Read Multiple" :
				"Read Multiple Variable Length",
				data->cur_handle + 1, data->num_handles,
				handle);

	next_attr = gatt_db_get_attribute(data->server->db, handle);

	if (!next_attr) {
		ecode = BT_ATT_ERROR_INVALID_HANDLE;
		goto error;
	}

	ecode = check_permissions(data->server, next_attr, BT_ATT_PERM_READ |
						BT_ATT_PERM_READ_AUTHEN |
						BT_ATT_PERM_READ_ENCRYPT);
	if (ecode)
		goto error;

	if (gatt_db_attribute_read(next_attr, 0, data->opcode,
					data->server->att,
					read_multiple_complete_cb, data))
		return;

	ecode = BT_ATT_ERROR_UNLIKELY;

error:
	bt_att_chan_send_error_rsp(data->chan, data->opcode, handle, ecode);
	read_mult_data_free(data);
}