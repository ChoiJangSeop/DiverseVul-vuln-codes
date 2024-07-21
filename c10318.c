static int fuse_flush(struct file *file, fl_owner_t id)
{
	struct inode *inode = file_inode(file);
	struct fuse_mount *fm = get_fuse_mount(inode);
	struct fuse_file *ff = file->private_data;
	struct fuse_flush_in inarg;
	FUSE_ARGS(args);
	int err;

	if (is_bad_inode(inode))
		return -EIO;

	err = write_inode_now(inode, 1);
	if (err)
		return err;

	inode_lock(inode);
	fuse_sync_writes(inode);
	inode_unlock(inode);

	err = filemap_check_errors(file->f_mapping);
	if (err)
		return err;

	err = 0;
	if (fm->fc->no_flush)
		goto inval_attr_out;

	memset(&inarg, 0, sizeof(inarg));
	inarg.fh = ff->fh;
	inarg.lock_owner = fuse_lock_owner_id(fm->fc, id);
	args.opcode = FUSE_FLUSH;
	args.nodeid = get_node_id(inode);
	args.in_numargs = 1;
	args.in_args[0].size = sizeof(inarg);
	args.in_args[0].value = &inarg;
	args.force = true;

	err = fuse_simple_request(fm, &args);
	if (err == -ENOSYS) {
		fm->fc->no_flush = 1;
		err = 0;
	}

inval_attr_out:
	/*
	 * In memory i_blocks is not maintained by fuse, if writeback cache is
	 * enabled, i_blocks from cached attr may not be accurate.
	 */
	if (!err && fm->fc->writeback_cache)
		fuse_invalidate_attr(inode);
	return err;
}