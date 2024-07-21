ext4_xattr_block_get(struct inode *inode, int name_index, const char *name,
		     void *buffer, size_t buffer_size)
{
	struct buffer_head *bh = NULL;
	struct ext4_xattr_entry *entry;
	size_t size;
	void *end;
	int error;
	struct mb_cache *ea_block_cache = EA_BLOCK_CACHE(inode);

	ea_idebug(inode, "name=%d.%s, buffer=%p, buffer_size=%ld",
		  name_index, name, buffer, (long)buffer_size);

	error = -ENODATA;
	if (!EXT4_I(inode)->i_file_acl)
		goto cleanup;
	ea_idebug(inode, "reading block %llu",
		  (unsigned long long)EXT4_I(inode)->i_file_acl);
	bh = sb_bread(inode->i_sb, EXT4_I(inode)->i_file_acl);
	if (!bh)
		goto cleanup;
	ea_bdebug(bh, "b_count=%d, refcount=%d",
		atomic_read(&(bh->b_count)), le32_to_cpu(BHDR(bh)->h_refcount));
	error = ext4_xattr_check_block(inode, bh);
	if (error)
		goto cleanup;
	ext4_xattr_block_cache_insert(ea_block_cache, bh);
	entry = BFIRST(bh);
	end = bh->b_data + bh->b_size;
	error = xattr_find_entry(inode, &entry, end, name_index, name, 1);
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
			memcpy(buffer, bh->b_data +
			       le16_to_cpu(entry->e_value_offs), size);
		}
	}
	error = size;

cleanup:
	brelse(bh);
	return error;
}