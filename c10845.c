static void desc_read_cb(struct gatt_db_attribute *attrib,
					unsigned int id, uint16_t offset,
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

	if (send_read(device, attrib, desc->proxy, desc->pending_reads, id,
					offset, bt_att_get_link_type(att)))
		return;

fail:
	gatt_db_attribute_read_result(attrib, id, BT_ATT_ERROR_UNLIKELY,
								NULL, 0);
}