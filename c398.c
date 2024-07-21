nautilus_application_startup (NautilusApplication *application,
			      gboolean kill_shell,
			      gboolean no_default_window,
			      gboolean no_desktop,
			      gboolean browser_window,
			      const char *geometry,
			      char **urls)
{
	UniqueMessageData *message;
	
	/* Check the user's ~/.nautilus directories and post warnings
	 * if there are problems.
	 */
	if (!kill_shell && !check_required_directories (application)) {
		return;
	}

	if (kill_shell) {
		if (unique_app_is_running (application->unique_app)) {
			unique_app_send_message (application->unique_app,
						 UNIQUE_CLOSE, NULL);
			
		}
	} else {
		/* If KDE desktop is running, then force no_desktop */
		if (is_kdesktop_present ()) {
			no_desktop = TRUE;
		}
		
		if (!no_desktop && eel_preferences_get_boolean (NAUTILUS_PREFERENCES_SHOW_DESKTOP)) {
			if (unique_app_is_running (application->unique_app)) {
				unique_app_send_message (application->unique_app,
							 COMMAND_START_DESKTOP, NULL);
			} else {
				nautilus_application_open_desktop (application);
			}
		}

		if (!unique_app_is_running (application->unique_app)) {
			finish_startup (application);
			g_signal_connect (application->unique_app, "message-received", G_CALLBACK (message_received_cb), application);			
		}
		
		/* Monitor the preference to show or hide the desktop */
		eel_preferences_add_callback_while_alive (NAUTILUS_PREFERENCES_SHOW_DESKTOP,
							  desktop_changed_callback,
							  application,
							  G_OBJECT (application));

		/* Monitor the preference to have the desktop */
		/* point to the Unix home folder */
		eel_preferences_add_callback_while_alive (NAUTILUS_PREFERENCES_DESKTOP_IS_HOME_DIR,
							  desktop_location_changed_callback,
							  NULL,
							  G_OBJECT (application));

	  	/* Create the other windows. */
		if (urls != NULL || !no_default_window) {
			if (unique_app_is_running (application->unique_app)) {
				message = unique_message_data_new ();
				_unique_message_data_set_geometry_and_uris (message, geometry, urls);
				if (browser_window) {
					unique_app_send_message (application->unique_app,
								 COMMAND_OPEN_BROWSER, message);
				} else {
					unique_app_send_message (application->unique_app,
								 UNIQUE_OPEN, message);
				}
				unique_message_data_free (message);				
			} else {
				open_windows (application, NULL,
					      urls,
					      geometry,
					      browser_window);
			}
		}

		/* Load session info if availible */
		nautilus_application_load_session (application);
	}
}