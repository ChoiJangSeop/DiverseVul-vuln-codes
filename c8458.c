static int __ext4_expand_extra_isize(struct inode *inode,
				     unsigned int new_extra_isize,
				     struct ext4_iloc *iloc,
				     handle_t *handle, int *no_expand)
{
	struct ext4_inode *raw_inode;
	struct ext4_xattr_ibody_header *header;
	int error;

	raw_inode = ext4_raw_inode(iloc);

	header = IHDR(inode, raw_inode);

	/* No extended attributes present */
	if (!ext4_test_inode_state(inode, EXT4_STATE_XATTR) ||
	    header->h_magic != cpu_to_le32(EXT4_XATTR_MAGIC)) {
		memset((void *)raw_inode + EXT4_GOOD_OLD_INODE_SIZE +
		       EXT4_I(inode)->i_extra_isize, 0,
		       new_extra_isize - EXT4_I(inode)->i_extra_isize);
		EXT4_I(inode)->i_extra_isize = new_extra_isize;
		return 0;
	}

	/* try to expand with EAs present */
	error = ext4_expand_extra_isize_ea(inode, new_extra_isize,
					   raw_inode, handle);
	if (error) {
		/*
		 * Inode size expansion failed; don't try again
		 */
		*no_expand = 1;
	}

	return error;
}