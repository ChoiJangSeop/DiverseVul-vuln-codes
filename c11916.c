static int update_table(struct libmnt_update *upd, struct libmnt_table *tb)
{
	FILE *f;
	int rc, fd;
	char *uq = NULL;

	if (!tb || !upd->filename)
		return -EINVAL;

	DBG(UPDATE, mnt_debug_h(upd, "%s: updating", upd->filename));

	fd = mnt_open_uniq_filename(upd->filename, &uq);
	if (fd < 0)
		return fd;	/* error */

	f = fdopen(fd, "w");
	if (f) {
		struct stat st;
		struct libmnt_iter itr;
		struct libmnt_fs *fs;
		int fd;

		mnt_reset_iter(&itr, MNT_ITER_FORWARD);
		while(mnt_table_next_fs(tb, &itr, &fs) == 0) {
			if (upd->userspace_only)
				fprintf_utab_fs(f, fs);
			else
				fprintf_mtab_fs(f, fs);
		}
		fd = fileno(f);
		rc = fchmod(fd, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH) ? -errno : 0;

		if (!rc && stat(upd->filename, &st) == 0)
			/* Copy uid/gid from the present file before renaming. */
			rc = fchown(fd, st.st_uid, st.st_gid) ? -errno : 0;

		fclose(f);
		rc = rename(uq, upd->filename) ? -errno : 0;
	} else {
		rc = -errno;
		close(fd);
	}

	unlink(uq);	/* be paranoid */
	free(uq);
	return rc;
}