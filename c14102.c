ext4_xattr_block_list(struct dentry *dentry, char *buffer, size_t buffer_size)
{
	struct inode *inode = d_inode(dentry);
	struct buffer_head *bh = NULL;
	int error;
	struct mb_cache *ext4_mb_cache = EXT4_GET_MB_CACHE(inode);

	ea_idebug(inode, "buffer=%p, buffer_size=%ld",
		  buffer, (long)buffer_size);

	error = 0;
	if (!EXT4_I(inode)->i_file_acl)
		goto cleanup;
	ea_idebug(inode, "reading block %llu",
		  (unsigned long long)EXT4_I(inode)->i_file_acl);
	bh = sb_bread(inode->i_sb, EXT4_I(inode)->i_file_acl);
	error = -EIO;
	if (!bh)
		goto cleanup;
	ea_bdebug(bh, "b_count=%d, refcount=%d",
		atomic_read(&(bh->b_count)), le32_to_cpu(BHDR(bh)->h_refcount));
	if (ext4_xattr_check_block(inode, bh)) {
		EXT4_ERROR_INODE(inode, "bad block %llu",
				 EXT4_I(inode)->i_file_acl);
		error = -EFSCORRUPTED;
		goto cleanup;
	}
	ext4_xattr_cache_insert(ext4_mb_cache, bh);
	error = ext4_xattr_list_entries(dentry, BFIRST(bh), buffer, buffer_size);

cleanup:
	brelse(bh);

	return error;
}