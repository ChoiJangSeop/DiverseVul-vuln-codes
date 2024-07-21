void btrfs_trans_release_chunk_metadata(struct btrfs_trans_handle *trans)
{
	struct btrfs_fs_info *fs_info = trans->fs_info;
	struct btrfs_transaction *cur_trans = trans->transaction;

	if (!trans->chunk_bytes_reserved)
		return;

	WARN_ON_ONCE(!list_empty(&trans->new_bgs));

	btrfs_block_rsv_release(fs_info, &fs_info->chunk_block_rsv,
				trans->chunk_bytes_reserved, NULL);
	atomic64_sub(trans->chunk_bytes_reserved, &cur_trans->chunk_bytes_reserved);
	cond_wake_up(&cur_trans->chunk_reserve_wait);
	trans->chunk_bytes_reserved = 0;
}