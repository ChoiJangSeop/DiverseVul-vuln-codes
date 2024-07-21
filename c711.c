add_mtab(char *devname, char *mountpoint, unsigned long flags)
{
	int rc = 0;
	char *mount_user;
	struct mntent mountent;
	FILE *pmntfile;

	atexit(unlock_mtab);
	rc = lock_mtab();
	if (rc) {
		fprintf(stderr, "cannot lock mtab");
		rc = EX_FILEIO;
		goto add_mtab_exit;
	}

	pmntfile = setmntent(MOUNTED, "a+");
	if (!pmntfile) {
		fprintf(stderr, "could not update mount table\n");
		unlock_mtab();
		rc = EX_FILEIO;
		goto add_mtab_exit;
	}

	mountent.mnt_fsname = devname;
	mountent.mnt_dir = mountpoint;
	mountent.mnt_type = (char *)(void *)cifs_fstype;
	mountent.mnt_opts = (char *)calloc(MTAB_OPTIONS_LEN, 1);
	if (mountent.mnt_opts) {
		if (flags & MS_RDONLY)
			strlcat(mountent.mnt_opts, "ro", MTAB_OPTIONS_LEN);
		else
			strlcat(mountent.mnt_opts, "rw", MTAB_OPTIONS_LEN);

		if (flags & MS_MANDLOCK)
			strlcat(mountent.mnt_opts, ",mand", MTAB_OPTIONS_LEN);
		if (flags & MS_NOEXEC)
			strlcat(mountent.mnt_opts, ",noexec", MTAB_OPTIONS_LEN);
		if (flags & MS_NOSUID)
			strlcat(mountent.mnt_opts, ",nosuid", MTAB_OPTIONS_LEN);
		if (flags & MS_NODEV)
			strlcat(mountent.mnt_opts, ",nodev", MTAB_OPTIONS_LEN);
		if (flags & MS_SYNCHRONOUS)
			strlcat(mountent.mnt_opts, ",sync", MTAB_OPTIONS_LEN);
		if (getuid() != 0) {
			strlcat(mountent.mnt_opts, ",user=", MTAB_OPTIONS_LEN);
			mount_user = getusername();
			if (mount_user)
				strlcat(mountent.mnt_opts, mount_user,
					MTAB_OPTIONS_LEN);
		}
	}
	mountent.mnt_freq = 0;
	mountent.mnt_passno = 0;
	rc = addmntent(pmntfile, &mountent);
	endmntent(pmntfile);
	unlock_mtab();
	SAFE_FREE(mountent.mnt_opts);
add_mtab_exit:
	if (rc) {
		fprintf(stderr, "unable to add mount entry to mtab\n");
		rc = EX_FILEIO;
	}

	return rc;
}