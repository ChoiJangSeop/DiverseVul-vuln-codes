int fuse_readdir(struct file *file, struct dir_context *ctx)
{
	struct fuse_file *ff = file->private_data;
	struct inode *inode = file_inode(file);
	int err;

	if (is_bad_inode(inode))
		return -EIO;

	mutex_lock(&ff->readdir.lock);

	err = UNCACHED;
	if (ff->open_flags & FOPEN_CACHE_DIR)
		err = fuse_readdir_cached(file, ctx);
	if (err == UNCACHED)
		err = fuse_readdir_uncached(file, ctx);

	mutex_unlock(&ff->readdir.lock);

	return err;
}