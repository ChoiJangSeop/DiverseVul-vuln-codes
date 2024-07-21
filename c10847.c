static void desc_write_cb(struct gatt_db_attribute *attrib,
					unsigned int id, uint16_t offset,
					const uint8_t *value, size_t len,
					uint8_t opcode, struct bt_att *att,
					void *user_data)
{
	struct external_desc *desc = user_data;
	struct btd_device *device;

	if (desc->attrib != attrib) {
		error("Read callback called with incorrect attribute");
		goto fail;
	}

	device = att_get_device(att);
	if (!device) {
		error("Unable to find device object");
		goto fail;
	}

	if (opcode == BT_ATT_OP_PREP_WRITE_REQ) {
		if (!device_is_trusted(device) && !desc->prep_authorized &&
						desc->req_prep_authorization)
			send_write(device, attrib, desc->proxy,
					desc->pending_writes, id, value, len,
					offset, bt_att_get_link_type(att),
					false, true);
		else
			gatt_db_attribute_write_result(attrib, id, 0);

		return;
	}

	if (opcode == BT_ATT_OP_EXEC_WRITE_REQ)
		desc->prep_authorized = false;

	if (send_write(device, attrib, desc->proxy, desc->pending_writes, id,
			value, len, offset, bt_att_get_link_type(att), false,
			false))
		return;

fail:
	gatt_db_attribute_write_result(attrib, id, BT_ATT_ERROR_UNLIKELY);
}