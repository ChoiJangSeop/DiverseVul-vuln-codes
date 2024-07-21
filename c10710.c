void check_system_chunk(struct btrfs_trans_handle *trans, u64 type)
{
	struct btrfs_transaction *cur_trans = trans->transaction;
	struct btrfs_fs_info *fs_info = trans->fs_info;
	struct btrfs_space_info *info;
	u64 left;
	u64 thresh;
	int ret = 0;
	u64 num_devs;

	/*
	 * Needed because we can end up allocating a system chunk and for an
	 * atomic and race free space reservation in the chunk block reserve.
	 */
	lockdep_assert_held(&fs_info->chunk_mutex);

	info = btrfs_find_space_info(fs_info, BTRFS_BLOCK_GROUP_SYSTEM);
again:
	spin_lock(&info->lock);
	left = info->total_bytes - btrfs_space_info_used(info, true);
	spin_unlock(&info->lock);

	num_devs = get_profile_num_devs(fs_info, type);

	/* num_devs device items to update and 1 chunk item to add or remove */
	thresh = btrfs_calc_metadata_size(fs_info, num_devs) +
		btrfs_calc_insert_metadata_size(fs_info, 1);

	if (left < thresh && btrfs_test_opt(fs_info, ENOSPC_DEBUG)) {
		btrfs_info(fs_info, "left=%llu, need=%llu, flags=%llu",
			   left, thresh, type);
		btrfs_dump_space_info(fs_info, info, 0, 0);
	}

	if (left < thresh) {
		u64 flags = btrfs_system_alloc_profile(fs_info);
		u64 reserved = atomic64_read(&cur_trans->chunk_bytes_reserved);

		/*
		 * If there's not available space for the chunk tree (system
		 * space) and there are other tasks that reserved space for
		 * creating a new system block group, wait for them to complete
		 * the creation of their system block group and release excess
		 * reserved space. We do this because:
		 *
		 * *) We can end up allocating more system chunks than necessary
		 *    when there are multiple tasks that are concurrently
		 *    allocating block groups, which can lead to exhaustion of
		 *    the system array in the superblock;
		 *
		 * *) If we allocate extra and unnecessary system block groups,
		 *    despite being empty for a long time, and possibly forever,
		 *    they end not being added to the list of unused block groups
		 *    because that typically happens only when deallocating the
		 *    last extent from a block group - which never happens since
		 *    we never allocate from them in the first place. The few
		 *    exceptions are when mounting a filesystem or running scrub,
		 *    which add unused block groups to the list of unused block
		 *    groups, to be deleted by the cleaner kthread.
		 *    And even when they are added to the list of unused block
		 *    groups, it can take a long time until they get deleted,
		 *    since the cleaner kthread might be sleeping or busy with
		 *    other work (deleting subvolumes, running delayed iputs,
		 *    defrag scheduling, etc);
		 *
		 * This is rare in practice, but can happen when too many tasks
		 * are allocating blocks groups in parallel (via fallocate())
		 * and before the one that reserved space for a new system block
		 * group finishes the block group creation and releases the space
		 * reserved in excess (at btrfs_create_pending_block_groups()),
		 * other tasks end up here and see free system space temporarily
		 * not enough for updating the chunk tree.
		 *
		 * We unlock the chunk mutex before waiting for such tasks and
		 * lock it again after the wait, otherwise we would deadlock.
		 * It is safe to do so because allocating a system chunk is the
		 * first thing done while allocating a new block group.
		 */
		if (reserved > trans->chunk_bytes_reserved) {
			const u64 min_needed = reserved - thresh;

			mutex_unlock(&fs_info->chunk_mutex);
			wait_event(cur_trans->chunk_reserve_wait,
			   atomic64_read(&cur_trans->chunk_bytes_reserved) <=
			   min_needed);
			mutex_lock(&fs_info->chunk_mutex);
			goto again;
		}

		/*
		 * Ignore failure to create system chunk. We might end up not
		 * needing it, as we might not need to COW all nodes/leafs from
		 * the paths we visit in the chunk tree (they were already COWed
		 * or created in the current transaction for example).
		 */
		ret = btrfs_alloc_chunk(trans, flags);
	}

	if (!ret) {
		ret = btrfs_block_rsv_add(fs_info->chunk_root,
					  &fs_info->chunk_block_rsv,
					  thresh, BTRFS_RESERVE_NO_FLUSH);
		if (!ret) {
			atomic64_add(thresh, &cur_trans->chunk_bytes_reserved);
			trans->chunk_bytes_reserved += thresh;
		}
	}
}