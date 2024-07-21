ext4_xattr_ibody_get(struct inode *inode, int name_index, const char *name,
		     void *buffer, size_t buffer_size)
{
	struct ext4_xattr_ibody_header *header;
	struct ext4_xattr_entry *entry;
	struct ext4_inode *raw_inode;
	struct ext4_iloc iloc;
	size_t size;
	void *end;
	int error;

	if (!ext4_test_inode_state(inode, EXT4_STATE_XATTR))
		return -ENODATA;
	error = ext4_get_inode_loc(inode, &iloc);
	if (error)
		return error;
	raw_inode = ext4_raw_inode(&iloc);
	header = IHDR(inode, raw_inode);
	end = (void *)raw_inode + EXT4_SB(inode->i_sb)->s_inode_size;
	error = xattr_check_inode(inode, header, end);
	if (error)
		goto cleanup;
	entry = IFIRST(header);
	error = xattr_find_entry(inode, &entry, end, name_index, name, 0);
	if (error)
		goto cleanup;
	size = le32_to_cpu(entry->e_value_size);
	if (buffer) {
		error = -ERANGE;
		if (size > buffer_size)
			goto cleanup;
		if (entry->e_value_inum) {
			error = ext4_xattr_inode_get(inode, entry, buffer,
						     size);
			if (error)
				goto cleanup;
		} else {
			memcpy(buffer, (void *)IFIRST(header) +
			       le16_to_cpu(entry->e_value_offs), size);
		}
	}
	error = size;

cleanup:
	brelse(iloc.bh);
	return error;
}