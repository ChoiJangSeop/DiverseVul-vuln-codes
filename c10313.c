static void fuse_evict_inode(struct inode *inode)
{
	struct fuse_inode *fi = get_fuse_inode(inode);

	truncate_inode_pages_final(&inode->i_data);
	clear_inode(inode);
	if (inode->i_sb->s_flags & SB_ACTIVE) {
		struct fuse_conn *fc = get_fuse_conn(inode);

		if (FUSE_IS_DAX(inode))
			fuse_dax_inode_cleanup(inode);
		if (fi->nlookup) {
			fuse_queue_forget(fc, fi->forget, fi->nodeid,
					  fi->nlookup);
			fi->forget = NULL;
		}
	}
	if (S_ISREG(inode->i_mode) && !is_bad_inode(inode)) {
		WARN_ON(!list_empty(&fi->write_files));
		WARN_ON(!list_empty(&fi->queued_writes));
	}
}