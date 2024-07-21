static int fuse_atomic_open(struct inode *dir, struct dentry *entry,
			    struct file *file, unsigned flags,
			    umode_t mode)
{
	int err;
	struct fuse_conn *fc = get_fuse_conn(dir);
	struct dentry *res = NULL;

	if (d_in_lookup(entry)) {
		res = fuse_lookup(dir, entry, 0);
		if (IS_ERR(res))
			return PTR_ERR(res);

		if (res)
			entry = res;
	}

	if (!(flags & O_CREAT) || d_really_is_positive(entry))
		goto no_open;

	/* Only creates */
	file->f_mode |= FMODE_CREATED;

	if (fc->no_create)
		goto mknod;

	err = fuse_create_open(dir, entry, file, flags, mode);
	if (err == -ENOSYS) {
		fc->no_create = 1;
		goto mknod;
	}
out_dput:
	dput(res);
	return err;

mknod:
	err = fuse_mknod(dir, entry, mode, 0);
	if (err)
		goto out_dput;
no_open:
	return finish_no_open(file, res);
}