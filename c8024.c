v9fs_file_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
	struct file *file = iocb->ki_filp;
	ssize_t retval;
	loff_t origin;
	int err = 0;

	retval = generic_write_checks(iocb, from);
	if (retval <= 0)
		return retval;

	origin = iocb->ki_pos;
	retval = p9_client_write(file->private_data, iocb->ki_pos, from, &err);
	if (retval > 0) {
		struct inode *inode = file_inode(file);
		loff_t i_size;
		unsigned long pg_start, pg_end;
		pg_start = origin >> PAGE_SHIFT;
		pg_end = (origin + retval - 1) >> PAGE_SHIFT;
		if (inode->i_mapping && inode->i_mapping->nrpages)
			invalidate_inode_pages2_range(inode->i_mapping,
						      pg_start, pg_end);
		iocb->ki_pos += retval;
		i_size = i_size_read(inode);
		if (iocb->ki_pos > i_size) {
			inode_add_bytes(inode, iocb->ki_pos - i_size);
			i_size_write(inode, iocb->ki_pos);
		}
		return retval;
	}
	return err;
}