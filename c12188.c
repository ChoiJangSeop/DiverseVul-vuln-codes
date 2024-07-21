static int ext4_clear_blocks(handle_t *handle, struct inode *inode,
			     struct buffer_head *bh,
			     ext4_fsblk_t block_to_free,
			     unsigned long count, __le32 *first,
			     __le32 *last)
{
	__le32 *p;
	int	flags = EXT4_FREE_BLOCKS_VALIDATED;
	int	err;

	if (S_ISDIR(inode->i_mode) || S_ISLNK(inode->i_mode) ||
	    ext4_test_inode_flag(inode, EXT4_INODE_EA_INODE))
		flags |= EXT4_FREE_BLOCKS_FORGET | EXT4_FREE_BLOCKS_METADATA;
	else if (ext4_should_journal_data(inode))
		flags |= EXT4_FREE_BLOCKS_FORGET;

	if (!ext4_data_block_valid(EXT4_SB(inode->i_sb), block_to_free,
				   count)) {
		EXT4_ERROR_INODE(inode, "attempt to clear invalid "
				 "blocks %llu len %lu",
				 (unsigned long long) block_to_free, count);
		return 1;
	}

	err = ext4_ind_truncate_ensure_credits(handle, inode, bh,
				ext4_free_data_revoke_credits(inode, count));
	if (err < 0)
		goto out_err;

	for (p = first; p < last; p++)
		*p = 0;

	ext4_free_blocks(handle, inode, NULL, block_to_free, count, flags);
	return 0;
out_err:
	ext4_std_error(inode->i_sb, err);
	return err;
}