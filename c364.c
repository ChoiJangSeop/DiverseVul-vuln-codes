nma_gconf_settings_new (void)
{
	return (NMAGConfSettings *) g_object_new (NMA_TYPE_GCONF_SETTINGS, NULL);
}