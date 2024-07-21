static int update_add_entry(struct libmnt_update *upd, struct libmnt_lock *lc)
{
	FILE *f;
	int rc = 0, u_lc = -1;

	assert(upd);
	assert(upd->fs);

	DBG(UPDATE, mnt_debug_h(upd, "%s: add entry", upd->filename));

	if (lc)
		mnt_lock_file(lc);
	else if (upd->userspace_only)
		u_lc = utab_lock(upd->filename);

	f = fopen(upd->filename, "a+");
	if (f) {
		rc = upd->userspace_only ? fprintf_utab_fs(f, upd->fs) :
					   fprintf_mtab_fs(f, upd->fs);
		DBG(UPDATE, mnt_debug_h(upd, "%s: add [rc=%d]", upd->filename, rc));
		fclose(f);
	} else {
		DBG(UPDATE, mnt_debug_h(upd, "%s: failed: %m", upd->filename));
		rc = -errno;
	}
	if (lc)
		mnt_unlock_file(lc);
	else if (u_lc != -1)
		utab_unlock(u_lc);
	return rc;
}