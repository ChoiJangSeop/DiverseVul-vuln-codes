static void settings_changed(struct btd_adapter *adapter, uint32_t settings)
{
	uint32_t changed_mask;

	changed_mask = adapter->current_settings ^ settings;

	adapter->current_settings = settings;
	adapter->pending_settings &= ~changed_mask;

	DBG("Changed settings: 0x%08x", changed_mask);
	DBG("Pending settings: 0x%08x", adapter->pending_settings);

	if (changed_mask & MGMT_SETTING_POWERED) {
	        g_dbus_emit_property_changed(dbus_conn, adapter->path,
					ADAPTER_INTERFACE, "Powered");

		if (adapter->current_settings & MGMT_SETTING_POWERED) {
			adapter_start(adapter);
		} else {
			adapter_stop(adapter);

			if (powering_down) {
				adapter_remaining--;

				if (!adapter_remaining)
					btd_exit();
			}
		}
	}

	if ((changed_mask & MGMT_SETTING_LE) &&
				btd_adapter_get_powered(adapter) &&
				(adapter->current_settings & MGMT_SETTING_LE))
		trigger_passive_scanning(adapter);

	if (changed_mask & MGMT_SETTING_DISCOVERABLE) {
		g_dbus_emit_property_changed(dbus_conn, adapter->path,
					ADAPTER_INTERFACE, "Discoverable");
		store_adapter_info(adapter);
		btd_adv_manager_refresh(adapter->adv_manager);
	}

	if (changed_mask & MGMT_SETTING_BONDABLE) {
		g_dbus_emit_property_changed(dbus_conn, adapter->path,
					ADAPTER_INTERFACE, "Pairable");

		trigger_pairable_timeout(adapter);
	}
}