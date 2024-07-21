struct libmnt_update *mnt_new_update(void)
{
	struct libmnt_update *upd;

	upd = calloc(1, sizeof(*upd));
	if (!upd)
		return NULL;

	DBG(UPDATE, mnt_debug_h(upd, "allocate"));

	return upd;
}