static long vmsplice_to_user(struct file *file, const struct iovec __user *iov,
			     unsigned long nr_segs, unsigned int flags)
{
	struct pipe_inode_info *pipe;
	struct splice_desc sd;
	ssize_t size;
	int error;
	long ret;

	pipe = pipe_info(file->f_path.dentry->d_inode);
	if (!pipe)
		return -EBADF;

	if (pipe->inode)
		mutex_lock(&pipe->inode->i_mutex);

	error = ret = 0;
	while (nr_segs) {
		void __user *base;
		size_t len;

		/*
		 * Get user address base and length for this iovec.
		 */
		error = get_user(base, &iov->iov_base);
		if (unlikely(error))
			break;
		error = get_user(len, &iov->iov_len);
		if (unlikely(error))
			break;

		/*
		 * Sanity check this iovec. 0 read succeeds.
		 */
		if (unlikely(!len))
			break;
		if (unlikely(!base)) {
			error = -EFAULT;
			break;
		}

		sd.len = 0;
		sd.total_len = len;
		sd.flags = flags;
		sd.u.userptr = base;
		sd.pos = 0;

		size = __splice_from_pipe(pipe, &sd, pipe_to_user);
		if (size < 0) {
			if (!ret)
				ret = size;

			break;
		}

		ret += size;

		if (size < len)
			break;

		nr_segs--;
		iov++;
	}

	if (pipe->inode)
		mutex_unlock(&pipe->inode->i_mutex);

	if (!ret)
		ret = error;

	return ret;
}