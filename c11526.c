void preproc_build_firejail_dir(void) {
	struct stat s;

	// CentOS 6 doesn't have /run directory
	if (stat(RUN_FIREJAIL_BASEDIR, &s)) {
		create_empty_dir_as_root(RUN_FIREJAIL_BASEDIR, 0755);
	}

	if (stat(RUN_FIREJAIL_DIR, &s)) {
		create_empty_dir_as_root(RUN_FIREJAIL_DIR, 0755);
	}

	if (stat(RUN_FIREJAIL_NETWORK_DIR, &s)) {
		create_empty_dir_as_root(RUN_FIREJAIL_NETWORK_DIR, 0755);
	}

	if (stat(RUN_FIREJAIL_BANDWIDTH_DIR, &s)) {
		create_empty_dir_as_root(RUN_FIREJAIL_BANDWIDTH_DIR, 0755);
	}

	if (stat(RUN_FIREJAIL_NAME_DIR, &s)) {
		create_empty_dir_as_root(RUN_FIREJAIL_NAME_DIR, 0755);
	}

	if (stat(RUN_FIREJAIL_PROFILE_DIR, &s)) {
		create_empty_dir_as_root(RUN_FIREJAIL_PROFILE_DIR, 0755);
	}

	if (stat(RUN_FIREJAIL_X11_DIR, &s)) {
		create_empty_dir_as_root(RUN_FIREJAIL_X11_DIR, 0755);
	}

	if (stat(RUN_FIREJAIL_DBUS_DIR, &s)) {
		create_empty_dir_as_root(RUN_FIREJAIL_DBUS_DIR, 0755);
		if (arg_debug)
			printf("Remounting the " RUN_FIREJAIL_DBUS_DIR
				   " directory as noexec\n");
		if (mount(RUN_FIREJAIL_DBUS_DIR, RUN_FIREJAIL_DBUS_DIR, NULL,
				  MS_BIND, NULL) == -1)
			errExit("mounting " RUN_FIREJAIL_DBUS_DIR);
		if (mount(NULL, RUN_FIREJAIL_DBUS_DIR, NULL,
				  MS_REMOUNT | MS_BIND | MS_NOSUID | MS_NOEXEC | MS_NODEV,
				  "mode=755,gid=0") == -1)
			errExit("remounting " RUN_FIREJAIL_DBUS_DIR);
	}

	if (stat(RUN_FIREJAIL_APPIMAGE_DIR, &s)) {
		create_empty_dir_as_root(RUN_FIREJAIL_APPIMAGE_DIR, 0755);
	}

	if (stat(RUN_FIREJAIL_LIB_DIR, &s)) {
		create_empty_dir_as_root(RUN_FIREJAIL_LIB_DIR, 0755);
	}

	if (stat(RUN_MNT_DIR, &s)) {
		create_empty_dir_as_root(RUN_MNT_DIR, 0755);
	}

	create_empty_file_as_root(RUN_RO_FILE, S_IRUSR);
	fs_remount(RUN_RO_FILE, MOUNT_READONLY, 0);

	create_empty_dir_as_root(RUN_RO_DIR, S_IRUSR);
	fs_remount(RUN_RO_DIR, MOUNT_READONLY, 0);
}