finish_startup (NautilusApplication *application)
{
	GList *drives;

	/* initialize nautilus modules */
	nautilus_module_setup ();

	/* attach menu-provider module callback */
	menu_provider_init_callback ();
	
	/* Initialize the desktop link monitor singleton */
	nautilus_desktop_link_monitor_get ();

	/* Watch for mounts so we can restore open windows This used
	 * to be for showing new window on mount, but is not used
	 * anymore */

	/* Watch for unmounts so we can close open windows */
	/* TODO-gio: This should be using the UNMOUNTED feature of GFileMonitor instead */
	application->volume_monitor = g_volume_monitor_get ();
	g_signal_connect_object (application->volume_monitor, "mount_removed",
				 G_CALLBACK (mount_removed_callback), application, 0);
	g_signal_connect_object (application->volume_monitor, "mount_pre_unmount",
				 G_CALLBACK (mount_removed_callback), application, 0);
	g_signal_connect_object (application->volume_monitor, "mount_added",
				 G_CALLBACK (mount_added_callback), application, 0);
	g_signal_connect_object (application->volume_monitor, "volume_added",
				 G_CALLBACK (volume_added_callback), application, 0);
	g_signal_connect_object (application->volume_monitor, "drive_connected",
				 G_CALLBACK (drive_connected_callback), application, 0);

	/* listen for eject button presses */
	drives = g_volume_monitor_get_connected_drives (application->volume_monitor);
	g_list_foreach (drives, (GFunc) drive_listen_for_eject_button, application);
	g_list_foreach (drives, (GFunc) g_object_unref, NULL);
	g_list_free (drives);

	application->automount_idle_id = 
		g_idle_add_full (G_PRIORITY_LOW,
				 automount_all_volumes_idle_cb,
				 application, NULL);
}