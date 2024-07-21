static int fuse_rmdir(struct inode *dir, struct dentry *entry)
{
	int err;
	struct fuse_mount *fm = get_fuse_mount(dir);
	FUSE_ARGS(args);

	args.opcode = FUSE_RMDIR;
	args.nodeid = get_node_id(dir);
	args.in_numargs = 1;
	args.in_args[0].size = entry->d_name.len + 1;
	args.in_args[0].value = entry->d_name.name;
	err = fuse_simple_request(fm, &args);
	if (!err) {
		clear_nlink(d_inode(entry));
		fuse_dir_changed(dir);
		fuse_invalidate_entry_cache(entry);
	} else if (err == -EINTR)
		fuse_invalidate_entry(entry);
	return err;
}