static int create_new_entry(struct fuse_mount *fm, struct fuse_args *args,
			    struct inode *dir, struct dentry *entry,
			    umode_t mode)
{
	struct fuse_entry_out outarg;
	struct inode *inode;
	struct dentry *d;
	int err;
	struct fuse_forget_link *forget;

	forget = fuse_alloc_forget();
	if (!forget)
		return -ENOMEM;

	memset(&outarg, 0, sizeof(outarg));
	args->nodeid = get_node_id(dir);
	args->out_numargs = 1;
	args->out_args[0].size = sizeof(outarg);
	args->out_args[0].value = &outarg;
	err = fuse_simple_request(fm, args);
	if (err)
		goto out_put_forget_req;

	err = -EIO;
	if (invalid_nodeid(outarg.nodeid) || fuse_invalid_attr(&outarg.attr))
		goto out_put_forget_req;

	if ((outarg.attr.mode ^ mode) & S_IFMT)
		goto out_put_forget_req;

	inode = fuse_iget(dir->i_sb, outarg.nodeid, outarg.generation,
			  &outarg.attr, entry_attr_timeout(&outarg), 0);
	if (!inode) {
		fuse_queue_forget(fm->fc, forget, outarg.nodeid, 1);
		return -ENOMEM;
	}
	kfree(forget);

	d_drop(entry);
	d = d_splice_alias(inode, entry);
	if (IS_ERR(d))
		return PTR_ERR(d);

	if (d) {
		fuse_change_entry_timeout(d, &outarg);
		dput(d);
	} else {
		fuse_change_entry_timeout(entry, &outarg);
	}
	fuse_dir_changed(dir);
	return 0;

 out_put_forget_req:
	kfree(forget);
	return err;
}