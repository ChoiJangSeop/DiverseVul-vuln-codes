int ext4_check_blockref(const char *function, unsigned int line,
			struct inode *inode, __le32 *p, unsigned int max)
{
	__le32 *bref = p;
	unsigned int blk;

	if (ext4_has_feature_journal(inode->i_sb) &&
	    (inode->i_ino ==
	     le32_to_cpu(EXT4_SB(inode->i_sb)->s_es->s_journal_inum)))
		return 0;

	while (bref < p+max) {
		blk = le32_to_cpu(*bref++);
		if (blk &&
		    unlikely(!ext4_data_block_valid(EXT4_SB(inode->i_sb),
						    blk, 1))) {
			ext4_error_inode(inode, function, line, blk,
					 "invalid block");
			return -EFSCORRUPTED;
		}
	}
	return 0;
}