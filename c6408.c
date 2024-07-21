int ext4_try_to_evict_inline_data(handle_t *handle,
				  struct inode *inode,
				  int needed)
{
	int error;
	struct ext4_xattr_entry *entry;
	struct ext4_inode *raw_inode;
	struct ext4_iloc iloc;

	error = ext4_get_inode_loc(inode, &iloc);
	if (error)
		return error;

	raw_inode = ext4_raw_inode(&iloc);
	entry = (struct ext4_xattr_entry *)((void *)raw_inode +
					    EXT4_I(inode)->i_inline_off);
	if (EXT4_XATTR_LEN(entry->e_name_len) +
	    EXT4_XATTR_SIZE(le32_to_cpu(entry->e_value_size)) < needed) {
		error = -ENOSPC;
		goto out;
	}

	error = ext4_convert_inline_data_nolock(handle, inode, &iloc);
out:
	brelse(iloc.bh);
	return error;
}