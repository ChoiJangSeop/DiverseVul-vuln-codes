static int fuse_setattr(struct dentry *entry, struct iattr *attr)
{
	struct inode *inode = d_inode(entry);
	struct fuse_conn *fc = get_fuse_conn(inode);
	struct file *file = (attr->ia_valid & ATTR_FILE) ? attr->ia_file : NULL;
	int ret;

	if (!fuse_allow_current_process(get_fuse_conn(inode)))
		return -EACCES;

	if (attr->ia_valid & (ATTR_KILL_SUID | ATTR_KILL_SGID)) {
		attr->ia_valid &= ~(ATTR_KILL_SUID | ATTR_KILL_SGID |
				    ATTR_MODE);

		/*
		 * The only sane way to reliably kill suid/sgid is to do it in
		 * the userspace filesystem
		 *
		 * This should be done on write(), truncate() and chown().
		 */
		if (!fc->handle_killpriv && !fc->handle_killpriv_v2) {
			/*
			 * ia_mode calculation may have used stale i_mode.
			 * Refresh and recalculate.
			 */
			ret = fuse_do_getattr(inode, NULL, file);
			if (ret)
				return ret;

			attr->ia_mode = inode->i_mode;
			if (inode->i_mode & S_ISUID) {
				attr->ia_valid |= ATTR_MODE;
				attr->ia_mode &= ~S_ISUID;
			}
			if ((inode->i_mode & (S_ISGID | S_IXGRP)) == (S_ISGID | S_IXGRP)) {
				attr->ia_valid |= ATTR_MODE;
				attr->ia_mode &= ~S_ISGID;
			}
		}
	}
	if (!attr->ia_valid)
		return 0;

	ret = fuse_do_setattr(entry, attr, file);
	if (!ret) {
		/*
		 * If filesystem supports acls it may have updated acl xattrs in
		 * the filesystem, so forget cached acls for the inode.
		 */
		if (fc->posix_acl)
			forget_all_cached_acls(inode);

		/* Directory mode changed, may need to revalidate access */
		if (d_is_dir(entry) && (attr->ia_valid & ATTR_MODE))
			fuse_invalidate_entry_cache(entry);
	}
	return ret;
}