void fs_rebuild_etc(void) {
	int have_dhcp = 1;
	if (cfg.dns1 == NULL && !any_dhcp())
		have_dhcp = 0;

	if (arg_debug)
		printf("rebuilding /etc directory\n");
	if (mkdir(RUN_DNS_ETC, 0755))
		errExit("mkdir");
	selinux_relabel_path(RUN_DNS_ETC, "/etc");
	fs_logger("tmpfs /etc");

	DIR *dir = opendir("/etc");
	if (!dir)
		errExit("opendir");

	struct stat s;
	struct dirent *entry;
	while ((entry = readdir(dir))) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		// skip files in cfg.profile_rebuild_etc list
		// these files are already blacklisted
		{
			ProfileEntry *prf = cfg.profile_rebuild_etc;
			int found = 0;
			while (prf) {
				if (strcmp(entry->d_name, prf->data + 5) == 0) { // 5 is strlen("/etc/")
					found = 1;
					break;
				}
				prf = prf->next;
			}
			if (found)
				continue;
		}

		// for resolv.conf we might have to  create a brand new file later
		if (have_dhcp &&
		    (strcmp(entry->d_name, "resolv.conf") == 0 ||
		     strcmp(entry->d_name, "resolv.conf.dhclient-new") == 0))
			continue;
//		printf("linking %s\n", entry->d_name);

		char *src;
		if (asprintf(&src, "/etc/%s", entry->d_name) == -1)
			errExit("asprintf");
		if (stat(src, &s) != 0) {
			free(src);
			continue;
		}

		char *dest;
		if (asprintf(&dest, "%s/%s", RUN_DNS_ETC, entry->d_name) == -1)
			errExit("asprintf");

		int symlink_done = 0;
		if (is_link(src)) {
			char *rp =realpath(src, NULL);
			if (rp == NULL) {
				free(src);
				free(dest);
				continue;
			}
			if (symlink(rp, dest))
				errExit("symlink");
			else
				symlink_done = 1;
		}
		else if (S_ISDIR(s.st_mode))
			create_empty_dir_as_root(dest, s.st_mode);
		else
			create_empty_file_as_root(dest, s.st_mode);

		// bind-mount src on top of dest
		if (!symlink_done) {
			if (mount(src, dest, NULL, MS_BIND|MS_REC, NULL) < 0)
				errExit("mount bind mirroring /etc");
		}
		fs_logger2("clone", src);

		free(src);
		free(dest);
	}
	closedir(dir);

	// mount bind our private etc directory on top of /etc
	if (arg_debug)
		printf("Mount-bind %s on top of /etc\n", RUN_DNS_ETC);
	if (mount(RUN_DNS_ETC, "/etc", NULL, MS_BIND|MS_REC, NULL) < 0)
		errExit("mount bind mirroring /etc");
	fs_logger("mount /etc");

	if (have_dhcp == 0)
		return;

	if (arg_debug)
		printf("Creating a new /etc/resolv.conf file\n");
	FILE *fp = fopen("/etc/resolv.conf", "wxe");
	if (!fp) {
		fprintf(stderr, "Error: cannot create /etc/resolv.conf file\n");
		exit(1);
	}

	if (cfg.dns1) {
		if (any_dhcp())
			fwarning("network setup uses DHCP, nameservers will likely be overwritten\n");
		fprintf(fp, "nameserver %s\n", cfg.dns1);
	}
	if (cfg.dns2)
		fprintf(fp, "nameserver %s\n", cfg.dns2);
	if (cfg.dns3)
		fprintf(fp, "nameserver %s\n", cfg.dns3);
	if (cfg.dns4)
		fprintf(fp, "nameserver %s\n", cfg.dns4);

	// mode and owner
	SET_PERMS_STREAM(fp, 0, 0, 0644);

	fclose(fp);

	fs_logger("create /etc/resolv.conf");
}