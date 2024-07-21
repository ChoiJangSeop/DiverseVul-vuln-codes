static int input_init(void)
{
	GKeyFile *config;
	GError *err = NULL;

	config = load_config_file(CONFIGDIR "/input.conf");
	if (config) {
		int idle_timeout;
		gboolean uhid_enabled;

		idle_timeout = g_key_file_get_integer(config, "General",
							"IdleTimeout", &err);
		if (!err) {
			DBG("input.conf: IdleTimeout=%d", idle_timeout);
			input_set_idle_timeout(idle_timeout * 60);
		} else
			g_clear_error(&err);

		uhid_enabled = g_key_file_get_boolean(config, "General",
							"UserspaceHID", &err);
		if (!err) {
			DBG("input.conf: UserspaceHID=%s", uhid_enabled ?
							"true" : "false");
			input_enable_userspace_hid(uhid_enabled);
		} else
			g_clear_error(&err);
	}

	btd_profile_register(&input_profile);

	if (config)
		g_key_file_free(config);

	return 0;
}