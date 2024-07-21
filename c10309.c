static int fuse_rename2(struct inode *olddir, struct dentry *oldent,
			struct inode *newdir, struct dentry *newent,
			unsigned int flags)
{
	struct fuse_conn *fc = get_fuse_conn(olddir);
	int err;

	if (flags & ~(RENAME_NOREPLACE | RENAME_EXCHANGE | RENAME_WHITEOUT))
		return -EINVAL;

	if (flags) {
		if (fc->no_rename2 || fc->minor < 23)
			return -EINVAL;

		err = fuse_rename_common(olddir, oldent, newdir, newent, flags,
					 FUSE_RENAME2,
					 sizeof(struct fuse_rename2_in));
		if (err == -ENOSYS) {
			fc->no_rename2 = 1;
			err = -EINVAL;
		}
	} else {
		err = fuse_rename_common(olddir, oldent, newdir, newent, 0,
					 FUSE_RENAME,
					 sizeof(struct fuse_rename_in));
	}

	return err;
}