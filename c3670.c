long ext4_fallocate(struct file *file, int mode, loff_t offset, loff_t len)
{
	struct inode *inode = file_inode(file);
	loff_t new_size = 0;
	unsigned int max_blocks;
	int ret = 0;
	int flags;
	ext4_lblk_t lblk;
	unsigned int blkbits = inode->i_blkbits;

	/*
	 * Encrypted inodes can't handle collapse range or insert
	 * range since we would need to re-encrypt blocks with a
	 * different IV or XTS tweak (which are based on the logical
	 * block number).
	 *
	 * XXX It's not clear why zero range isn't working, but we'll
	 * leave it disabled for encrypted inodes for now.  This is a
	 * bug we should fix....
	 */
	if (ext4_encrypted_inode(inode) &&
	    (mode & (FALLOC_FL_COLLAPSE_RANGE | FALLOC_FL_INSERT_RANGE |
		     FALLOC_FL_ZERO_RANGE)))
		return -EOPNOTSUPP;

	/* Return error if mode is not supported */
	if (mode & ~(FALLOC_FL_KEEP_SIZE | FALLOC_FL_PUNCH_HOLE |
		     FALLOC_FL_COLLAPSE_RANGE | FALLOC_FL_ZERO_RANGE |
		     FALLOC_FL_INSERT_RANGE))
		return -EOPNOTSUPP;

	if (mode & FALLOC_FL_PUNCH_HOLE)
		return ext4_punch_hole(inode, offset, len);

	ret = ext4_convert_inline_data(inode);
	if (ret)
		return ret;

	if (mode & FALLOC_FL_COLLAPSE_RANGE)
		return ext4_collapse_range(inode, offset, len);

	if (mode & FALLOC_FL_INSERT_RANGE)
		return ext4_insert_range(inode, offset, len);

	if (mode & FALLOC_FL_ZERO_RANGE)
		return ext4_zero_range(file, offset, len, mode);

	trace_ext4_fallocate_enter(inode, offset, len, mode);
	lblk = offset >> blkbits;
	/*
	 * We can't just convert len to max_blocks because
	 * If blocksize = 4096 offset = 3072 and len = 2048
	 */
	max_blocks = (EXT4_BLOCK_ALIGN(len + offset, blkbits) >> blkbits)
		- lblk;

	flags = EXT4_GET_BLOCKS_CREATE_UNWRIT_EXT;
	if (mode & FALLOC_FL_KEEP_SIZE)
		flags |= EXT4_GET_BLOCKS_KEEP_SIZE;

	mutex_lock(&inode->i_mutex);

	/*
	 * We only support preallocation for extent-based files only
	 */
	if (!(ext4_test_inode_flag(inode, EXT4_INODE_EXTENTS))) {
		ret = -EOPNOTSUPP;
		goto out;
	}

	if (!(mode & FALLOC_FL_KEEP_SIZE) &&
	     offset + len > i_size_read(inode)) {
		new_size = offset + len;
		ret = inode_newsize_ok(inode, new_size);
		if (ret)
			goto out;
	}

	ret = ext4_alloc_file_blocks(file, lblk, max_blocks, new_size,
				     flags, mode);
	if (ret)
		goto out;

	if (file->f_flags & O_SYNC && EXT4_SB(inode->i_sb)->s_journal) {
		ret = jbd2_complete_transaction(EXT4_SB(inode->i_sb)->s_journal,
						EXT4_I(inode)->i_sync_tid);
	}
out:
	mutex_unlock(&inode->i_mutex);
	trace_ext4_fallocate_exit(inode, offset, max_blocks, ret);
	return ret;
}