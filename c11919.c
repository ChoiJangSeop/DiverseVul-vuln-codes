static int update_remove_entry(struct libmnt_update *upd, struct libmnt_lock *lc)
{
	struct libmnt_table *tb;
	int rc = 0, u_lc = -1;

	assert(upd);
	assert(upd->target);

	DBG(UPDATE, mnt_debug_h(upd, "%s: remove entry", upd->filename));

	if (lc)
		mnt_lock_file(lc);
	else if (upd->userspace_only)
		u_lc = utab_lock(upd->filename);

	tb = __mnt_new_table_from_file(upd->filename,
			upd->userspace_only ? MNT_FMT_UTAB : MNT_FMT_MTAB);
	if (tb) {
		struct libmnt_fs *rem = mnt_table_find_target(tb, upd->target, MNT_ITER_BACKWARD);
		if (rem) {
			mnt_table_remove_fs(tb, rem);
			rc = update_table(upd, tb);
			mnt_free_fs(rem);
		}
	}
	if (lc)
		mnt_unlock_file(lc);
	else if (u_lc != -1)
		utab_unlock(u_lc);

	mnt_free_table(tb);
	return rc;
}