static void adapter_stop(struct btd_adapter *adapter)
{
	/* check pending requests */
	reply_pending_requests(adapter);

	cancel_passive_scanning(adapter);

	remove_discovery_list(adapter);

	discovery_cleanup(adapter, 0);

	adapter->filtered_discovery = false;
	adapter->no_scan_restart_delay = false;
	g_free(adapter->current_discovery_filter);
	adapter->current_discovery_filter = NULL;

	adapter->discovering = false;

	while (adapter->connections) {
		struct btd_device *device = adapter->connections->data;
		uint8_t addr_type = btd_device_get_bdaddr_type(device);

		adapter_remove_connection(adapter, device, BDADDR_BREDR);
		if (addr_type != BDADDR_BREDR)
			adapter_remove_connection(adapter, device, addr_type);
	}

	g_dbus_emit_property_changed(dbus_conn, adapter->path,
					ADAPTER_INTERFACE, "Discovering");

	if (adapter->dev_class) {
		/* the kernel should reset the class of device when powering
		 * down, but it does not. So force it here ... */
		adapter->dev_class = 0;
		g_dbus_emit_property_changed(dbus_conn, adapter->path,
						ADAPTER_INTERFACE, "Class");
	}

	g_dbus_emit_property_changed(dbus_conn, adapter->path,
						ADAPTER_INTERFACE, "Powered");

	DBG("adapter %s has been disabled", adapter->path);
}