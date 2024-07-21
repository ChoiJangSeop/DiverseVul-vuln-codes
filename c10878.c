static int match_func(struct libmnt_fs *fs,
		      void *data __attribute__ ((__unused__)))
{
	int rc = flags & FL_INVERT ? 1 : 0;
	const char *m;
	void *md;

	m = get_match(COL_FSTYPE);
	if (m && !mnt_fs_match_fstype(fs, m))
		return rc;

	m = get_match(COL_OPTIONS);
	if (m && !mnt_fs_match_options(fs, m))
		return rc;

	md = get_match_data(COL_MAJMIN);
	if (md && mnt_fs_get_devno(fs) != *((dev_t *) md))
		return rc;

	m = get_match(COL_TARGET);
	if (m && !mnt_fs_match_target(fs, m, cache))
		return rc;

	m = get_match(COL_SOURCE);
	if (m && !mnt_fs_match_source(fs, m, cache))
		return rc;

	if ((flags & FL_DF) && !(flags & FL_ALL)) {
		const char *type = mnt_fs_get_fstype(fs);

		if (type && strstr(type, "tmpfs"))	/* tmpfs is wanted */
			return !rc;

		if (mnt_fs_is_pseudofs(fs))
			return rc;
	}

	if ((flags & FL_REAL) && mnt_fs_is_pseudofs(fs))
	    return rc;

	if ((flags & FL_PSEUDO) && !mnt_fs_is_pseudofs(fs))
	    return rc;

	if ((flags & FL_SHADOWED)) {
		struct libmnt_table *tb = NULL;

		mnt_fs_get_table(fs, &tb);
		if (tb && mnt_table_over_fs(tb, fs, NULL) != 0)
			return rc;
	}

	if ((flags & FL_DELETED) && !mnt_fs_is_deleted(fs))
		return rc;

	return !rc;
}