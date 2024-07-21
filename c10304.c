static ssize_t fuse_file_read_iter(struct kiocb *iocb, struct iov_iter *to)
{
	struct file *file = iocb->ki_filp;
	struct fuse_file *ff = file->private_data;
	struct inode *inode = file_inode(file);

	if (is_bad_inode(inode))
		return -EIO;

	if (FUSE_IS_DAX(inode))
		return fuse_dax_read_iter(iocb, to);

	if (!(ff->open_flags & FOPEN_DIRECT_IO))
		return fuse_cache_read_iter(iocb, to);
	else
		return fuse_direct_read_iter(iocb, to);
}