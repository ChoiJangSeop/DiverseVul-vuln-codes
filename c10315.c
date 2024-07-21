static int fuse_getattr(const struct path *path, struct kstat *stat,
			u32 request_mask, unsigned int flags)
{
	struct inode *inode = d_inode(path->dentry);
	struct fuse_conn *fc = get_fuse_conn(inode);

	if (!fuse_allow_current_process(fc)) {
		if (!request_mask) {
			/*
			 * If user explicitly requested *nothing* then don't
			 * error out, but return st_dev only.
			 */
			stat->result_mask = 0;
			stat->dev = inode->i_sb->s_dev;
			return 0;
		}
		return -EACCES;
	}

	return fuse_update_get_attr(inode, NULL, stat, request_mask, flags);
}