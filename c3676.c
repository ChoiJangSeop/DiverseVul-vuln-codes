static long ext4_zero_range(struct file *file, loff_t offset,
			    loff_t len, int mode)
{
	struct inode *inode = file_inode(file);
	handle_t *handle = NULL;
	unsigned int max_blocks;
	loff_t new_size = 0;
	int ret = 0;
	int flags;
	int credits;
	int partial_begin, partial_end;
	loff_t start, end;
	ext4_lblk_t lblk;
	unsigned int blkbits = inode->i_blkbits;

	trace_ext4_zero_range(inode, offset, len, mode);

	if (!S_ISREG(inode->i_mode))
		return -EINVAL;

	/* Call ext4_force_commit to flush all data in case of data=journal. */
	if (ext4_should_journal_data(inode)) {
		ret = ext4_force_commit(inode->i_sb);
		if (ret)
			return ret;
	}

	/*
	 * Round up offset. This is not fallocate, we neet to zero out
	 * blocks, so convert interior block aligned part of the range to
	 * unwritten and possibly manually zero out unaligned parts of the
	 * range.
	 */
	start = round_up(offset, 1 << blkbits);
	end = round_down((offset + len), 1 << blkbits);

	if (start < offset || end > offset + len)
		return -EINVAL;
	partial_begin = offset & ((1 << blkbits) - 1);
	partial_end = (offset + len) & ((1 << blkbits) - 1);

	lblk = start >> blkbits;
	max_blocks = (end >> blkbits);
	if (max_blocks < lblk)
		max_blocks = 0;
	else
		max_blocks -= lblk;

	mutex_lock(&inode->i_mutex);

	/*
	 * Indirect files do not support unwritten extnets
	 */
	if (!(ext4_test_inode_flag(inode, EXT4_INODE_EXTENTS))) {
		ret = -EOPNOTSUPP;
		goto out_mutex;
	}

	if (!(mode & FALLOC_FL_KEEP_SIZE) &&
	     offset + len > i_size_read(inode)) {
		new_size = offset + len;
		ret = inode_newsize_ok(inode, new_size);
		if (ret)
			goto out_mutex;
	}

	flags = EXT4_GET_BLOCKS_CREATE_UNWRIT_EXT;
	if (mode & FALLOC_FL_KEEP_SIZE)
		flags |= EXT4_GET_BLOCKS_KEEP_SIZE;

	/* Wait all existing dio workers, newcomers will block on i_mutex */
	ext4_inode_block_unlocked_dio(inode);
	inode_dio_wait(inode);

	/* Preallocate the range including the unaligned edges */
	if (partial_begin || partial_end) {
		ret = ext4_alloc_file_blocks(file,
				round_down(offset, 1 << blkbits) >> blkbits,
				(round_up((offset + len), 1 << blkbits) -
				 round_down(offset, 1 << blkbits)) >> blkbits,
				new_size, flags, mode);
		if (ret)
			goto out_dio;

	}

	/* Zero range excluding the unaligned edges */
	if (max_blocks > 0) {
		flags |= (EXT4_GET_BLOCKS_CONVERT_UNWRITTEN |
			  EXT4_EX_NOCACHE);

		/*
		 * Prevent page faults from reinstantiating pages we have
		 * released from page cache.
		 */
		down_write(&EXT4_I(inode)->i_mmap_sem);
		/* Now release the pages and zero block aligned part of pages */
		truncate_pagecache_range(inode, start, end - 1);
		inode->i_mtime = inode->i_ctime = ext4_current_time(inode);

		ret = ext4_alloc_file_blocks(file, lblk, max_blocks, new_size,
					     flags, mode);
		up_write(&EXT4_I(inode)->i_mmap_sem);
		if (ret)
			goto out_dio;
	}
	if (!partial_begin && !partial_end)
		goto out_dio;

	/*
	 * In worst case we have to writeout two nonadjacent unwritten
	 * blocks and update the inode
	 */
	credits = (2 * ext4_ext_index_trans_blocks(inode, 2)) + 1;
	if (ext4_should_journal_data(inode))
		credits += 2;
	handle = ext4_journal_start(inode, EXT4_HT_MISC, credits);
	if (IS_ERR(handle)) {
		ret = PTR_ERR(handle);
		ext4_std_error(inode->i_sb, ret);
		goto out_dio;
	}

	inode->i_mtime = inode->i_ctime = ext4_current_time(inode);
	if (new_size) {
		ext4_update_inode_size(inode, new_size);
	} else {
		/*
		* Mark that we allocate beyond EOF so the subsequent truncate
		* can proceed even if the new size is the same as i_size.
		*/
		if ((offset + len) > i_size_read(inode))
			ext4_set_inode_flag(inode, EXT4_INODE_EOFBLOCKS);
	}
	ext4_mark_inode_dirty(handle, inode);

	/* Zero out partial block at the edges of the range */
	ret = ext4_zero_partial_blocks(handle, inode, offset, len);

	if (file->f_flags & O_SYNC)
		ext4_handle_sync(handle);

	ext4_journal_stop(handle);
out_dio:
	ext4_inode_resume_unlocked_dio(inode);
out_mutex:
	mutex_unlock(&inode->i_mutex);
	return ret;
}