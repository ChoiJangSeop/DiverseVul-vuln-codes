int ext4_data_block_valid(struct ext4_sb_info *sbi, ext4_fsblk_t start_blk,
			  unsigned int count)
{
	struct ext4_system_blocks *system_blks;
	int ret;

	/*
	 * Lock the system zone to prevent it being released concurrently
	 * when doing a remount which inverse current "[no]block_validity"
	 * mount option.
	 */
	rcu_read_lock();
	system_blks = rcu_dereference(sbi->system_blks);
	ret = ext4_data_block_valid_rcu(sbi, system_blks, start_blk,
					count);
	rcu_read_unlock();
	return ret;
}