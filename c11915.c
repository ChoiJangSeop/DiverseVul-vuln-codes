static int fprintf_mtab_fs(FILE *f, struct libmnt_fs *fs)
{
	char *o;
	char *m1, *m2, *m3, *m4;
	int rc;

	assert(fs);
	assert(f);

	o = mnt_fs_strdup_options(fs);
	if (!o)
		return -ENOMEM;

	m1 = mangle(mnt_fs_get_source(fs));
	m2 = mangle(mnt_fs_get_target(fs));
	m3 = mangle(mnt_fs_get_fstype(fs));
	m4 = mangle(o);

	if (m1 && m2 && m3 && m4)
		rc = !fprintf(f, "%s %s %s %s %d %d\n",
				m1, m2, m3, m4,
				mnt_fs_get_freq(fs),
				mnt_fs_get_passno(fs));
	else
		rc = -ENOMEM;

	free(o);
	free(m1);
	free(m2);
	free(m3);
	free(m4);

	return rc;
}