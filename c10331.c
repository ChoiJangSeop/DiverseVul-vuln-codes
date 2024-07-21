int fuse_do_setattr(struct dentry *dentry, struct iattr *attr,
		    struct file *file)
{
	struct inode *inode = d_inode(dentry);
	struct fuse_mount *fm = get_fuse_mount(inode);
	struct fuse_conn *fc = fm->fc;
	struct fuse_inode *fi = get_fuse_inode(inode);
	FUSE_ARGS(args);
	struct fuse_setattr_in inarg;
	struct fuse_attr_out outarg;
	bool is_truncate = false;
	bool is_wb = fc->writeback_cache;
	loff_t oldsize;
	int err;
	bool trust_local_cmtime = is_wb && S_ISREG(inode->i_mode);
	bool fault_blocked = false;

	if (!fc->default_permissions)
		attr->ia_valid |= ATTR_FORCE;

	err = setattr_prepare(dentry, attr);
	if (err)
		return err;

	if (attr->ia_valid & ATTR_SIZE) {
		if (WARN_ON(!S_ISREG(inode->i_mode)))
			return -EIO;
		is_truncate = true;
	}

	if (FUSE_IS_DAX(inode) && is_truncate) {
		down_write(&fi->i_mmap_sem);
		fault_blocked = true;
		err = fuse_dax_break_layouts(inode, 0, 0);
		if (err) {
			up_write(&fi->i_mmap_sem);
			return err;
		}
	}

	if (attr->ia_valid & ATTR_OPEN) {
		/* This is coming from open(..., ... | O_TRUNC); */
		WARN_ON(!(attr->ia_valid & ATTR_SIZE));
		WARN_ON(attr->ia_size != 0);
		if (fc->atomic_o_trunc) {
			/*
			 * No need to send request to userspace, since actual
			 * truncation has already been done by OPEN.  But still
			 * need to truncate page cache.
			 */
			i_size_write(inode, 0);
			truncate_pagecache(inode, 0);
			goto out;
		}
		file = NULL;
	}

	/* Flush dirty data/metadata before non-truncate SETATTR */
	if (is_wb && S_ISREG(inode->i_mode) &&
	    attr->ia_valid &
			(ATTR_MODE | ATTR_UID | ATTR_GID | ATTR_MTIME_SET |
			 ATTR_TIMES_SET)) {
		err = write_inode_now(inode, true);
		if (err)
			return err;

		fuse_set_nowrite(inode);
		fuse_release_nowrite(inode);
	}

	if (is_truncate) {
		fuse_set_nowrite(inode);
		set_bit(FUSE_I_SIZE_UNSTABLE, &fi->state);
		if (trust_local_cmtime && attr->ia_size != inode->i_size)
			attr->ia_valid |= ATTR_MTIME | ATTR_CTIME;
	}

	memset(&inarg, 0, sizeof(inarg));
	memset(&outarg, 0, sizeof(outarg));
	iattr_to_fattr(fc, attr, &inarg, trust_local_cmtime);
	if (file) {
		struct fuse_file *ff = file->private_data;
		inarg.valid |= FATTR_FH;
		inarg.fh = ff->fh;
	}

	/* Kill suid/sgid for non-directory chown unconditionally */
	if (fc->handle_killpriv_v2 && !S_ISDIR(inode->i_mode) &&
	    attr->ia_valid & (ATTR_UID | ATTR_GID))
		inarg.valid |= FATTR_KILL_SUIDGID;

	if (attr->ia_valid & ATTR_SIZE) {
		/* For mandatory locking in truncate */
		inarg.valid |= FATTR_LOCKOWNER;
		inarg.lock_owner = fuse_lock_owner_id(fc, current->files);

		/* Kill suid/sgid for truncate only if no CAP_FSETID */
		if (fc->handle_killpriv_v2 && !capable(CAP_FSETID))
			inarg.valid |= FATTR_KILL_SUIDGID;
	}
	fuse_setattr_fill(fc, &args, inode, &inarg, &outarg);
	err = fuse_simple_request(fm, &args);
	if (err) {
		if (err == -EINTR)
			fuse_invalidate_attr(inode);
		goto error;
	}

	if (fuse_invalid_attr(&outarg.attr) ||
	    (inode->i_mode ^ outarg.attr.mode) & S_IFMT) {
		make_bad_inode(inode);
		err = -EIO;
		goto error;
	}

	spin_lock(&fi->lock);
	/* the kernel maintains i_mtime locally */
	if (trust_local_cmtime) {
		if (attr->ia_valid & ATTR_MTIME)
			inode->i_mtime = attr->ia_mtime;
		if (attr->ia_valid & ATTR_CTIME)
			inode->i_ctime = attr->ia_ctime;
		/* FIXME: clear I_DIRTY_SYNC? */
	}

	fuse_change_attributes_common(inode, &outarg.attr,
				      attr_timeout(&outarg));
	oldsize = inode->i_size;
	/* see the comment in fuse_change_attributes() */
	if (!is_wb || is_truncate || !S_ISREG(inode->i_mode))
		i_size_write(inode, outarg.attr.size);

	if (is_truncate) {
		/* NOTE: this may release/reacquire fi->lock */
		__fuse_release_nowrite(inode);
	}
	spin_unlock(&fi->lock);

	/*
	 * Only call invalidate_inode_pages2() after removing
	 * FUSE_NOWRITE, otherwise fuse_launder_page() would deadlock.
	 */
	if ((is_truncate || !is_wb) &&
	    S_ISREG(inode->i_mode) && oldsize != outarg.attr.size) {
		truncate_pagecache(inode, outarg.attr.size);
		invalidate_inode_pages2(inode->i_mapping);
	}

	clear_bit(FUSE_I_SIZE_UNSTABLE, &fi->state);
out:
	if (fault_blocked)
		up_write(&fi->i_mmap_sem);

	return 0;

error:
	if (is_truncate)
		fuse_release_nowrite(inode);

	clear_bit(FUSE_I_SIZE_UNSTABLE, &fi->state);

	if (fault_blocked)
		up_write(&fi->i_mmap_sem);
	return err;
}