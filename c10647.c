static int discovery_stop(struct discovery_client *client)
{
	struct btd_adapter *adapter = client->adapter;
	struct mgmt_cp_stop_discovery cp;

	/* Check if there are more client discovering */
	if (g_slist_next(adapter->discovery_list)) {
		discovery_remove(client);
		update_discovery_filter(adapter);
		return 0;
	}

	if (adapter->discovery_discoverable)
		set_discovery_discoverable(adapter, false);

	/*
	 * In the idle phase of a discovery, there is no need to stop it
	 * and so it is enough to send out the signal and just return.
	 */
	if (adapter->discovery_enable == 0x00) {
		discovery_remove(client);
		adapter->discovering = false;
		g_dbus_emit_property_changed(dbus_conn, adapter->path,
					ADAPTER_INTERFACE, "Discovering");

		trigger_passive_scanning(adapter);

		return 0;
	}

	cp.type = adapter->discovery_type;
	adapter->client = client;

	mgmt_send(adapter->mgmt, MGMT_OP_STOP_DISCOVERY,
			adapter->dev_id, sizeof(cp), &cp,
			stop_discovery_complete, adapter, NULL);

	return -EINPROGRESS;
}