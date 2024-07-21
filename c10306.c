static int fuse_direntplus_link(struct file *file,
				struct fuse_direntplus *direntplus,
				u64 attr_version)
{
	struct fuse_entry_out *o = &direntplus->entry_out;
	struct fuse_dirent *dirent = &direntplus->dirent;
	struct dentry *parent = file->f_path.dentry;
	struct qstr name = QSTR_INIT(dirent->name, dirent->namelen);
	struct dentry *dentry;
	struct dentry *alias;
	struct inode *dir = d_inode(parent);
	struct fuse_conn *fc;
	struct inode *inode;
	DECLARE_WAIT_QUEUE_HEAD_ONSTACK(wq);

	if (!o->nodeid) {
		/*
		 * Unlike in the case of fuse_lookup, zero nodeid does not mean
		 * ENOENT. Instead, it only means the userspace filesystem did
		 * not want to return attributes/handle for this entry.
		 *
		 * So do nothing.
		 */
		return 0;
	}

	if (name.name[0] == '.') {
		/*
		 * We could potentially refresh the attributes of the directory
		 * and its parent?
		 */
		if (name.len == 1)
			return 0;
		if (name.name[1] == '.' && name.len == 2)
			return 0;
	}

	if (invalid_nodeid(o->nodeid))
		return -EIO;
	if (fuse_invalid_attr(&o->attr))
		return -EIO;

	fc = get_fuse_conn(dir);

	name.hash = full_name_hash(parent, name.name, name.len);
	dentry = d_lookup(parent, &name);
	if (!dentry) {
retry:
		dentry = d_alloc_parallel(parent, &name, &wq);
		if (IS_ERR(dentry))
			return PTR_ERR(dentry);
	}
	if (!d_in_lookup(dentry)) {
		struct fuse_inode *fi;
		inode = d_inode(dentry);
		if (!inode ||
		    get_node_id(inode) != o->nodeid ||
		    ((o->attr.mode ^ inode->i_mode) & S_IFMT)) {
			d_invalidate(dentry);
			dput(dentry);
			goto retry;
		}
		if (is_bad_inode(inode)) {
			dput(dentry);
			return -EIO;
		}

		fi = get_fuse_inode(inode);
		spin_lock(&fi->lock);
		fi->nlookup++;
		spin_unlock(&fi->lock);

		forget_all_cached_acls(inode);
		fuse_change_attributes(inode, &o->attr,
				       entry_attr_timeout(o),
				       attr_version);
		/*
		 * The other branch comes via fuse_iget()
		 * which bumps nlookup inside
		 */
	} else {
		inode = fuse_iget(dir->i_sb, o->nodeid, o->generation,
				  &o->attr, entry_attr_timeout(o),
				  attr_version);
		if (!inode)
			inode = ERR_PTR(-ENOMEM);

		alias = d_splice_alias(inode, dentry);
		d_lookup_done(dentry);
		if (alias) {
			dput(dentry);
			dentry = alias;
		}
		if (IS_ERR(dentry))
			return PTR_ERR(dentry);
	}
	if (fc->readdirplus_auto)
		set_bit(FUSE_I_INIT_RDPLUS, &get_fuse_inode(inode)->state);
	fuse_change_entry_timeout(dentry, o);

	dput(dentry);
	return 0;
}