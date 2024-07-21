static int update_modify_options(struct libmnt_update *upd, struct libmnt_lock *lc)
{
	struct libmnt_table *tb = NULL;
	int rc = 0, u_lc = -1;
	struct libmnt_fs *fs;

	assert(upd);
	assert(upd->fs);

	DBG(UPDATE, mnt_debug_h(upd, "%s: modify options", upd->filename));

	fs = upd->fs;

	if (lc)
		mnt_lock_file(lc);
	else if (upd->userspace_only)
		u_lc = utab_lock(upd->filename);

	tb = __mnt_new_table_from_file(upd->filename,
			upd->userspace_only ? MNT_FMT_UTAB : MNT_FMT_MTAB);
	if (tb) {
		struct libmnt_fs *cur = mnt_table_find_target(tb,
					mnt_fs_get_target(fs),
					MNT_ITER_BACKWARD);
		if (cur) {
			if (upd->userspace_only)
				rc = mnt_fs_set_attributes(cur,	mnt_fs_get_attributes(fs));
			if (!rc && !upd->userspace_only)
				rc = mnt_fs_set_vfs_options(cur, mnt_fs_get_vfs_options(fs));
			if (!rc && !upd->userspace_only)
				rc = mnt_fs_set_fs_options(cur, mnt_fs_get_fs_options(fs));
			if (!rc)
				rc = mnt_fs_set_user_options(cur,
						mnt_fs_get_user_options(fs));
			if (!rc)
				rc = update_table(upd, tb);
		}
	}

	if (lc)
		mnt_unlock_file(lc);
	else if (u_lc != -1)
		utab_unlock(u_lc);

	mnt_free_table(tb);
	return rc;
}