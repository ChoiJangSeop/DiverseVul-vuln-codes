static void append_options(DBusMessageIter *iter, void *user_data)
{
	struct pending_op *op = user_data;
	const char *path = device_get_path(op->device);
	struct bt_gatt_server *server;
	const char *link;
	uint16_t mtu;

	switch (op->link_type) {
	case BT_ATT_BREDR:
		link = "BR/EDR";
		break;
	case BT_ATT_LE:
		link = "LE";
		break;
	default:
		link = NULL;
		break;
	}

	dict_append_entry(iter, "device", DBUS_TYPE_OBJECT_PATH, &path);
	if (op->offset)
		dict_append_entry(iter, "offset", DBUS_TYPE_UINT16,
							&op->offset);
	if (link)
		dict_append_entry(iter, "link", DBUS_TYPE_STRING, &link);
	if (op->prep_authorize)
		dict_append_entry(iter, "prepare-authorize", DBUS_TYPE_BOOLEAN,
							&op->prep_authorize);

	server = btd_device_get_gatt_server(op->device);
	mtu = bt_gatt_server_get_mtu(server);

	dict_append_entry(iter, "mtu", DBUS_TYPE_UINT16, &mtu);
}