int ext4_xattr_ibody_inline_set(handle_t *handle, struct inode *inode,
				struct ext4_xattr_info *i,
				struct ext4_xattr_ibody_find *is)
{
	struct ext4_xattr_ibody_header *header;
	struct ext4_xattr_search *s = &is->s;
	int error;

	if (EXT4_I(inode)->i_extra_isize == 0)
		return -ENOSPC;
	error = ext4_xattr_set_entry(i, s, handle, inode, false /* is_block */);
	if (error) {
		if (error == -ENOSPC &&
		    ext4_has_inline_data(inode)) {
			error = ext4_try_to_evict_inline_data(handle, inode,
					EXT4_XATTR_LEN(strlen(i->name) +
					EXT4_XATTR_SIZE(i->value_len)));
			if (error)
				return error;
			error = ext4_xattr_ibody_find(inode, i, is);
			if (error)
				return error;
			error = ext4_xattr_set_entry(i, s, handle, inode,
						     false /* is_block */);
		}
		if (error)
			return error;
	}
	header = IHDR(inode, ext4_raw_inode(&is->iloc));
	if (!IS_LAST_ENTRY(s->first)) {
		header->h_magic = cpu_to_le32(EXT4_XATTR_MAGIC);
		ext4_set_inode_state(inode, EXT4_STATE_XATTR);
	} else {
		header->h_magic = cpu_to_le32(0);
		ext4_clear_inode_state(inode, EXT4_STATE_XATTR);
	}
	return 0;
}