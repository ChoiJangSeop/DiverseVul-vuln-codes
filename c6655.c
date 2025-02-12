int ovl_permission(struct inode *inode, int mask)
{
	struct ovl_entry *oe = inode->i_private;
	bool is_upper;
	struct dentry *realdentry = ovl_entry_real(oe, &is_upper);
	struct inode *realinode;
	int err;

	if (ovl_is_default_permissions(inode)) {
		struct kstat stat;
		struct path realpath = { .dentry = realdentry };

		if (mask & MAY_NOT_BLOCK)
			return -ECHILD;

		realpath.mnt = ovl_entry_mnt_real(oe, inode, is_upper);

		err = vfs_getattr(&realpath, &stat);
		if (err)
			return err;

		if ((stat.mode ^ inode->i_mode) & S_IFMT)
			return -ESTALE;

		inode->i_mode = stat.mode;
		inode->i_uid = stat.uid;
		inode->i_gid = stat.gid;

		return generic_permission(inode, mask);
	}

	/* Careful in RCU walk mode */
	realinode = d_inode_rcu(realdentry);
	if (!realinode) {
		WARN_ON(!(mask & MAY_NOT_BLOCK));
		return -ENOENT;
	}

	if (mask & MAY_WRITE) {
		umode_t mode = realinode->i_mode;

		/*
		 * Writes will always be redirected to upper layer, so
		 * ignore lower layer being read-only.
		 *
		 * If the overlay itself is read-only then proceed
		 * with the permission check, don't return EROFS.
		 * This will only happen if this is the lower layer of
		 * another overlayfs.
		 *
		 * If upper fs becomes read-only after the overlay was
		 * constructed return EROFS to prevent modification of
		 * upper layer.
		 */
		if (is_upper && !IS_RDONLY(inode) && IS_RDONLY(realinode) &&
		    (S_ISREG(mode) || S_ISDIR(mode) || S_ISLNK(mode)))
			return -EROFS;
	}

	return __inode_permission(realinode, mask);
}