struct inode *fuse_iget(struct super_block *sb, u64 nodeid,
			int generation, struct fuse_attr *attr,
			u64 attr_valid, u64 attr_version)
{
	struct inode *inode;
	struct fuse_inode *fi;
	struct fuse_conn *fc = get_fuse_conn_super(sb);

	/*
	 * Auto mount points get their node id from the submount root, which is
	 * not a unique identifier within this filesystem.
	 *
	 * To avoid conflicts, do not place submount points into the inode hash
	 * table.
	 */
	if (fc->auto_submounts && (attr->flags & FUSE_ATTR_SUBMOUNT) &&
	    S_ISDIR(attr->mode)) {
		inode = new_inode(sb);
		if (!inode)
			return NULL;

		fuse_init_inode(inode, attr);
		get_fuse_inode(inode)->nodeid = nodeid;
		inode->i_flags |= S_AUTOMOUNT;
		goto done;
	}

retry:
	inode = iget5_locked(sb, nodeid, fuse_inode_eq, fuse_inode_set, &nodeid);
	if (!inode)
		return NULL;

	if ((inode->i_state & I_NEW)) {
		inode->i_flags |= S_NOATIME;
		if (!fc->writeback_cache || !S_ISREG(attr->mode))
			inode->i_flags |= S_NOCMTIME;
		inode->i_generation = generation;
		fuse_init_inode(inode, attr);
		unlock_new_inode(inode);
	} else if ((inode->i_mode ^ attr->mode) & S_IFMT) {
		/* Inode has changed type, any I/O on the old should fail */
		make_bad_inode(inode);
		iput(inode);
		goto retry;
	}
done:
	fi = get_fuse_inode(inode);
	spin_lock(&fi->lock);
	fi->nlookup++;
	spin_unlock(&fi->lock);
	fuse_change_attributes(inode, attr, attr_valid, attr_version);

	return inode;
}