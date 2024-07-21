static int fuse_permission(struct inode *inode, int mask)
{
	struct fuse_conn *fc = get_fuse_conn(inode);
	bool refreshed = false;
	int err = 0;

	if (!fuse_allow_current_process(fc))
		return -EACCES;

	/*
	 * If attributes are needed, refresh them before proceeding
	 */
	if (fc->default_permissions ||
	    ((mask & MAY_EXEC) && S_ISREG(inode->i_mode))) {
		struct fuse_inode *fi = get_fuse_inode(inode);
		u32 perm_mask = STATX_MODE | STATX_UID | STATX_GID;

		if (perm_mask & READ_ONCE(fi->inval_mask) ||
		    time_before64(fi->i_time, get_jiffies_64())) {
			refreshed = true;

			err = fuse_perm_getattr(inode, mask);
			if (err)
				return err;
		}
	}

	if (fc->default_permissions) {
		err = generic_permission(inode, mask);

		/* If permission is denied, try to refresh file
		   attributes.  This is also needed, because the root
		   node will at first have no permissions */
		if (err == -EACCES && !refreshed) {
			err = fuse_perm_getattr(inode, mask);
			if (!err)
				err = generic_permission(inode, mask);
		}

		/* Note: the opposite of the above test does not
		   exist.  So if permissions are revoked this won't be
		   noticed immediately, only after the attribute
		   timeout has expired */
	} else if (mask & (MAY_ACCESS | MAY_CHDIR)) {
		err = fuse_access(inode, mask);
	} else if ((mask & MAY_EXEC) && S_ISREG(inode->i_mode)) {
		if (!(inode->i_mode & S_IXUGO)) {
			if (refreshed)
				return -EACCES;

			err = fuse_perm_getattr(inode, mask);
			if (!err && !(inode->i_mode & S_IXUGO))
				return -EACCES;
		}
	}
	return err;
}