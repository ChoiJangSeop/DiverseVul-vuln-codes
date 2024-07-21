static int fuse_dentry_revalidate(struct dentry *entry, unsigned int flags)
{
	struct inode *inode;
	struct dentry *parent;
	struct fuse_mount *fm;
	struct fuse_inode *fi;
	int ret;

	inode = d_inode_rcu(entry);
	if (inode && is_bad_inode(inode))
		goto invalid;
	else if (time_before64(fuse_dentry_time(entry), get_jiffies_64()) ||
		 (flags & (LOOKUP_EXCL | LOOKUP_REVAL))) {
		struct fuse_entry_out outarg;
		FUSE_ARGS(args);
		struct fuse_forget_link *forget;
		u64 attr_version;

		/* For negative dentries, always do a fresh lookup */
		if (!inode)
			goto invalid;

		ret = -ECHILD;
		if (flags & LOOKUP_RCU)
			goto out;

		fm = get_fuse_mount(inode);

		forget = fuse_alloc_forget();
		ret = -ENOMEM;
		if (!forget)
			goto out;

		attr_version = fuse_get_attr_version(fm->fc);

		parent = dget_parent(entry);
		fuse_lookup_init(fm->fc, &args, get_node_id(d_inode(parent)),
				 &entry->d_name, &outarg);
		ret = fuse_simple_request(fm, &args);
		dput(parent);
		/* Zero nodeid is same as -ENOENT */
		if (!ret && !outarg.nodeid)
			ret = -ENOENT;
		if (!ret) {
			fi = get_fuse_inode(inode);
			if (outarg.nodeid != get_node_id(inode) ||
			    (bool) IS_AUTOMOUNT(inode) != (bool) (outarg.attr.flags & FUSE_ATTR_SUBMOUNT)) {
				fuse_queue_forget(fm->fc, forget,
						  outarg.nodeid, 1);
				goto invalid;
			}
			spin_lock(&fi->lock);
			fi->nlookup++;
			spin_unlock(&fi->lock);
		}
		kfree(forget);
		if (ret == -ENOMEM)
			goto out;
		if (ret || fuse_invalid_attr(&outarg.attr) ||
		    (outarg.attr.mode ^ inode->i_mode) & S_IFMT)
			goto invalid;

		forget_all_cached_acls(inode);
		fuse_change_attributes(inode, &outarg.attr,
				       entry_attr_timeout(&outarg),
				       attr_version);
		fuse_change_entry_timeout(entry, &outarg);
	} else if (inode) {
		fi = get_fuse_inode(inode);
		if (flags & LOOKUP_RCU) {
			if (test_bit(FUSE_I_INIT_RDPLUS, &fi->state))
				return -ECHILD;
		} else if (test_and_clear_bit(FUSE_I_INIT_RDPLUS, &fi->state)) {
			parent = dget_parent(entry);
			fuse_advise_use_readdirplus(d_inode(parent));
			dput(parent);
		}
	}
	ret = 1;
out:
	return ret;

invalid:
	ret = 0;
	goto out;
}