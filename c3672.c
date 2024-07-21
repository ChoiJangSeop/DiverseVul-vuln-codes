static int ext4_alloc_file_blocks(struct file *file, ext4_lblk_t offset,
				  ext4_lblk_t len, loff_t new_size,
				  int flags, int mode)
{
	struct inode *inode = file_inode(file);
	handle_t *handle;
	int ret = 0;
	int ret2 = 0;
	int retries = 0;
	int depth = 0;
	struct ext4_map_blocks map;
	unsigned int credits;
	loff_t epos;

	map.m_lblk = offset;
	map.m_len = len;
	/*
	 * Don't normalize the request if it can fit in one extent so
	 * that it doesn't get unnecessarily split into multiple
	 * extents.
	 */
	if (len <= EXT_UNWRITTEN_MAX_LEN)
		flags |= EXT4_GET_BLOCKS_NO_NORMALIZE;

	/* Wait all existing dio workers, newcomers will block on i_mutex */
	ext4_inode_block_unlocked_dio(inode);
	inode_dio_wait(inode);

	/*
	 * credits to insert 1 extent into extent tree
	 */
	credits = ext4_chunk_trans_blocks(inode, len);
	/*
	 * We can only call ext_depth() on extent based inodes
	 */
	if (ext4_test_inode_flag(inode, EXT4_INODE_EXTENTS))
		depth = ext_depth(inode);
	else
		depth = -1;

retry:
	while (ret >= 0 && len) {
		/*
		 * Recalculate credits when extent tree depth changes.
		 */
		if (depth >= 0 && depth != ext_depth(inode)) {
			credits = ext4_chunk_trans_blocks(inode, len);
			depth = ext_depth(inode);
		}

		handle = ext4_journal_start(inode, EXT4_HT_MAP_BLOCKS,
					    credits);
		if (IS_ERR(handle)) {
			ret = PTR_ERR(handle);
			break;
		}
		ret = ext4_map_blocks(handle, inode, &map, flags);
		if (ret <= 0) {
			ext4_debug("inode #%lu: block %u: len %u: "
				   "ext4_ext_map_blocks returned %d",
				   inode->i_ino, map.m_lblk,
				   map.m_len, ret);
			ext4_mark_inode_dirty(handle, inode);
			ret2 = ext4_journal_stop(handle);
			break;
		}
		map.m_lblk += ret;
		map.m_len = len = len - ret;
		epos = (loff_t)map.m_lblk << inode->i_blkbits;
		inode->i_ctime = ext4_current_time(inode);
		if (new_size) {
			if (epos > new_size)
				epos = new_size;
			if (ext4_update_inode_size(inode, epos) & 0x1)
				inode->i_mtime = inode->i_ctime;
		} else {
			if (epos > inode->i_size)
				ext4_set_inode_flag(inode,
						    EXT4_INODE_EOFBLOCKS);
		}
		ext4_mark_inode_dirty(handle, inode);
		ret2 = ext4_journal_stop(handle);
		if (ret2)
			break;
	}
	if (ret == -ENOSPC &&
			ext4_should_retry_alloc(inode->i_sb, &retries)) {
		ret = 0;
		goto retry;
	}

	ext4_inode_resume_unlocked_dio(inode);

	return ret > 0 ? ret2 : ret;
}