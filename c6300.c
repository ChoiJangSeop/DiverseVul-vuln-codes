static void stub_device_rebind(void)
{
#if IS_MODULE(CONFIG_USBIP_HOST)
	struct bus_id_priv *busid_priv;
	int i;

	/* update status to STUB_BUSID_OTHER so probe ignores the device */
	spin_lock(&busid_table_lock);
	for (i = 0; i < MAX_BUSID; i++) {
		if (busid_table[i].name[0] &&
		    busid_table[i].shutdown_busid) {
			busid_priv = &(busid_table[i]);
			busid_priv->status = STUB_BUSID_OTHER;
		}
	}
	spin_unlock(&busid_table_lock);

	/* now run rebind */
	for (i = 0; i < MAX_BUSID; i++) {
		if (busid_table[i].name[0] &&
		    busid_table[i].shutdown_busid) {
			busid_priv = &(busid_table[i]);
			do_rebind(busid_table[i].name, busid_priv);
		}
	}
#endif
}