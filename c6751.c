amgtar_selfcheck(
    application_argument_t *argument)
{
    if (argument->dle.disk) {
	delete_message(print_message(build_message(
			AMANDA_FILE, __LINE__, 3700000, MSG_INFO, 3,
			"disk", argument->dle.disk,
			"device", argument->dle.device,
			"hostname", argument->host)));
    }

    delete_message(print_message(build_message(
			AMANDA_FILE, __LINE__, 3700001, MSG_INFO, 4,
			"version", VERSION,
			"disk", argument->dle.disk,
			"device", argument->dle.device,
			"hostname", argument->host)));
    amgtar_build_exinclude(&argument->dle, 1, NULL, NULL, NULL, NULL);

    delete_message(print_message(build_message(
			AMANDA_FILE, __LINE__, 3700004, MSG_INFO, 3,
			"disk", argument->dle.disk,
			"device", argument->dle.device,
			"hostname", argument->host)));
    if (gnutar_path) {
	message_t *message;
	if ((message = check_exec_for_suid_message(gnutar_path))) {
	    delete_message(print_message(message));
	} else {

	    message = print_message(check_file_message(gnutar_path, X_OK));
	    if (message && message_get_severity(message) <= MSG_INFO) {
		char *gtar_version;
		GPtrArray *argv_ptr = g_ptr_array_new();

		g_ptr_array_add(argv_ptr, gnutar_path);
		g_ptr_array_add(argv_ptr, "--version");
		g_ptr_array_add(argv_ptr, NULL);

		gtar_version = get_first_line(argv_ptr);
		if (gtar_version) {
		    char *gv;
		    for (gv = gtar_version; *gv && !g_ascii_isdigit(*gv); gv++);
		    delete_message(print_message(build_message(
			AMANDA_FILE, __LINE__, 3700002, MSG_INFO, 4,
			"gtar-version", gv,
			"disk", argument->dle.disk,
			"device", argument->dle.device,
			"hostname", argument->host)));
		} else {
		    delete_message(print_message(build_message(
			AMANDA_FILE, __LINE__, 3700003, MSG_ERROR, 4,
			"gtar-path", gnutar_path,
			"disk", argument->dle.disk,
			"device", argument->dle.device,
			"hostname", argument->host)));
		}

		g_ptr_array_free(argv_ptr, TRUE);
		amfree(gtar_version);
	    }
	    if (message)
		delete_message(message);
	}
    } else {
	delete_message(print_message(build_message(
			AMANDA_FILE, __LINE__, 3700005, MSG_ERROR, 3,
			"disk", argument->dle.disk,
			"device", argument->dle.device,
			"hostname", argument->host)));
    }
    if (gnutar_listdir && strlen(gnutar_listdir) == 0)
	gnutar_listdir = NULL;
    if (gnutar_listdir) {
	delete_message(print_message(check_dir_message(gnutar_listdir, R_OK|W_OK)));
    } else {
	delete_message(print_message(build_message(
			AMANDA_FILE, __LINE__, 3700006, MSG_ERROR, 3,
			"disk", argument->dle.disk,
			"device", argument->dle.device,
			"hostname", argument->host)));
    }

    set_root_privs(1);
    if (gnutar_directory) {
	delete_message(print_message(check_dir_message(gnutar_directory, R_OK)));
    } else if (argument->dle.device) {
	delete_message(print_message(check_dir_message(argument->dle.device, R_OK)));
    }
    if (argument->calcsize) {
	char *calcsize = g_strjoin(NULL, amlibexecdir, "/", "calcsize", NULL);
	delete_message(print_message(check_exec_for_suid_message(calcsize)));
	delete_message(print_message(check_file_message(calcsize, X_OK)));
	delete_message(print_message(check_suid_message(calcsize)));
	amfree(calcsize);
    }
    set_root_privs(0);
}