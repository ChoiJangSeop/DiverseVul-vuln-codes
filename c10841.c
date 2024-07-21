static void write_reply_cb(DBusMessage *message, void *user_data)
{
	struct pending_op *op = user_data;
	struct external_chrc *chrc;
	struct external_desc *desc;
	DBusError err;
	DBusMessageIter iter;
	uint8_t ecode = 0;

	if (!op->owner_queue) {
		DBG("Pending write was canceled when object got removed");
		return;
	}

	dbus_error_init(&err);

	if (dbus_set_error_from_message(&err, message) == TRUE) {
		DBG("Failed to write value: %s: %s", err.name, err.message);
		ecode = dbus_error_to_att_ecode(err.name,
					BT_ATT_ERROR_WRITE_NOT_PERMITTED);
		dbus_error_free(&err);
		goto done;
	}

	if (op->prep_authorize) {
		if (op->is_characteristic) {
			chrc = gatt_db_attribute_get_user_data(op->attrib);
			chrc->prep_authorized = true;
		} else {
			desc = gatt_db_attribute_get_user_data(op->attrib);
			desc->prep_authorized = true;
		}
	}

	dbus_message_iter_init(message, &iter);
	if (dbus_message_iter_has_next(&iter)) {
		/*
		 * Return not supported for this, as the external app basically
		 * doesn't properly support the "WriteValue" API.
		 */
		ecode = BT_ATT_ERROR_REQUEST_NOT_SUPPORTED;
		error("Invalid return value received for \"WriteValue\"");
	}

done:
	gatt_db_attribute_write_result(op->attrib, op->id, ecode);
}