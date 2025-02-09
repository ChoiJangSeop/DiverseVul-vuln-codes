int __fsnotify_parent(const struct path *path, struct dentry *dentry, __u32 mask)
{
	struct dentry *parent;
	struct inode *p_inode;
	int ret = 0;

	if (!dentry)
		dentry = path->dentry;

	if (!(dentry->d_flags & DCACHE_FSNOTIFY_PARENT_WATCHED))
		return 0;

	parent = dget_parent(dentry);
	p_inode = parent->d_inode;

	if (unlikely(!fsnotify_inode_watches_children(p_inode)))
		__fsnotify_update_child_dentry_flags(p_inode);
	else if (p_inode->i_fsnotify_mask & mask) {
		/* we are notifying a parent so come up with the new mask which
		 * specifies these are events which came from a child. */
		mask |= FS_EVENT_ON_CHILD;

		if (path)
			ret = fsnotify(p_inode, mask, path, FSNOTIFY_EVENT_PATH,
				       dentry->d_name.name, 0);
		else
			ret = fsnotify(p_inode, mask, dentry->d_inode, FSNOTIFY_EVENT_INODE,
				       dentry->d_name.name, 0);
	}

	dput(parent);

	return ret;
}