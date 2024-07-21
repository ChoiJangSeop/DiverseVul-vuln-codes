void make_bad_inode(struct inode * inode) 
{
	remove_inode_hash(inode);

	inode->i_mode = S_IFREG;
	inode->i_atime = inode->i_mtime = inode->i_ctime =
		current_fs_time(inode->i_sb);
	inode->i_op = &bad_inode_ops;	
	inode->i_fop = &bad_file_ops;	
}