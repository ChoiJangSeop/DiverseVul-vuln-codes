static void handle_read_req(struct bt_att_chan *chan,
				struct bt_gatt_server *server, uint8_t opcode,
				uint16_t handle, uint16_t offset)
{
	struct gatt_db_attribute *attr;
	uint8_t ecode;
	struct async_read_op *op = NULL;

	ecode = authorize_req(server, opcode, handle);
	if (ecode)
		goto error;

	attr = gatt_db_get_attribute(server->db, handle);
	if (!attr) {
		ecode = BT_ATT_ERROR_INVALID_HANDLE;
		goto error;
	}

	util_debug(server->debug_callback, server->debug_data,
			"Read %sReq - handle: 0x%04x",
			opcode == BT_ATT_OP_READ_BLOB_REQ ? "Blob " : "",
			handle);

	ecode = check_permissions(server, attr, BT_ATT_PERM_READ |
						BT_ATT_PERM_READ_AUTHEN |
						BT_ATT_PERM_READ_ENCRYPT);
	if (ecode)
		goto error;

	op = new0(struct async_read_op, 1);
	op->chan = chan;
	op->opcode = opcode;
	op->server = bt_gatt_server_ref(server);

	if (gatt_db_attribute_read(attr, offset, opcode, server->att,
							read_complete_cb, op))
		return;

	ecode = BT_ATT_ERROR_UNLIKELY;

error:
	if (op)
		async_read_op_destroy(op);

	bt_att_chan_send_error_rsp(chan, opcode, handle, ecode);
}