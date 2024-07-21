int fuse_open_common(struct inode *inode, struct file *file, bool isdir)
{
	struct fuse_mount *fm = get_fuse_mount(inode);
	struct fuse_conn *fc = fm->fc;
	int err;
	bool is_wb_truncate = (file->f_flags & O_TRUNC) &&
			  fc->atomic_o_trunc &&
			  fc->writeback_cache;
	bool dax_truncate = (file->f_flags & O_TRUNC) &&
			  fc->atomic_o_trunc && FUSE_IS_DAX(inode);

	err = generic_file_open(inode, file);
	if (err)
		return err;

	if (is_wb_truncate || dax_truncate) {
		inode_lock(inode);
		fuse_set_nowrite(inode);
	}

	if (dax_truncate) {
		down_write(&get_fuse_inode(inode)->i_mmap_sem);
		err = fuse_dax_break_layouts(inode, 0, 0);
		if (err)
			goto out;
	}

	err = fuse_do_open(fm, get_node_id(inode), file, isdir);
	if (!err)
		fuse_finish_open(inode, file);

out:
	if (dax_truncate)
		up_write(&get_fuse_inode(inode)->i_mmap_sem);

	if (is_wb_truncate | dax_truncate) {
		fuse_release_nowrite(inode);
		inode_unlock(inode);
	}

	return err;
}