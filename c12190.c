void ext4_free_blocks(handle_t *handle, struct inode *inode,
		      struct buffer_head *bh, ext4_fsblk_t block,
		      unsigned long count, int flags)
{
	struct buffer_head *bitmap_bh = NULL;
	struct super_block *sb = inode->i_sb;
	struct ext4_group_desc *gdp;
	unsigned int overflow;
	ext4_grpblk_t bit;
	struct buffer_head *gd_bh;
	ext4_group_t block_group;
	struct ext4_sb_info *sbi;
	struct ext4_buddy e4b;
	unsigned int count_clusters;
	int err = 0;
	int ret;

	might_sleep();
	if (bh) {
		if (block)
			BUG_ON(block != bh->b_blocknr);
		else
			block = bh->b_blocknr;
	}

	sbi = EXT4_SB(sb);
	if (!(flags & EXT4_FREE_BLOCKS_VALIDATED) &&
	    !ext4_data_block_valid(sbi, block, count)) {
		ext4_error(sb, "Freeing blocks not in datazone - "
			   "block = %llu, count = %lu", block, count);
		goto error_return;
	}

	ext4_debug("freeing block %llu\n", block);
	trace_ext4_free_blocks(inode, block, count, flags);

	if (bh && (flags & EXT4_FREE_BLOCKS_FORGET)) {
		BUG_ON(count > 1);

		ext4_forget(handle, flags & EXT4_FREE_BLOCKS_METADATA,
			    inode, bh, block);
	}

	/*
	 * If the extent to be freed does not begin on a cluster
	 * boundary, we need to deal with partial clusters at the
	 * beginning and end of the extent.  Normally we will free
	 * blocks at the beginning or the end unless we are explicitly
	 * requested to avoid doing so.
	 */
	overflow = EXT4_PBLK_COFF(sbi, block);
	if (overflow) {
		if (flags & EXT4_FREE_BLOCKS_NOFREE_FIRST_CLUSTER) {
			overflow = sbi->s_cluster_ratio - overflow;
			block += overflow;
			if (count > overflow)
				count -= overflow;
			else
				return;
		} else {
			block -= overflow;
			count += overflow;
		}
	}
	overflow = EXT4_LBLK_COFF(sbi, count);
	if (overflow) {
		if (flags & EXT4_FREE_BLOCKS_NOFREE_LAST_CLUSTER) {
			if (count > overflow)
				count -= overflow;
			else
				return;
		} else
			count += sbi->s_cluster_ratio - overflow;
	}

	if (!bh && (flags & EXT4_FREE_BLOCKS_FORGET)) {
		int i;
		int is_metadata = flags & EXT4_FREE_BLOCKS_METADATA;

		for (i = 0; i < count; i++) {
			cond_resched();
			if (is_metadata)
				bh = sb_find_get_block(inode->i_sb, block + i);
			ext4_forget(handle, is_metadata, inode, bh, block + i);
		}
	}

do_more:
	overflow = 0;
	ext4_get_group_no_and_offset(sb, block, &block_group, &bit);

	if (unlikely(EXT4_MB_GRP_BBITMAP_CORRUPT(
			ext4_get_group_info(sb, block_group))))
		return;

	/*
	 * Check to see if we are freeing blocks across a group
	 * boundary.
	 */
	if (EXT4_C2B(sbi, bit) + count > EXT4_BLOCKS_PER_GROUP(sb)) {
		overflow = EXT4_C2B(sbi, bit) + count -
			EXT4_BLOCKS_PER_GROUP(sb);
		count -= overflow;
	}
	count_clusters = EXT4_NUM_B2C(sbi, count);
	bitmap_bh = ext4_read_block_bitmap(sb, block_group);
	if (IS_ERR(bitmap_bh)) {
		err = PTR_ERR(bitmap_bh);
		bitmap_bh = NULL;
		goto error_return;
	}
	gdp = ext4_get_group_desc(sb, block_group, &gd_bh);
	if (!gdp) {
		err = -EIO;
		goto error_return;
	}

	if (in_range(ext4_block_bitmap(sb, gdp), block, count) ||
	    in_range(ext4_inode_bitmap(sb, gdp), block, count) ||
	    in_range(block, ext4_inode_table(sb, gdp),
		     sbi->s_itb_per_group) ||
	    in_range(block + count - 1, ext4_inode_table(sb, gdp),
		     sbi->s_itb_per_group)) {

		ext4_error(sb, "Freeing blocks in system zone - "
			   "Block = %llu, count = %lu", block, count);
		/* err = 0. ext4_std_error should be a no op */
		goto error_return;
	}

	BUFFER_TRACE(bitmap_bh, "getting write access");
	err = ext4_journal_get_write_access(handle, bitmap_bh);
	if (err)
		goto error_return;

	/*
	 * We are about to modify some metadata.  Call the journal APIs
	 * to unshare ->b_data if a currently-committing transaction is
	 * using it
	 */
	BUFFER_TRACE(gd_bh, "get_write_access");
	err = ext4_journal_get_write_access(handle, gd_bh);
	if (err)
		goto error_return;
#ifdef AGGRESSIVE_CHECK
	{
		int i;
		for (i = 0; i < count_clusters; i++)
			BUG_ON(!mb_test_bit(bit + i, bitmap_bh->b_data));
	}
#endif
	trace_ext4_mballoc_free(sb, inode, block_group, bit, count_clusters);

	/* __GFP_NOFAIL: retry infinitely, ignore TIF_MEMDIE and memcg limit. */
	err = ext4_mb_load_buddy_gfp(sb, block_group, &e4b,
				     GFP_NOFS|__GFP_NOFAIL);
	if (err)
		goto error_return;

	/*
	 * We need to make sure we don't reuse the freed block until after the
	 * transaction is committed. We make an exception if the inode is to be
	 * written in writeback mode since writeback mode has weak data
	 * consistency guarantees.
	 */
	if (ext4_handle_valid(handle) &&
	    ((flags & EXT4_FREE_BLOCKS_METADATA) ||
	     !ext4_should_writeback_data(inode))) {
		struct ext4_free_data *new_entry;
		/*
		 * We use __GFP_NOFAIL because ext4_free_blocks() is not allowed
		 * to fail.
		 */
		new_entry = kmem_cache_alloc(ext4_free_data_cachep,
				GFP_NOFS|__GFP_NOFAIL);
		new_entry->efd_start_cluster = bit;
		new_entry->efd_group = block_group;
		new_entry->efd_count = count_clusters;
		new_entry->efd_tid = handle->h_transaction->t_tid;

		ext4_lock_group(sb, block_group);
		mb_clear_bits(bitmap_bh->b_data, bit, count_clusters);
		ext4_mb_free_metadata(handle, &e4b, new_entry);
	} else {
		/* need to update group_info->bb_free and bitmap
		 * with group lock held. generate_buddy look at
		 * them with group lock_held
		 */
		if (test_opt(sb, DISCARD)) {
			err = ext4_issue_discard(sb, block_group, bit, count,
						 NULL);
			if (err && err != -EOPNOTSUPP)
				ext4_msg(sb, KERN_WARNING, "discard request in"
					 " group:%d block:%d count:%lu failed"
					 " with %d", block_group, bit, count,
					 err);
		} else
			EXT4_MB_GRP_CLEAR_TRIMMED(e4b.bd_info);

		ext4_lock_group(sb, block_group);
		mb_clear_bits(bitmap_bh->b_data, bit, count_clusters);
		mb_free_blocks(inode, &e4b, bit, count_clusters);
	}

	ret = ext4_free_group_clusters(sb, gdp) + count_clusters;
	ext4_free_group_clusters_set(sb, gdp, ret);
	ext4_block_bitmap_csum_set(sb, block_group, gdp, bitmap_bh);
	ext4_group_desc_csum_set(sb, block_group, gdp);
	ext4_unlock_group(sb, block_group);

	if (sbi->s_log_groups_per_flex) {
		ext4_group_t flex_group = ext4_flex_group(sbi, block_group);
		atomic64_add(count_clusters,
			     &sbi_array_rcu_deref(sbi, s_flex_groups,
						  flex_group)->free_clusters);
	}

	/*
	 * on a bigalloc file system, defer the s_freeclusters_counter
	 * update to the caller (ext4_remove_space and friends) so they
	 * can determine if a cluster freed here should be rereserved
	 */
	if (!(flags & EXT4_FREE_BLOCKS_RERESERVE_CLUSTER)) {
		if (!(flags & EXT4_FREE_BLOCKS_NO_QUOT_UPDATE))
			dquot_free_block(inode, EXT4_C2B(sbi, count_clusters));
		percpu_counter_add(&sbi->s_freeclusters_counter,
				   count_clusters);
	}

	ext4_mb_unload_buddy(&e4b);

	/* We dirtied the bitmap block */
	BUFFER_TRACE(bitmap_bh, "dirtied bitmap block");
	err = ext4_handle_dirty_metadata(handle, NULL, bitmap_bh);

	/* And the group descriptor block */
	BUFFER_TRACE(gd_bh, "dirtied group descriptor block");
	ret = ext4_handle_dirty_metadata(handle, NULL, gd_bh);
	if (!err)
		err = ret;

	if (overflow && !err) {
		block += count;
		count = overflow;
		put_bh(bitmap_bh);
		goto do_more;
	}
error_return:
	brelse(bitmap_bh);
	ext4_std_error(sb, err);
	return;
}