activate_files (ActivateParameters *parameters)
{
	NautilusWindowInfo *window_info;
	NautilusWindowOpenFlags flags;
	NautilusFile *file;
	GList *launch_desktop_files;
	GList *launch_files;
	GList *launch_in_terminal_files;
	GList *open_in_app_files;
	GList *open_in_app_parameters;
	GList *unhandled_open_in_app_files;
	ApplicationLaunchParameters *one_parameters;
	GList *open_in_view_files;
	GList *l;
	int count;
	char *uri;
	char *executable_path, *quoted_path, *name;
	char *old_working_dir;
	ActivationAction action;
	GdkScreen *screen;
	
	screen = gtk_widget_get_screen (GTK_WIDGET (parameters->parent_window));

	launch_desktop_files = NULL;
	launch_files = NULL;
	launch_in_terminal_files = NULL;
	open_in_app_files = NULL;
	open_in_view_files = NULL;

	for (l = parameters->files; l != NULL; l = l->next) {
		file = NAUTILUS_FILE (l->data);

		if (file_was_cancelled (file)) {
			continue;
		}

		action = get_activation_action (file);
		if (action == ACTIVATION_ACTION_ASK) {
			/* Special case for executable text files, since it might be
			 * dangerous & unexpected to launch these.
			 */
			pause_activation_timed_cancel (parameters);
			action = get_executable_text_file_action (parameters->parent_window, file);
			unpause_activation_timed_cancel (parameters);
		}

		switch (action) {
		case ACTIVATION_ACTION_LAUNCH_DESKTOP_FILE :
			launch_desktop_files = g_list_prepend (launch_desktop_files, file);
			break;
		case ACTIVATION_ACTION_LAUNCH :
			launch_files = g_list_prepend (launch_files, file);
			break;
		case ACTIVATION_ACTION_LAUNCH_IN_TERMINAL :
			launch_in_terminal_files = g_list_prepend (launch_in_terminal_files, file);
			break;
		case ACTIVATION_ACTION_OPEN_IN_VIEW :
			open_in_view_files = g_list_prepend (open_in_view_files, file);
			break;
		case ACTIVATION_ACTION_OPEN_IN_APPLICATION :
			open_in_app_files = g_list_prepend (open_in_app_files, file);
			break;
		case ACTIVATION_ACTION_DO_NOTHING :
			break;
		case ACTIVATION_ACTION_ASK :
			g_assert_not_reached ();
			break;
		}
	}

	launch_desktop_files = g_list_reverse (launch_desktop_files);
	for (l = launch_desktop_files; l != NULL; l = l->next) {
		file = NAUTILUS_FILE (l->data);
		
		uri = nautilus_file_get_uri (file);
		nautilus_debug_log (FALSE, NAUTILUS_DEBUG_LOG_DOMAIN_USER,
				    "directory view activate_callback launch_desktop_file window=%p: %s",
				    parameters->parent_window, uri);
		nautilus_launch_desktop_file (screen, uri, NULL,
					      parameters->parent_window);
		g_free (uri);
	}

	old_working_dir = NULL;
	if (parameters->activation_directory &&
	    (launch_files != NULL || launch_in_terminal_files != NULL)) {
		old_working_dir = g_get_current_dir ();
		g_chdir (parameters->activation_directory);
		
	}

	launch_files = g_list_reverse (launch_files);
	for (l = launch_files; l != NULL; l = l->next) {
		file = NAUTILUS_FILE (l->data);

		uri = nautilus_file_get_activation_uri (file);
		executable_path = g_filename_from_uri (uri, NULL, NULL);
		quoted_path = g_shell_quote (executable_path);
		name = nautilus_file_get_name (file);

		nautilus_debug_log (FALSE, NAUTILUS_DEBUG_LOG_DOMAIN_USER,
				    "directory view activate_callback launch_file window=%p: %s",
				    parameters->parent_window, quoted_path);

		nautilus_launch_application_from_command (screen, name, quoted_path, FALSE, NULL);
		g_free (name);
		g_free (quoted_path);
		g_free (executable_path);
		g_free (uri);
			
	}

	launch_in_terminal_files = g_list_reverse (launch_in_terminal_files);
	for (l = launch_in_terminal_files; l != NULL; l = l->next) {
		file = NAUTILUS_FILE (l->data);

		uri = nautilus_file_get_activation_uri (file);
		executable_path = g_filename_from_uri (uri, NULL, NULL);
		quoted_path = g_shell_quote (executable_path);
		name = nautilus_file_get_name (file);

		nautilus_debug_log (FALSE, NAUTILUS_DEBUG_LOG_DOMAIN_USER,
				    "directory view activate_callback launch_in_terminal window=%p: %s",
				    parameters->parent_window, quoted_path);

		nautilus_launch_application_from_command (screen, name, quoted_path, TRUE, NULL);
		g_free (name);
		g_free (quoted_path);
		g_free (executable_path);
		g_free (uri);
	}

	if (old_working_dir != NULL) {
		g_chdir (old_working_dir);
		g_free (old_working_dir);
	}

	open_in_view_files = g_list_reverse (open_in_view_files);
	count = g_list_length (open_in_view_files);

	flags = parameters->flags;
	if (count > 1) {
		if (eel_preferences_get_boolean (NAUTILUS_PREFERENCES_ENABLE_TABS) &&
		    (parameters->flags & NAUTILUS_WINDOW_OPEN_FLAG_NEW_WINDOW) == 0) {
			flags |= NAUTILUS_WINDOW_OPEN_FLAG_NEW_TAB;
		} else {
			flags |= NAUTILUS_WINDOW_OPEN_FLAG_NEW_WINDOW;
		}
	}

	if (parameters->slot_info != NULL &&
	    (!parameters->user_confirmation ||
	     confirm_multiple_windows (parameters->parent_window, count,
				       (flags & NAUTILUS_WINDOW_OPEN_FLAG_NEW_TAB) != 0))) {

		if ((flags & NAUTILUS_WINDOW_OPEN_FLAG_NEW_TAB) != 0 &&
		    eel_preferences_get_enum (NAUTILUS_PREFERENCES_NEW_TAB_POSITION) ==
		    NAUTILUS_NEW_TAB_POSITION_AFTER_CURRENT_TAB) {
			/* When inserting N tabs after the current one,
			 * we first open tab N, then tab N-1, ..., then tab 0.
			 * Each of them is appended to the current tab, i.e.
			 * prepended to the list of tabs to open.
			 */
			open_in_view_files = g_list_reverse (open_in_view_files);
		}


		for (l = open_in_view_files; l != NULL; l = l->next) {
			GFile *f;
			/* The ui should ask for navigation or object windows
			 * depending on what the current one is */
			file = NAUTILUS_FILE (l->data);

			uri = nautilus_file_get_activation_uri (file);
			f = g_file_new_for_uri (uri);
			nautilus_window_slot_info_open_location (parameters->slot_info,
								 f, parameters->mode, flags, NULL);
			g_object_unref (f);
			g_free (uri);
		}
	}

	open_in_app_parameters = NULL;
	unhandled_open_in_app_files = NULL;

	if (open_in_app_files != NULL) {
		open_in_app_files = g_list_reverse (open_in_app_files);

		open_in_app_parameters = fm_directory_view_make_activation_parameters
			(open_in_app_files, &unhandled_open_in_app_files);
	}

	for (l = open_in_app_parameters; l != NULL; l = l->next) {
		one_parameters = l->data;

		nautilus_launch_application (one_parameters->application,
					     one_parameters->files,
					     parameters->parent_window);
		application_launch_parameters_free (one_parameters);
	}

	for (l = unhandled_open_in_app_files; l != NULL; l = l->next) {
		file = NAUTILUS_FILE (l->data);

		/* this does not block */
		application_unhandled_file (parameters, file);
	}

	window_info = NULL;
	if (parameters->slot_info != NULL) {
		window_info = nautilus_window_slot_info_get_window (parameters->slot_info);
	}

	if (open_in_app_parameters != NULL ||
	    unhandled_open_in_app_files != NULL) {
		if ((parameters->flags & NAUTILUS_WINDOW_OPEN_FLAG_CLOSE_BEHIND) != 0 &&
		    window_info != NULL && 
		     nautilus_window_info_get_window_type (window_info) == NAUTILUS_WINDOW_SPATIAL) {
			nautilus_window_info_close (window_info);
		}
	}

	g_list_free (launch_desktop_files);
	g_list_free (launch_files);
	g_list_free (launch_in_terminal_files);
	g_list_free (open_in_view_files);
	g_list_free (open_in_app_files);
	g_list_free (open_in_app_parameters);
	g_list_free (unhandled_open_in_app_files);
	
	activation_parameters_free (parameters);
}