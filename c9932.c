static void process_read_by_type(struct async_read_op *op)
{
	struct bt_gatt_server *server = op->server;
	uint8_t ecode;
	struct gatt_db_attribute *attr;

	attr = queue_pop_head(op->db_data);

	if (op->done || !attr) {
		bt_att_chan_send_rsp(op->chan, BT_ATT_OP_READ_BY_TYPE_RSP,
						op->pdu, op->pdu_len);
		async_read_op_destroy(op);
		return;
	}

	ecode = check_permissions(server, attr, BT_ATT_PERM_READ |
						BT_ATT_PERM_READ_AUTHEN |
						BT_ATT_PERM_READ_ENCRYPT);
	if (ecode)
		goto error;

	if (gatt_db_attribute_read(attr, 0, op->opcode, server->att,
					read_by_type_read_complete_cb, op))
		return;

	ecode = BT_ATT_ERROR_UNLIKELY;

error:
	bt_att_chan_send_error_rsp(op->chan, BT_ATT_OP_READ_BY_TYPE_REQ,
				gatt_db_attribute_get_handle(attr), ecode);
	async_read_op_destroy(op);
}