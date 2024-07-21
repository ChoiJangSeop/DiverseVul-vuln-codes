static int fuse_dir_fsync(struct file *file, loff_t start, loff_t end,
			  int datasync)
{
	struct inode *inode = file->f_mapping->host;
	struct fuse_conn *fc = get_fuse_conn(inode);
	int err;

	if (is_bad_inode(inode))
		return -EIO;

	if (fc->no_fsyncdir)
		return 0;

	inode_lock(inode);
	err = fuse_fsync_common(file, start, end, datasync, FUSE_FSYNCDIR);
	if (err == -ENOSYS) {
		fc->no_fsyncdir = 1;
		err = 0;
	}
	inode_unlock(inode);

	return err;
}