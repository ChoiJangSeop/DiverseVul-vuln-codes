static int fuse_unlink(struct inode *dir, struct dentry *entry)
{
	int err;
	struct fuse_mount *fm = get_fuse_mount(dir);
	FUSE_ARGS(args);

	args.opcode = FUSE_UNLINK;
	args.nodeid = get_node_id(dir);
	args.in_numargs = 1;
	args.in_args[0].size = entry->d_name.len + 1;
	args.in_args[0].value = entry->d_name.name;
	err = fuse_simple_request(fm, &args);
	if (!err) {
		struct inode *inode = d_inode(entry);
		struct fuse_inode *fi = get_fuse_inode(inode);

		spin_lock(&fi->lock);
		fi->attr_version = atomic64_inc_return(&fm->fc->attr_version);
		/*
		 * If i_nlink == 0 then unlink doesn't make sense, yet this can
		 * happen if userspace filesystem is careless.  It would be
		 * difficult to enforce correct nlink usage so just ignore this
		 * condition here
		 */
		if (inode->i_nlink > 0)
			drop_nlink(inode);
		spin_unlock(&fi->lock);
		fuse_invalidate_attr(inode);
		fuse_dir_changed(dir);
		fuse_invalidate_entry_cache(entry);
		fuse_update_ctime(inode);
	} else if (err == -EINTR)
		fuse_invalidate_entry(entry);
	return err;
}